#include <cute.h>
using namespace Cute;

// Parses all the Cute headers and generates documentation pages in .md format.

#include "internal/cute_file_system_internal.h"

static bool s_is_space(int cp)
{
	switch (cp) {
	case ' ':
	case '\n':
	case '\t':
	case '\v':
	case '\f':
	case '\r': return true;
	default:   return false;
	}
}

static void panic(String msg)
{
	printf("ERROR: %s\n", msg.c_str());
	exit(-1);
}

enum DocType
{
	DOC_EMPTY,
	DOC_ENUM,
	DOC_FUNCTION,
	DOC_STRUCT,
};

struct Doc
{
	DocType type;
	Path path;
	String web_category;
	String title;
	String file;
	String brief;
	int this_index = -1;
	String signature;
	Array<String> param_names;
	Array<String> param_briefs;
	Array<String> enum_entries;
	Array<String> enum_entry_briefs;
	Array<String> member_briefs;
	Array<int> member_functions;
	Array<String> members;
	String return_value;
	String example_brief;
	String example;
	String remarks;
	Array<String> related;

	void emit_title(FILE* fp);
	void emit_brief(FILE* fp);
	void emit_signature(FILE* fp);
	void emit_members(FILE* fp);
	void emit_member_function_links(FILE* fp);
	void emit_params(FILE* fp);
	void emit_enum_entries(FILE* fp);
	void emit_return(FILE* fp);
	void emit_example(FILE* fp);
	void emit_remarks(FILE* fp);
	void emit_related(FILE* fp);
};

struct State
{
	// Header parsing state.
	const char* in;
	const char* end;
	String token;
	String file;
	Map<const char*, const char*> categories;
	Map<const char*, Array<const char*>> category_index_lists;

	// Stored strings for output docs file.
	Doc doc;
	Array<Doc> docs;
	Map<const char*, int> page_to_doc_index;

	void flush_doc()
	{
		Path path = "../docs";
		path.add(doc.web_category);
		String title = doc.title;
		title.to_lower().replace(" ", "_").append(".md");
		path.add(title);
		doc.path = path;
		doc.file = file;
		if (page_to_doc_index.has(sintern(title))) {
			panic(String::fmt("Tried to add a duplicate page for %s.", title.c_str()));
		}
		page_to_doc_index.insert(sintern(title), docs.count());
		docs.add(doc);
		doc = Doc();
	}

	int get_doc_index(String title)
	{
		title.to_lower().replace(" ", "_").append(".md");
		int* index = page_to_doc_index.try_find(sintern(title));
		return index ? *index : -1;
	}

	bool doc_has_link(String title)
	{
		return get_doc_index(title) != -1;
	}

	String doc_get_link(String title)
	{
		int index = get_doc_index(title);
		String link = docs[index].path.c_str();
		String link_prefix = "https://github.com/RandyGaul/cute_framework/blob/master/docs";
		link.replace("../docs", link_prefix.c_str());
		return link;
	}

	void clear() { token.clear(); in = end = NULL; }
	bool done() { return in >= end; }
	void append(int ch) { token.append(ch); }
	void ltrim() { while (!done()) { int cp = *in; if (s_is_space(cp)) ++in; else break; } }
	int next() { int cp; in = cf_decode_UTF8(in, &cp); return cp; }
	int peek() { int cp; cf_decode_UTF8(in, &cp); return cp; }
	void skip() { int cp; in = cf_decode_UTF8(in, &cp); }
	bool expect(int ch) { int cp = next(); if (cp != ch) { return false; } return true; }
	bool try_next(int ch) { int cp; const char* next = cf_decode_UTF8(in, &cp); if (cp == ch) { in = next; return true; } return false; }
};

State state;
State* s = &state;

String linkify(String text, String scan, bool ticks = true)
{
	if (s->doc_has_link(scan)) {
		String link = s->doc_get_link(scan);
		String coded_link = String::fmt("[%s](%s)", scan.c_str(), link.c_str());
		scan = String::fmt(ticks ? "`%s`" : "%s", scan.c_str());
		text.replace(scan, coded_link);
	}
	return text;
}

void Doc::emit_title(FILE* fp)
{
	fprintf(fp, "# %s |", title.c_str());
	String link = linkify(web_category, web_category, false);
	fprintf(fp, " [%s](https://github.com/RandyGaul/cute_framework/blob/master/docs/%s_readme.md)", web_category.c_str(), link.c_str());
	fprintf(fp, " | [%s](https://github.com/RandyGaul/cute_framework/blob/master/include/%s)\n\n", file.c_str(), file.c_str());
}

void Doc::emit_brief(FILE* fp)
{
	fprintf(fp, "%s\n\n", brief.c_str());
}

void Doc::emit_signature(FILE* fp)
{
	fprintf(fp, "```cpp\n%s\n```\n\n", signature.c_str());
}

void Doc::emit_members(FILE* fp)
{
	if (members.count()) {
		fprintf(fp, "Struct Members | Description\n--- | ---\n");
		for (int i = 0; i < members.count(); ++i) {
			fprintf(fp, "`%s` | %s\n", members[i].c_str(), member_briefs[i].c_str());
		}
		fprintf(fp, "\n");
	}
}

void Doc::emit_member_function_links(FILE* fp)
{
	// This function is not finished.
	if (member_functions.count()) {
		fprintf(fp, "Functions | Description\n--- | ---\n");
		for (int i = 0; i < member_functions.count(); ++i) {
			int index = member_functions[i];
			Doc& doc = s->docs[index];
			// TODO - Linkify title spot here.
			fprintf(fp, "%s | %s\n", doc.title.c_str(), doc.brief.c_str());
		}
		fprintf(fp, "\n");
	}
}

void Doc::emit_params(FILE* fp)
{
	if (param_names.count()) {
		fprintf(fp, "Parameters | Description\n--- | ---\n");
		for (int i = 0; i < param_names.count(); ++i) {
			fprintf(fp, "%s | %s\n", param_names[i].c_str(), param_briefs[i].c_str());
		}
		fprintf(fp, "\n");
	}
}

void Doc::emit_enum_entries(FILE* fp)
{
	fprintf(fp, "## Values\n\n");
	fprintf(fp, "Enum | Description\n--- | ---\n");
	for (int i = 0; i < enum_entries.count(); ++i) {
		fprintf(fp, "%s | %s\n", enum_entries[i].c_str(), enum_entry_briefs[i].c_str());
	}
	fprintf(fp, "\n");
}

void Doc::emit_return(FILE* fp)
{
	if (return_value.len()) {
		fprintf(fp, "## Return Value\n\n%s\n\n", return_value.c_str());
	}
}

void Doc::emit_example(FILE* fp)
{
	if (example.len()) {
		fprintf(fp, "## Code Example\n\n");
		if (example_brief.len()) {
			fprintf(fp, "%s\n\n", example_brief.c_str());
		}
		fprintf(fp, "```cpp%s```\n\n", example.c_str());
	}
}

void Doc::emit_remarks(FILE* fp)
{
	if (remarks.len()) {
		fprintf(fp, "## Remarks\n\n");
		fprintf(fp, "%s\n\n", remarks.c_str());
	}
}

void Doc::emit_related(FILE* fp)
{
	if (related.count()) {
		fprintf(fp, "## Related Pages\n\n");
		for (int i = 0; i < related.count(); ++i) {
			fprintf(fp, "%s\n", related[i].c_str());
		}
	}
}

void parse_token()
{
	s->token.clear();
	while (!s->done()) {
		int cp = s->next();
		if (s_is_space(cp)) {
			if (s->token.len()) {
				return;
			}
		} else {
			s->append(cp);
		}
	}
}

String parse_single_comment_line()
{
	String line;
	if (s->try_next(' ')) {
		if (s->try_next('*')) {
			if (s->try_next('/')) {
				s->in--;
				s->in--;
				return line;
			}
		}
	}
	while (!s->done()) {
		int cp = s->peek();
		if (cp == '\n') {
			break;
		} else {
			s->skip();
			line.add(cp);
		}
	}
	return line.trim();
}

String parse_paragraph(bool example = false)
{
	String paragraph;
	while (!s->done()) {
		int cp = s->peek();
		if (cp == '\n') {
			paragraph.append(cp);
			s->skip();
			if (s->try_next(' ')) {
				if (s->try_next('*')) {
					if (s->try_next('/')) {
						s->in--;
						s->in--;
						break;
					} else {
						s->try_next(' ');
					}
				}
			}
		} else if (cp == '@') {
			break;
		} else if (cp == '*') {
			s->skip();
			if (s->try_next('/')) {
				s->in--;
				s->in--;
				break;
			}
		} else {
			if (cp != '\r') {
				paragraph.append(cp);
			}
			s->skip();
		}
	}
	return !example ? paragraph.trim().replace("\n          ", "\n") : paragraph;
}

void parse_command()
{
	if (s->token == "@brief") {
		s->doc.brief = parse_paragraph();
	} else if (s->token == "@example") {
		s->doc.example_brief = parse_single_comment_line();
		s->doc.example = parse_paragraph(true).replace("\n    ", "\n");
	} else if (s->token == "@remarks") {
		s->doc.remarks = parse_paragraph();
	} else if (s->token == "@related") {
		s->doc.related = parse_paragraph().split(' ');
	} else if (s->token == "@param") {
		parse_token();
		s->doc.param_names.add(s->token);
		s->doc.param_briefs.add(parse_paragraph());
	} else if (s->token == "@return") {
		s->doc.return_value = parse_paragraph();
	} else if (s->token == "@category") {
		s->doc.web_category = parse_paragraph();
		const char* category = sintern(s->doc.web_category);
		if (!s->categories.has(category)) {
			s->categories.insert(category, category);
		}
	} else {
		panic(String::fmt("Found unrecognized command %s.", s->token.c_str()));
	}
}

void parse_enum()
{
	s->doc.type = DOC_ENUM;
	s->doc.title = parse_paragraph();
	while (!s->done()) {
		parse_token();
		if (s->token.first() == '@') {
			parse_command();
		} else if (s->token == "*/") {
			break;
		}
	}
	while (!s->done()) {
		parse_token();
		if (s->token.first() == '@') {
			if (s->token == "@entry") {
				s->doc.enum_entry_briefs.add(parse_paragraph());
			} else if (s->token == "@end") {
				break;
			} else {
				panic(String::fmt("Expected to find @entry or @end, but instead found %s.", s->token.c_str()));
			}
		} else if (s->token.begins_with("CF_ENUM(")) {
			s->token.replace("CF_ENUM(", "");
			s->token.pop(); // Remove ','
			s->doc.enum_entries.add(s->token);
		}
	}
	if (s->doc.enum_entries.count() != s->doc.enum_entry_briefs.count()) {
		int a = s->doc.enum_entries.count();
		int b = s->doc.enum_entry_briefs.count();
		panic(String::fmt("Enum entries count %d did not match entry description count %d.", a, b));
	}
	s->flush_doc();
}

String parse_line()
{
	String line;
	s->try_next('\n');
	while (!s->done()) {
		int cp = s->peek();
		if (cp == '\n') {
			break;
		} else {
			s->skip();
			line.add(cp);
		}
	}
	return line.trim();
}

void parse_comment_block()
{
	s->doc.title = parse_paragraph();
	while (!s->done()) {
		parse_token();
		if (s->token.first() == '@') {
			parse_command();
		} else if (s->token == "*/") {
			break;
		}
	}
}

void parse_function()
{
	s->doc.type = DOC_FUNCTION;
	parse_comment_block();
	s->doc.signature = parse_line().replace("CUTE_API ", "").replace("CUTE_CALL ", "").replace("CUTE_INLINE ", "");
	int index = s->doc.signature.find(" {");
	if (index != -1) {
		s->doc.signature.set_len(index);
	}
	s->flush_doc();
}

void parse_struct()
{
	s->doc.type = DOC_STRUCT;
	parse_comment_block();
	int doc_index = s->docs.count();
	s->flush_doc();
	Doc& doc = s->docs.last();
	while (!s->done()) {
		parse_token();
		if (s->token.first() == '@') {
			if (s->token == "@member") {
				String brief = parse_paragraph();
				parse_token(); // Skip "*/"
				String member = parse_line().replace("CUTE_INLINE ", "").replace(";", "");
				doc.member_briefs.add(brief);
				doc.members.add(member);
			} else if (s->token == "@function") {
				doc.member_functions.add(s->docs.count());
				parse_function();
				s->docs.last().this_index = doc_index;
			} else if (s->token == "@end") {
				break;
			} else {
				panic(String::fmt("Found unexpected command %s while parsing a struct, @member, @end, or @function.", s->token.c_str()));
			}
		}
	}
}

void parse(String header)
{
	size_t sz = 0;
	char* in = fs_read_entire_file_to_memory_and_nul_terminate(header, &sz);
	s->clear();
	s->in = in;
	s->end = in + sz;
	s->file = header;

	while (!s->done()) {
		parse_token();
		if (s->token.first() == '@') {
			if (s->token == "@enum") {
				parse_enum();
			} else if (s->token == "@function") {
				parse_function();
			} else if (s->token == "@struct") {
				parse_struct();
			} else {
				panic(String::fmt("Found unexpected command %s, expected @enum, @function, or @struct.", s->token.c_str()));
			}
		}
	}
}

// Scans text for `things in tick marks like this`. It will replace the tick mark text
// with a page link if an appropriate page link is found based on the title of the page.
String auto_generate_links(String text)
{
	String scan;

	bool found = false;
	for (int i = 0; i < text.len(); ++i) {
		char ch = text[i];
		if (found) {
			if (ch != '`') {
				scan.add(ch);
			} else {
				text = linkify(text, scan);
				found = false;
				scan.clear();
			}
		} else {
			if (ch == '`') {
				found = true;
			}
		}
	}

	return text;
}

int main(int argc, const char** argv)
{
	// Mount the headers folder as "".
	cf_fs_init(argv[0]);
	Path path = cf_fs_get_base_directory();
	path = path.normalize();
#ifdef _MSC_VER
	path.popn(2);
#else
	path.popn(1);
#endif
	path.add("/include");
	cf_fs_mount(path.c_str(), "", true);

	// Parse each header into docs.
	Directory headers = Directory::open("");
	for (const char* file = headers.next(); file; file = headers.next()) {
		if (spext_equ(file, ".h")) {
			parse(file);
		}
	}

	// Auto-generate links throughout the text.
	for (int i = 0; i < s->docs.count(); ++i) {
		Doc& doc = s->docs[i];
		doc.brief = auto_generate_links(doc.brief);
		for (int i = 0; i < doc.param_briefs.count(); ++i) {
			doc.param_briefs[i] = auto_generate_links(doc.param_briefs[i]);
		}
		for (int i = 0; i < doc.enum_entry_briefs.count(); ++i) {
			doc.enum_entry_briefs[i] = auto_generate_links(doc.enum_entry_briefs[i]);
		}
		for (int i = 0; i < doc.member_briefs.count(); ++i) {
			doc.member_briefs[i] = auto_generate_links(doc.member_briefs[i]);
		}
		doc.return_value = auto_generate_links(doc.return_value);
		doc.example_brief = auto_generate_links(doc.example_brief);
		doc.example = auto_generate_links(doc.example);
		doc.remarks = auto_generate_links(doc.remarks);
		for (int i = 0; i < doc.related.count();) {
			String related = doc.related[i];
			if (related == doc.title) {
				doc.related.unordered_remove(i);
				continue;
			}
			doc.related[i] = linkify(related, related, false);
			doc.related[i].append("  ");
			++i;
		}
	}

	// Save each doc file.
	for (int i = 0; i < s->docs.count(); ++i) {
		Doc doc = s->docs[i];
		FILE* fp = fopen(doc.path.c_str(), "wb");
		const char* category = sintern(doc.web_category.c_str());
		if (!s->category_index_lists.has(category)) {
			s->category_index_lists.insert(category);
		}
		Array<const char*>& categories = s->category_index_lists.find(category);
		categories.add(sintern(doc.title));

		if (doc.type == DOC_ENUM) {
			doc.emit_title(fp);
			doc.emit_brief(fp);
			doc.emit_enum_entries(fp);
			doc.emit_example(fp);
			doc.emit_remarks(fp);
			doc.emit_related(fp);
		} else if (doc.type == DOC_FUNCTION) {
			doc.emit_title(fp);
			doc.emit_brief(fp);
			doc.emit_signature(fp);
			doc.emit_params(fp);
			doc.emit_return(fp);
			doc.emit_example(fp);
			doc.emit_remarks(fp);
			doc.emit_related(fp);
		} else if (doc.type == DOC_STRUCT) {
			doc.emit_title(fp);
			doc.emit_brief(fp);
			doc.emit_members(fp);
			doc.emit_member_function_links(fp);
			doc.emit_example(fp);
			doc.emit_remarks(fp);
			doc.emit_related(fp);
		}
		fclose(fp);
	}

	// Save the categories index file.
	{
		Path topic_path = path;
		topic_path.pop();
		topic_path.add("docs/README.md");
		FILE* fp = fopen(topic_path.c_str(), "wb");
		fprintf(fp, "# Categories\n\n");
		const char** categories = s->categories.keys();
		for (int i = 0; i < s->categories.count(); ++i) {
			const char* category = categories[i];
			fprintf(fp, "- [%s](https://github.com/RandyGaul/cute_framework/blob/master/docs/%s/README.md)\n", category, category);
		}
		fclose(fp);
	}

	// Save each category index list.
	{
		const char** categories = s->categories.keys();
		for (int i = 0; i < s->categories.count(); ++i) {
			const char* category = categories[i];
			Array<const char*> index_list = s->category_index_lists.find(category);
			Path topic_path = path;
			topic_path.pop();
			topic_path.add(String::fmt("docs/%s/README.md", category));
			FILE* fp = fopen(topic_path.c_str(), "wb");
			fprintf(fp, "# Pages\n\n");
			for (int i = 0; i < index_list.count(); ++i) {
				String page = index_list[i];
				page.to_lower();
				fprintf(fp, "- [%s](https://github.com/RandyGaul/cute_framework/blob/master/docs/%s/%s.md)\n", page.c_str(), category, page.c_str());
			}
			fclose(fp);
		}
	}

	return 0;
}
