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
	String brief;
	String signature;
	Array<String> param_names;
	Array<String> param_briefs;
	Array<String> enum_entries;
	Array<String> enum_entry_briefs;
	Array<String> member_briefs;
	Array<String> members;
	String return_value;
	String example_brief;
	String example;
	String remarks;
	Array<String> related;

	void emit_title(FILE* fp)
	{
		fprintf(fp, "# %s\n\n", title.c_str());
	}

	void emit_brief(FILE* fp)
	{
		fprintf(fp, "%s\n\n", brief.c_str());
	}

	void emit_signature(FILE* fp)
	{
		fprintf(fp, "```cpp\n%s\n```\n\n", signature.c_str());
	}

	void emit_members(FILE* fp)
	{
		if (param_names.count()) {
			fprintf(fp, "Members | Description\n--- | ---\n");
			for (int i = 0; i < param_names.count(); ++i) {
				fprintf(fp, "`%s` | %s\n", param_names[i].c_str(), param_briefs[i].c_str());
			}
			fprintf(fp, "\n");
		}
	}

	void emit_params(FILE* fp)
	{
		if (param_names.count()) {
			fprintf(fp, "Parameters | Description\n--- | ---\n");
			for (int i = 0; i < param_names.count(); ++i) {
				fprintf(fp, "%s | %s\n", param_names[i].c_str(), param_briefs[i].c_str());
			}
			fprintf(fp, "\n");
		}
	}

	void emit_enum_entries(FILE* fp)
	{
		fprintf(fp, "## Values\n\n");
		fprintf(fp, "Enum | Description\n--- | ---\n");
		for (int i = 0; i < enum_entries.count(); ++i) {
			fprintf(fp, "%s | %s\n", enum_entries[i].c_str(), enum_entry_briefs[i].c_str());
		}
		fprintf(fp, "\n");
	}

	void emit_return(FILE* fp)
	{
		if (return_value.len()) {
			fprintf(fp, "## Return Value\n\n%s\n\n", return_value.c_str());
		}
	}

	void emit_example(FILE* fp)
	{
		if (example.len()) {
			fprintf(fp, "## Code Example\n\n");
			if (example_brief.len()) {
				fprintf(fp, "%s\n\n", example_brief.c_str());
			}
			fprintf(fp, "```cpp%s```\n\n", example.c_str());
		}
	}

	void emit_remarks(FILE* fp)
	{
		if (remarks.len()) {
			fprintf(fp, "## Remarks\n\n");
			fprintf(fp, "%s\n\n", remarks.c_str());
		}
	}

	void emit_related(FILE* fp)
	{
		if (related.count()) {
			fprintf(fp, "## Related Pages\n\n");
			for (int i = 0; i < related.count(); ++i) {
				fprintf(fp, "%s\n", related[i].c_str());
			}
		}
	}
};

struct State
{
	// Header parsing state.
	const char* in;
	const char* end;
	String token;

	// Stored strings for output docs file.
	Doc doc;
	Array<Doc> docs;
	Dictionary<const char*, int> page_to_doc_index;

	void flush_doc()
	{
		Path path = "../docs/gen";
		String title = doc.title;
		title.to_lower().replace(" ", "_").append(".md");
		path.add(title);
		doc.path = path;
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
		link_prefix.fmt_append("/%s", docs[index].web_category.c_str());
		link.replace("../docs/gen", link_prefix.c_str());
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
	return !example ? paragraph.trim().dedup(' ').replace("\n ", "\n") : paragraph;
}

void panic(String msg)
{
	printf("ERROR: %s\n", msg.c_str());
	exit(-1);
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
		s->doc.return_value = String("?");
	} else if (s->token == "@category") {
		s->doc.web_category = parse_paragraph();
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
	s->doc.signature = parse_line().replace("CUTE_API ", "").replace("CUTE_CALL ", "");
	s->flush_doc();
}

void parse_struct()
{
	s->doc.type = DOC_STRUCT;
	parse_comment_block();
	while (!s->done()) {
		parse_token();
		if (s->token.first() == '@member') {
			// WORKING HERE
		} else if (s->token == "*/") {
			break;
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
				panic(String::fmt("Found unexpected command %s, expected @enum, @function, @struct, or @define.", s->token.c_str()));
			}
		}
	}
}

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
	path.popn(2);
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
		doc.return_value = auto_generate_links(doc.return_value);
		doc.example_brief = auto_generate_links(doc.example_brief);
		doc.example = auto_generate_links(doc.example);
		doc.remarks = auto_generate_links(doc.remarks);
		for (int i = 0; i < doc.related.count(); ++i) {
			doc.related[i] = linkify(doc.related[i], doc.related[i], false);
			doc.related[i].append("  ");
		}
	}

	// Save each doc file.
	for (int i = 0; i < s->docs.count(); ++i) {
		Doc doc = s->docs[i];
		FILE* fp = fopen(doc.path.c_str(), "wb");
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
			doc.emit_example(fp);
			doc.emit_remarks(fp);
			doc.emit_related(fp);
		}
		fclose(fp);
	}

	return 0;
}
