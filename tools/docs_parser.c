/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Parses all the Cute headers and generates documentation pages in .md format.
// Only dependency is ckit.h (no CF runtime/SDL required).

#define CKIT_IMPLEMENTATION
#include "cute/ckit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#define dp_mkdir(path) _mkdir(path)
#else
#include <dirent.h>
#include <sys/stat.h>
#define dp_mkdir(path) mkdir(path, 0755)
#endif

// -------------------------------------------------------------------------------------------------
// Helpers

static void panic(const char* msg)
{
	printf("ERROR: %s\n", msg);
	exit(-1);
}

static int s_is_space(int cp)
{
	switch (cp) {
	case ' ':
	case '\n':
	case '\t':
	case '\v':
	case '\f':
	case '\r': return 1;
	default:   return 0;
	}
}

static char* read_file(const char* path, size_t* out_size)
{
	FILE* fp = fopen(path, "rb");
	if (!fp) return NULL;
	fseek(fp, 0, SEEK_END);
	size_t sz = (size_t)ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* data = (char*)malloc(sz + 1);
	fread(data, 1, sz, fp);
	data[sz] = 0;
	fclose(fp);
	if (out_size) *out_size = sz;
	return data;
}

static int dir_exists(const char* path)
{
#ifdef _WIN32
	struct _finddata_t fd;
	intptr_t h = _findfirst(path, &fd);
	if (h == -1) return 0;
	int result = (fd.attrib & _A_SUBDIR) != 0;
	_findclose(h);
	return result;
#else
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

// -------------------------------------------------------------------------------------------------
// Doc types

enum { DOC_EMPTY, DOC_ENUM, DOC_FUNCTION, DOC_STRUCT };

typedef struct Doc
{
	int type;
	char* path;           // sdyna
	char* web_category;   // sdyna
	char* title;          // sdyna
	char* file;           // sdyna
	char* brief;          // sdyna
	int this_index;
	char* signature;      // sdyna
	char** param_names;   // dyna of sdyna
	char** param_briefs;  // dyna of sdyna
	char** enum_entries;  // dyna of sdyna
	char** enum_entry_briefs; // dyna of sdyna
	char** member_briefs; // dyna of sdyna
	int* member_functions; // dyna
	char** members;       // dyna of sdyna
	char* return_value;   // sdyna
	char* example_brief;  // sdyna
	char* example;        // sdyna
	char* remarks;        // sdyna
	char** related;       // dyna of sdyna
} Doc;

static Doc make_doc()
{
	Doc d;
	memset(&d, 0, sizeof(d));
	d.this_index = -1;
	return d;
}

// -------------------------------------------------------------------------------------------------
// State

typedef struct State
{
	const char* in;
	const char* end;
	char* token;            // sdyna
	char* file;             // sdyna

	CK_MAP(const char*) categories;
	CK_MAP(const char**) category_index_lists; // value is dyna array of interned strings

	Doc doc;
	Doc* docs; // dyna
	CK_MAP(int) page_to_doc_index;
} State;

static State state;
static State* s = &state;

// -------------------------------------------------------------------------------------------------
// Relative path

static const char* g_relative_path;

static const char* get_relative_path()
{
	return g_relative_path;
}

// -------------------------------------------------------------------------------------------------
// State helpers

static void flush_doc()
{
	char* path = smake(get_relative_path());
	char ch = '/';
	spush(path, ch);
	sappend(path, s->doc.web_category);

	char* title_lower = smake(s->doc.title);
	stolower(title_lower);
	sreplace(title_lower, " ", "_");
	sappend(title_lower, ".md");

	spush(path, ch);
	sappend(path, title_lower);

	sfree(s->doc.path);
	s->doc.path = path;
	sfree(s->doc.file);
	s->doc.file = smake(s->file);

	const char* key = sintern(title_lower);
	if (map_has(s->page_to_doc_index, (uint64_t)key)) {
		int idx = map_get(s->page_to_doc_index, (uint64_t)key);
		char* msg = sfmake("Tried to add a duplicate page for %s (found in file %s, previously seen in file %s).",
			title_lower, s->doc.file, s->docs[idx].file);
		panic(msg);
	}
	int count = asize(s->docs);
	map_set(s->page_to_doc_index, (uint64_t)key, count);
	apush(s->docs, s->doc);
	s->doc = make_doc();
	sfree(title_lower);
}

static int get_doc_index(const char* title_raw)
{
	char* title = smake(title_raw);
	stolower(title);
	sreplace(title, " ", "_");
	sappend(title, ".md");
	const char* key = sintern(title);
	int* ptr = map_get_ptr(s->page_to_doc_index, (uint64_t)key);
	sfree(title);
	return ptr ? *ptr : -1;
}

static int doc_has_link(const char* title)
{
	return get_doc_index(title) != -1;
}

static char* doc_get_link(const char* title)
{
	int index = get_doc_index(title);
	char* link = smake(s->docs[index].path);
	sreplace(link, get_relative_path(), "");
	return link;
}

static int has_doc(const char* file)
{
	const char* key = sintern(file);
	return map_has(s->page_to_doc_index, (uint64_t)key);
}

// -------------------------------------------------------------------------------------------------
// Parsing helpers

static void state_clear() { sfree(s->token); s->token = NULL; s->in = s->end = NULL; }
static int state_done() { return s->in >= s->end; }
static void state_append(int ch) { spush(s->token, (char)ch); }
static void state_ltrim() { while (!state_done()) { int cp = *s->in; if (s_is_space(cp)) ++s->in; else break; } }
static int state_next() { int cp; s->in = cf_decode_UTF8(s->in, &cp); return cp; }
static int state_peek() { int cp; cf_decode_UTF8(s->in, &cp); return cp; }
static void state_skip() { int cp; s->in = cf_decode_UTF8(s->in, &cp); (void)cp; }
static int state_expect(int ch) { int cp = state_next(); return cp == ch; }
static int state_try_next(int ch) { int cp; const char* next = cf_decode_UTF8(s->in, &cp); if (cp == ch) { s->in = next; return 1; } return 0; }

// -------------------------------------------------------------------------------------------------
// Linkify

static char* linkify(char* text, const char* scan_str, int ticks)
{
	if (doc_has_link(scan_str)) {
		char* link = doc_get_link(scan_str);
		char* coded_link = sfmake("[%s](..%s)", scan_str, link);
		char* scan_fmt = ticks ? sfmake("`%s`", scan_str) : smake(scan_str);
		sreplace(text, scan_fmt, coded_link);
		sfree(link);
		sfree(coded_link);
		sfree(scan_fmt);
	}
	return text;
}

// -------------------------------------------------------------------------------------------------
// Emit functions

static void emit_title(FILE* fp, Doc* doc)
{
	fprintf(fp, "[//]: # (This file is automatically generated by Cute Framework's docs parser.)\n");
	fprintf(fp, "[//]: # (Do not edit this file by hand!)\n");
	fprintf(fp, "[//]: # (See: https://github.com/RandyGaul/cute_framework/blob/master/tools/docs_parser.c)\n");
	fprintf(fp, "# %s\n\n", doc->title);
	char* link_lower = smake(doc->web_category);
	stolower(link_lower);
	char* link = linkify(smake(doc->web_category), doc->web_category, 0);
	fprintf(fp, "Category: [%s](../api_reference.md#%s)  \n", doc->web_category, link_lower);
	fprintf(fp, "GitHub: [%s](https://github.com/RandyGaul/cute_framework/blob/master/include/%s)  \n---\n\n", doc->file, doc->file);
	sfree(link_lower);
	sfree(link);
}

static void emit_brief(FILE* fp, Doc* doc)
{
	fprintf(fp, "%s\n\n", doc->brief);
}

static void emit_signature(FILE* fp, Doc* doc)
{
	fprintf(fp, "```cpp\n%s\n```\n\n", doc->signature);
}

static void emit_members(FILE* fp, Doc* doc)
{
	if (asize(doc->members)) {
		fprintf(fp, "Struct Members | Description\n--- | ---\n");
		for (int i = 0; i < asize(doc->members); ++i) {
			fprintf(fp, "`%s` | %s\n", doc->members[i], doc->member_briefs[i]);
		}
		fprintf(fp, "\n");
	}
}

static void emit_member_function_links(FILE* fp, Doc* doc)
{
	if (asize(doc->member_functions)) {
		fprintf(fp, "Functions | Description\n--- | ---\n");
		for (int i = 0; i < asize(doc->member_functions); ++i) {
			int index = doc->member_functions[i];
			Doc* d = &s->docs[index];
			fprintf(fp, "%s | %s\n", d->title, d->brief);
		}
		fprintf(fp, "\n");
	}
}

static void emit_params(FILE* fp, Doc* doc)
{
	if (asize(doc->param_names)) {
		fprintf(fp, "Parameters | Description\n--- | ---\n");
		for (int i = 0; i < asize(doc->param_names); ++i) {
			fprintf(fp, "`%s` | %s\n", doc->param_names[i], doc->param_briefs[i]);
		}
		fprintf(fp, "\n");
	}
}

static void emit_enum_entries(FILE* fp, Doc* doc)
{
	fprintf(fp, "## Values\n\n");
	fprintf(fp, "Enum | Description\n--- | ---\n");
	for (int i = 0; i < asize(doc->enum_entries); ++i) {
		fprintf(fp, "`%s` | %s\n", doc->enum_entries[i], doc->enum_entry_briefs[i]);
	}
	fprintf(fp, "\n");
}

static void emit_return(FILE* fp, Doc* doc)
{
	if (doc->return_value && slen(doc->return_value)) {
		fprintf(fp, "## Return Value\n\n%s\n\n", doc->return_value);
	}
}

static void emit_example(FILE* fp, Doc* doc)
{
	if (doc->example && slen(doc->example)) {
		fprintf(fp, "## Code Example\n\n");
		if (doc->example_brief && slen(doc->example_brief)) {
			fprintf(fp, "%s\n\n", doc->example_brief);
		}
		fprintf(fp, "```cpp%s```\n\n", doc->example);
	}
}

static void emit_remarks(FILE* fp, Doc* doc)
{
	if (doc->remarks && slen(doc->remarks)) {
		fprintf(fp, "## Remarks\n\n");
		fprintf(fp, "%s\n\n", doc->remarks);
	}
}

static void emit_related(FILE* fp, Doc* doc)
{
	if (asize(doc->related)) {
		fprintf(fp, "## Related Pages\n\n");
		for (int i = 0; i < asize(doc->related); ++i) {
			fprintf(fp, "%s\n", doc->related[i]);
		}
	}
}

// -------------------------------------------------------------------------------------------------
// Parsing

static void parse_token()
{
	sfree(s->token);
	s->token = NULL;
	while (!state_done()) {
		int cp = state_next();
		if (s_is_space(cp)) {
			if (s->token && slen(s->token)) {
				return;
			}
		} else {
			state_append(cp);
		}
	}
}

static char* parse_single_comment_line()
{
	char* line = NULL;
	if (state_try_next(' ')) {
		if (state_try_next('*')) {
			if (state_try_next('/')) {
				s->in--;
				s->in--;
				return line;
			}
		}
	}
	while (!state_done()) {
		int cp = state_peek();
		if (cp == '\n') {
			break;
		} else {
			state_skip();
			spush(line, (char)cp);
		}
	}
	strim(line);
	return line;
}

static char* parse_paragraph(int is_example)
{
	char* paragraph = NULL;
	while (!state_done()) {
		int cp = state_peek();
		if (cp == '\n') {
			spush(paragraph, (char)cp);
			state_skip();
			// Try to consume comment prefix: optional space + asterisk + optional space
			// This handles both " * " (proper format) and "* " (malformed without leading space)
			state_try_next(' ');
			if (state_try_next('*')) {
				if (state_try_next('/')) {
					s->in--;
					s->in--;
					break;
				} else {
					state_try_next(' ');
				}
			}
		} else if (cp == '@') {
			break;
		} else if (cp == '*') {
			state_skip();
			if (state_try_next('/')) {
				s->in--;
				s->in--;
				break;
			} else {
				// Preserve asterisks that are part of content (not comment delimiters)
				spush(paragraph, '*');
			}
		} else {
			if (cp != '\r') {
				spush(paragraph, (char)cp);
			}
			state_skip();
		}
	}
	if (!is_example && paragraph) {
		strim(paragraph);
		sreplace(paragraph, "\n          ", "\n");
	}
	return paragraph;
}

static void parse_command()
{
	if (sequ(s->token, "@brief")) {
		sfree(s->doc.brief);
		s->doc.brief = parse_paragraph(0);
	} else if (sequ(s->token, "@example")) {
		sfree(s->doc.example_brief);
		s->doc.example_brief = parse_single_comment_line();
		char* ex = parse_paragraph(1);
		sreplace(ex, "\n    ", "\n");
		sfree(s->doc.example);
		s->doc.example = ex;
	} else if (sequ(s->token, "@remarks")) {
		sfree(s->doc.remarks);
		s->doc.remarks = parse_paragraph(0);
	} else if (sequ(s->token, "@related")) {
		char* text = parse_paragraph(0);
		// Split by space.
		char** parts = ssplit(text, ' ');
		for (int i = 0; i < asize(parts); ++i) {
			apush(s->doc.related, parts[i]);
		}
		afree(parts); // Free the outer array, not the inner strings (they're moved to related)
		sfree(text);
	} else if (sequ(s->token, "@param")) {
		parse_token();
		apush(s->doc.param_names, smake(s->token));
		apush(s->doc.param_briefs, parse_paragraph(0));
	} else if (sequ(s->token, "@return")) {
		sfree(s->doc.return_value);
		s->doc.return_value = parse_paragraph(0);
	} else if (sequ(s->token, "@category")) {
		sfree(s->doc.web_category);
		s->doc.web_category = parse_paragraph(0);
		const char* category = sintern(s->doc.web_category);
		if (!map_has(s->categories, (uint64_t)category)) {
			map_set(s->categories, (uint64_t)category, category);
		}
	} else {
		char* msg = sfmake("Found unrecognized command %s.", s->token);
		panic(msg);
	}
}

static void parse_enum()
{
	s->doc.type = DOC_ENUM;
	sfree(s->doc.title);
	s->doc.title = parse_paragraph(0);
	while (!state_done()) {
		parse_token();
		if (s->token && slen(s->token) && sfirst(s->token) == '@') {
			parse_command();
		} else if (sequ(s->token, "*/")) {
			break;
		}
	}
	while (!state_done()) {
		parse_token();
		if (s->token && slen(s->token) && sfirst(s->token) == '@') {
			if (sequ(s->token, "@entry")) {
				apush(s->doc.enum_entry_briefs, parse_paragraph(0));
			} else if (sequ(s->token, "@end")) {
				break;
			} else {
				char* msg = sfmake("Expected to find @entry or @end, but instead found %s.", s->token);
				panic(msg);
			}
		} else if (s->token && sprefix(s->token, "CF_ENUM(")) {
			sreplace(s->token, "CF_ENUM(", "");
			spop(s->token); // Remove ','
			char* entry = sfmake("CF_%s", s->token);
			apush(s->doc.enum_entries, entry);
		}
	}
	if (asize(s->doc.enum_entries) != asize(s->doc.enum_entry_briefs)) {
		int a = asize(s->doc.enum_entries);
		int b = asize(s->doc.enum_entry_briefs);
		char* msg = sfmake("Enum entries count %d did not match entry description count %d.", a, b);
		panic(msg);
	}
	flush_doc();
}

static char* parse_line()
{
	char* line = NULL;
	state_try_next('\n');
	while (!state_done()) {
		int cp = state_peek();
		if (cp == '\n') {
			break;
		} else {
			state_skip();
			spush(line, (char)cp);
		}
	}
	strim(line);
	return line;
}

static void parse_comment_block()
{
	sfree(s->doc.title);
	s->doc.title = parse_paragraph(0);
	int iter = 0;
	while (!state_done()) {
		iter++;
		parse_token();
		if (s->token && slen(s->token) && sfirst(s->token) == '@') {
			parse_command();
		} else if (sequ(s->token, "*/")) {
			break;
		}
	}
}

static void parse_function()
{
	s->doc.type = DOC_FUNCTION;
	parse_comment_block();
	char* sig = parse_line();
	sreplace(sig, "CF_API ", "");
	sreplace(sig, "CF_CALL ", "");
	sreplace(sig, "CF_INLINE ", "");
	char* brace = sfind(sig, " {");
	if (brace) {
		asetlen(sig, (int)(brace - sig));
	}
	sfree(s->doc.signature);
	s->doc.signature = sig;
	flush_doc();
}

static void parse_struct()
{
	s->doc.type = DOC_STRUCT;
	parse_comment_block();
	int doc_index = asize(s->docs);
	flush_doc();
	Doc* doc = &s->docs[asize(s->docs) - 1];
	while (!state_done()) {
		parse_token();
		if (s->token && slen(s->token) && sfirst(s->token) == '@') {
			if (sequ(s->token, "@member")) {
				char* brief = parse_paragraph(0);
				parse_token(); // Skip "*/"
				char* member = parse_line();
				sreplace(member, "CF_INLINE ", "");
				sreplace(member, ";", "");
				apush(doc->member_briefs, brief);
				apush(doc->members, member);
			} else if (sequ(s->token, "@function")) {
				apush(doc->member_functions, asize(s->docs));
				parse_function();
				s->docs[asize(s->docs) - 1].this_index = doc_index;
				doc = &s->docs[doc_index]; // Re-fetch, docs array may have reallocated.
			} else if (sequ(s->token, "@end")) {
				break;
			} else {
				char* msg = sfmake("Found unexpected command %s while parsing a struct, @member, @end, or @function.", s->token);
				panic(msg);
			}
		}
	}
}

static void parse_header(const char* include_dir, const char* header)
{
	char* fullpath = smake(include_dir);
	char sep = '/';
	spush(fullpath, sep);
	sappend(fullpath, header);

	size_t sz = 0;
	char* in = read_file(fullpath, &sz);
	sfree(fullpath);
	if (!in) return;

	state_clear();
	s->in = in;
	s->end = in + sz;
	sfree(s->file);
	s->file = smake(header);

	while (!state_done()) {
		parse_token();
		if (s->token && slen(s->token) && sfirst(s->token) == '@') {
			if (sequ(s->token, "@enum")) {
				parse_enum();
			} else if (sequ(s->token, "@function")) {
				parse_function();
			} else if (sequ(s->token, "@struct")) {
				parse_struct();
			} else {
				char* msg = sfmake("Found unexpected command %s, expected @enum, @function, or @struct.", s->token);
				panic(msg);
			}
		}
	}

	free(in);
}

// -------------------------------------------------------------------------------------------------
// Auto-generate links

static char* auto_generate_links(char* text)
{
	if (!text) return text;
	char* scan = NULL;

	int found = 0;
	for (int i = 0; i < slen(text); ++i) {
		char ch = text[i];
		if (found) {
			if (ch != '`') {
				spush(scan, ch);
			} else {
				text = linkify(text, scan, 1);
				found = 0;
				sfree(scan);
				scan = NULL;
			}
		} else {
			if (ch == '`') {
				found = 1;
			}
		}
	}
	sfree(scan);
	return text;
}

// -------------------------------------------------------------------------------------------------
// String comparison for qsort

static int cmp_istr(const void* a, const void* b)
{
	const char* sa = *(const char**)a;
	const char* sb = *(const char**)b;
	return sicmp(sa, sb);
}

// -------------------------------------------------------------------------------------------------
// Save API reference links

static void save_api_reference_links(FILE* fp, const char* category, const char** pages, int page_count, const char* type,
	CK_MAP(const char**) related)
{
	if (page_count) {
		fprintf(fp, "### %s\n", type);
		for (int i = 0; i < page_count; ++i) {
			char* page_lower = smake(pages[i]);
			stolower(page_lower);
			sreplace(page_lower, " ", "_");
			fprintf(fp, "- [%s](%s/%s.md)\n", pages[i], category, page_lower);
			sfree(page_lower);
		}
		fprintf(fp, "\n\n");
	}
	const char*** related_ptr = map_get_ptr(related, (uint64_t)sintern(category));
	if (related_ptr && *related_ptr) {
		const char** related_links = *related_ptr;
		if (asize(related_links)) {
			fprintf(fp, "### Related Reading\n");
			for (int i = 0; i < asize(related_links); ++i) {
				fprintf(fp, "%s\n", related_links[i]);
			}
			fprintf(fp, "\n");
		}
	}
}

// -------------------------------------------------------------------------------------------------
// Directory iteration

typedef struct DirIter
{
#ifdef _WIN32
	intptr_t handle;
	struct _finddata_t data;
	int first;
#else
	DIR* dir;
	struct dirent* ent;
#endif
} DirIter;

static DirIter dir_open(const char* path)
{
	DirIter it;
	memset(&it, 0, sizeof(it));
#ifdef _WIN32
	char* pattern = smake(path);
	sappend(pattern, "/*");
	it.handle = _findfirst(pattern, &it.data);
	it.first = 1;
	sfree(pattern);
#else
	it.dir = opendir(path);
#endif
	return it;
}

static const char* dir_next(DirIter* it)
{
#ifdef _WIN32
	if (it->handle == -1) return NULL;
	if (it->first) {
		it->first = 0;
		return it->data.name;
	}
	if (_findnext(it->handle, &it->data) == 0) {
		return it->data.name;
	}
	return NULL;
#else
	if (!it->dir) return NULL;
	it->ent = readdir(it->dir);
	return it->ent ? it->ent->d_name : NULL;
#endif
}

static void dir_close(DirIter* it)
{
#ifdef _WIN32
	if (it->handle != -1) _findclose(it->handle);
#else
	if (it->dir) closedir(it->dir);
#endif
}

// -------------------------------------------------------------------------------------------------
// Main

int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	// Find the cute_framework root directory.
	// We expect to be run from: cute_framework/build/Debug/ (or similar).
	// Or accept a command-line argument for the root.
	char* cf_root = NULL;
	if (argc > 1) {
		cf_root = smake(argv[1]);
	} else {
		// Try to find it relative to current working directory.
		// Check ../.. (e.g. build/Debug/docsparser -> cute_framework)
		if (dir_exists("../../include")) {
			cf_root = smake("../..");
		} else if (dir_exists("../include")) {
			cf_root = smake("..");
		} else if (dir_exists("include")) {
			cf_root = smake(".");
		} else {
			panic("Cannot find cute_framework root. Run from within the build directory or pass the root path as an argument.");
		}
	}

	char* include_dir = sfmake("%s/include", cf_root);
	char* docs_dir = sfmake("%s/docs", cf_root);
	g_relative_path = docs_dir;

	printf("Include dir: %s\n", include_dir);
	printf("Docs dir: %s\n", docs_dir);

	// Parse each header.
	DirIter headers = dir_open(include_dir);
	const char* file;
	while ((file = dir_next(&headers))) {
		if (spext_equ(file, ".h")) {
			parse_header(include_dir, file);
		}
	}
	dir_close(&headers);

	// Auto-generate links.
	for (int i = 0; i < asize(s->docs); ++i) {
		Doc* doc = &s->docs[i];
		doc->brief = auto_generate_links(doc->brief);
		for (int j = 0; j < asize(doc->param_briefs); ++j) {
			doc->param_briefs[j] = auto_generate_links(doc->param_briefs[j]);
		}
		for (int j = 0; j < asize(doc->enum_entry_briefs); ++j) {
			doc->enum_entry_briefs[j] = auto_generate_links(doc->enum_entry_briefs[j]);
		}
		for (int j = 0; j < asize(doc->member_briefs); ++j) {
			doc->member_briefs[j] = auto_generate_links(doc->member_briefs[j]);
		}
		doc->return_value = auto_generate_links(doc->return_value);
		doc->example_brief = auto_generate_links(doc->example_brief);
		doc->example = auto_generate_links(doc->example);
		doc->remarks = auto_generate_links(doc->remarks);
		for (int j = 0; j < asize(doc->related);) {
			const char* rel = doc->related[j];
			if (sequ(rel, doc->title)) {
				sfree(doc->related[j]);
				adel(doc->related, j);
				continue;
			}
			char* linked = linkify(smake(rel), rel, 0);
			sappend(linked, "  ");
			sfree(doc->related[j]);
			doc->related[j] = linked;
			++j;
		}
	}

	// Save each doc file.
	for (int i = 0; i < asize(s->docs); ++i) {
		Doc* doc = &s->docs[i];
		{
			// Create category directory.
			char* dir = sppop(doc->path);
			dp_mkdir(dir);
			sfree(dir);
		}
		FILE* fp = fopen(doc->path, "wb");
		if (!fp) {
			fprintf(stderr, "Failed to open: '%s'\n", doc->path);
			exit(1);
		}
		const char* category = sintern(doc->web_category);
		if (!map_has(s->category_index_lists, (uint64_t)category)) {
			const char** empty = NULL;
			map_set(s->category_index_lists, (uint64_t)category, empty);
		}
		const char*** arr_ptr = map_get_ptr(s->category_index_lists, (uint64_t)category);
		apush(*arr_ptr, sintern(doc->title));

		if (doc->type == DOC_ENUM) {
			emit_title(fp, doc);
			emit_brief(fp, doc);
			emit_enum_entries(fp, doc);
			emit_example(fp, doc);
			emit_remarks(fp, doc);
			emit_related(fp, doc);
		} else if (doc->type == DOC_FUNCTION) {
			emit_title(fp, doc);
			emit_brief(fp, doc);
			emit_signature(fp, doc);
			emit_params(fp, doc);
			emit_return(fp, doc);
			emit_example(fp, doc);
			emit_remarks(fp, doc);
			emit_related(fp, doc);
		} else if (doc->type == DOC_STRUCT) {
			emit_title(fp, doc);
			emit_brief(fp, doc);
			emit_members(fp, doc);
			emit_member_function_links(fp, doc);
			emit_example(fp, doc);
			emit_remarks(fp, doc);
			emit_related(fp, doc);
		}
		printf("Wrote %s\n", doc->path);
		fclose(fp);
	}

	// Generate api_reference.md.
	{
		char* ref_path = sfmake("%s/api_reference.md", docs_dir);
		FILE* fp = fopen(ref_path, "wb");
		fprintf(fp, "[//]: # (This file is automatically generated by Cute Framework's docs parser.)\n");
		fprintf(fp, "[//]: # (Do not edit this file by hand!)\n");
		fprintf(fp, "[//]: # (See: https://github.com/RandyGaul/cute_framework/blob/master/tools/docs_parser.c)\n");
		fprintf(fp, "# API Reference\n\n");
		fprintf(fp, "This is a list of all functions in Cute Framework organized by categories. This is great for users that want to see all the available functionality laid out plainly.\n\n\n");

		CK_MAP(const char**) related = NULL;

		// Build related reading links.
		#define ADD_RELATED(name, ...) do { \
			const char* _links[] = { __VA_ARGS__ }; \
			const char** _arr = NULL; \
			for (int _i = 0; _i < (int)(sizeof(_links)/sizeof(_links[0])); ++_i) apush(_arr, _links[_i]); \
			map_set(related, (uint64_t)sintern(name), _arr); \
		} while (0)

		ADD_RELATED("allocator", "- [Allocator](topics/allocator.md)");
		ADD_RELATED("app", "- [Application Window](topics/application_window.md)", "- [Game Loop and Time](topics/game_loop_and_time.md)");
		ADD_RELATED("array", "- [Data Structures](topics/data_structures.md)");
		ADD_RELATED("atomics", "- [Atomics](topics/atomics.md)");
		ADD_RELATED("collision", "- [Collision](topics/collision.md)");
		ADD_RELATED("coroutine", "- [Coroutines](topics/coroutines.md)");
		ADD_RELATED("draw", "- [Drawing](topics/drawing.md)");
		ADD_RELATED("ecs", "- [Entity Component System](topics/entity_component_system.md)");
		ADD_RELATED("file", "- [Virtual File System](topics/virtual_file_system.md)");
		ADD_RELATED("graphics", "- [Low Level Graphics](topics/low_level_graphics.md)");
		ADD_RELATED("haptic", "- [Input](topics/input.md)");
		ADD_RELATED("hash", "- [Data Structures](topics/data_structures.md)");
		ADD_RELATED("input", "- [Input](topics/input.md)");
		ADD_RELATED("list", "- [Data Structures](topics/data_structures.md)");
		ADD_RELATED("math", "- [Collision](topics/collision.md)");
		ADD_RELATED("multithreading", "- [Atomics](topics/atomics.md)", "- [Multithreading](topics/multithreading.md)");
		ADD_RELATED("net", "- [Networking](topics/networking.md)");
		ADD_RELATED("path", "- [Virtual File System](topics/virtual_file_system.md)");
		ADD_RELATED("pathfinding", "- [Path Finding](topics/pathfinding.md)");
		ADD_RELATED("random", "- [Random Numbers](topics/random_numbers.md)");
		ADD_RELATED("sprite", "- [Drawing](topics/drawing.md)");
		ADD_RELATED("string", "- [Strings](topics/strings.md)");
		ADD_RELATED("time", "- [Game Loop and Time](topics/game_loop_and_time.md)");
		ADD_RELATED("text", "- [Drawing](topics/drawing.md)", "- [Input](topics/input.md)");
		ADD_RELATED("web", "- [Networking](topics/networking.md)");
		#undef ADD_RELATED

		// Sort categories by name.
		map_ssort(s->categories, 1);

		for (int i = 0; i < map_size(s->categories); ++i) {
			const char* category = (const char*)map_key(s->categories, i);
			const char*** list_ptr = map_get_ptr(s->category_index_lists, (uint64_t)category);
			const char** index_list = list_ptr ? *list_ptr : NULL;
			fprintf(fp, "## %s\n\n", category);

			// Functions
			{
				const char** functions = NULL;
				for (int j = 0; j < asize(index_list); ++j) {
					const char* title = index_list[j];
					if (s->docs[get_doc_index(title)].type == DOC_FUNCTION) {
						apush(functions, title);
					}
				}
				qsort(functions, (size_t)asize(functions), sizeof(const char*), cmp_istr);
				save_api_reference_links(fp, category, functions, asize(functions), "functions", related);
				afree(functions);
			}

			// Structs
			{
				const char** structs = NULL;
				for (int j = 0; j < asize(index_list); ++j) {
					const char* title = index_list[j];
					if (s->docs[get_doc_index(title)].type == DOC_STRUCT) {
						apush(structs, title);
					}
				}
				qsort(structs, (size_t)asize(structs), sizeof(const char*), cmp_istr);
				save_api_reference_links(fp, category, structs, asize(structs), "structs", related);
				afree(structs);
			}

			// Enums
			{
				const char** enums = NULL;
				for (int j = 0; j < asize(index_list); ++j) {
					const char* title = index_list[j];
					if (s->docs[get_doc_index(title)].type == DOC_ENUM) {
						apush(enums, title);
					}
				}
				qsort(enums, (size_t)asize(enums), sizeof(const char*), cmp_istr);
				save_api_reference_links(fp, category, enums, asize(enums), "enums", related);
				afree(enums);
			}
		}
		fclose(fp);
		sfree(ref_path);
		map_free(related);
	}

	// Delete old document files that are no longer used.
	for (int i = 0; i < map_size(s->categories); ++i) {
		const char* category = (const char*)map_key(s->categories, i);
		char* dir_path = sfmake("%s/%s", docs_dir, category);
		DirIter dir = dir_open(dir_path);
		const char* f;
		while ((f = dir_next(&dir))) {
			if (siequ(f, "enums.md") || siequ(f, "functions.md") || siequ(f, "structs.md")) {
				continue;
			}
			if (f[0] == '.') continue;
			if (!has_doc(f)) {
				char* filepath = sfmake("%s/%s", dir_path, f);
				remove(filepath);
				sfree(filepath);
			}
		}
		dir_close(&dir);
		sfree(dir_path);
	}

	printf("docsparser: Success!\n");

	sfree(include_dir);
	sfree(cf_root);
	return 0;
}
