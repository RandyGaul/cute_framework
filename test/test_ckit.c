/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef _CRT_SECURE_NO_WARNINGS
#	define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE
#	define _CRT_NONSTDC_NO_DEPRECATE
#endif

// CKIT_IMPLEMENTATION is provided by cute.lib
#include <cute/ckit.h>

#include <pico/pico_unit.h>
#include <stdio.h>

//--------------------------------------------------------------------------------------------------
// Arrays.

TEST_CASE(test_ckit_array_push_pop)
{
	int* a = NULL;
	for (int i = 0; i < 10; ++i) apush(a, i);
	REQUIRE(asize(a) == 10);
	REQUIRE(acount(a) == 10);
	for (int i = 0; i < 10; ++i) REQUIRE(a[i] == i);
	REQUIRE(alast(a) == 9);
	REQUIRE(apop(a) == 9);
	REQUIRE(asize(a) == 9);
	afree(a);
	REQUIRE(a == NULL);
	return true;
}

TEST_CASE(test_ckit_array_fit_and_cap)
{
	int* a = NULL;
	afit(a, 100);
	REQUIRE(acap(a) >= 100);
	REQUIRE(asize(a) == 0);
	for (int i = 0; i < 100; ++i) apush(a, i);
	REQUIRE(asize(a) == 100);
	afree(a);
	return true;
}

TEST_CASE(test_ckit_array_clear)
{
	int* a = NULL;
	for (int i = 0; i < 5; ++i) apush(a, i);
	REQUIRE(asize(a) == 5);
	aclear(a);
	REQUIRE(asize(a) == 0);
	REQUIRE(acap(a) >= 5);
	afree(a);
	return true;
}

TEST_CASE(test_ckit_array_del)
{
	int* a = NULL;
	apush(a, 10);
	apush(a, 20);
	apush(a, 30);
	adel(a, 0);
	REQUIRE(asize(a) == 2);
	REQUIRE(a[0] == 30);
	REQUIRE(a[1] == 20);
	afree(a);
	return true;
}

TEST_CASE(test_ckit_array_end)
{
	int* a = NULL;
	apush(a, 1);
	apush(a, 2);
	apush(a, 3);
	int* e = aend(a);
	REQUIRE(e == a + 3);
	afree(a);
	return true;
}

TEST_CASE(test_ckit_array_large)
{
	int* a = NULL;
	for (int i = 0; i < 10000; ++i) apush(a, i);
	REQUIRE(asize(a) == 10000);
	for (int i = 0; i < 10000; ++i) REQUIRE(a[i] == i);
	afree(a);
	return true;
}

TEST_CASE(test_ckit_array_structs)
{
	typedef struct { int x; int y; } Pair;
	Pair* a = NULL;
	apush(a, ((Pair){1, 2}));
	apush(a, ((Pair){3, 4}));
	apush(a, ((Pair){5, 6}));
	REQUIRE(a[0].x == 1 && a[0].y == 2);
	REQUIRE(a[1].x == 3 && a[1].y == 4);
	REQUIRE(a[2].x == 5 && a[2].y == 6);
	Pair p = apop(a);
	REQUIRE(p.x == 5 && p.y == 6);
	afree(a);
	return true;
}

//--------------------------------------------------------------------------------------------------
// Strings - basic.

TEST_CASE(test_ckit_string_set_append)
{
	char* s = NULL;
	sset(s, "Hello ");
	sappend(s, "world!");
	REQUIRE(sequ(s, "Hello world!"));
	REQUIRE(slen(s) == 12);
	REQUIRE(scount(s) == 13);
	sfree(s);
	REQUIRE(s == NULL);
	return true;
}

TEST_CASE(test_ckit_string_push_pop)
{
	char* s = NULL;
	spush(s, 'a');
	spush(s, 'b');
	spush(s, 'c');
	REQUIRE(slen(s) == 3);
	REQUIRE(sequ(s, "abc"));
	REQUIRE(sfirst(s) == 'a');
	REQUIRE(slast(s) == 'c');
	spop(s);
	REQUIRE(slen(s) == 2);
	REQUIRE(sequ(s, "ab"));
	spopn(s, 2);
	REQUIRE(slen(s) == 0);
	REQUIRE(sempty(s));
	sfree(s);
	return true;
}

TEST_CASE(test_ckit_string_dup_make)
{
	char* orig = NULL;
	sset(orig, "duplicate me");
	char* copy = sdup(orig);
	REQUIRE(sequ(orig, copy));
	REQUIRE(orig != copy);
	char* made = smake("from literal");
	REQUIRE(sequ(made, "from literal"));
	sfree(orig);
	sfree(copy);
	sfree(made);
	return true;
}

TEST_CASE(test_ckit_string_clear)
{
	char* s = NULL;
	sset(s, "hello");
	sclear(s);
	REQUIRE(slen(s) == 0);
	REQUIRE(sempty(s));
	sappend(s, "world");
	REQUIRE(sequ(s, "world"));
	sfree(s);
	return true;
}

TEST_CASE(test_ckit_string_fit)
{
	char* s = NULL;
	sfit(s, 200);
	REQUIRE(scap(s) >= 200);
	REQUIRE(slen(s) == 0);
	sfree(s);
	return true;
}

//--------------------------------------------------------------------------------------------------
// Strings - formatting.

TEST_CASE(test_ckit_string_fmt)
{
	char* s = NULL;
	sfmt(s, "x=%d y=%d", 10, 20);
	REQUIRE(sequ(s, "x=10 y=20"));
	sfmt(s, "%s", "overwrite");
	REQUIRE(sequ(s, "overwrite"));
	sfree(s);
	return true;
}

TEST_CASE(test_ckit_string_fmt_append)
{
	char* s = NULL;
	sfmt(s, "%d,", 1);
	sfmt_append(s, "%d,", 2);
	sfmt_append(s, "%d", 3);
	REQUIRE(sequ(s, "1,2,3"));
	sfree(s);

	// Test fmt_append on NULL.
	char* s2 = NULL;
	sfmt_append(s2, "%d,%d", 42, 99);
	REQUIRE(sequ(s2, "42,99"));
	sfree(s2);
	return true;
}

TEST_CASE(test_ckit_string_fmt_long)
{
	char* s = NULL;
	const char* long_str = "some string longer than the default string length forcing an sfit call to test its code path.";
	sfmt(s, "%s", long_str);
	REQUIRE(sequ(s, long_str));
	sfree(s);
	return true;
}

//--------------------------------------------------------------------------------------------------
// Strings - compare.

TEST_CASE(test_ckit_string_compare)
{
	REQUIRE(sequ("abc", "abc"));
	REQUIRE(!sequ("abc", "def"));
	REQUIRE(sequ(NULL, NULL));
	REQUIRE(!sequ("abc", NULL));
	REQUIRE(!sequ(NULL, "abc"));

	REQUIRE(siequ("ABC", "abc"));
	REQUIRE(siequ("Hello", "hello"));
	REQUIRE(!siequ("abc", "def"));

	REQUIRE(scmp("abc", "abc") == 0);
	REQUIRE(scmp("abc", "abd") < 0);
	REQUIRE(sicmp("ABC", "abc") == 0);
	return true;
}

//--------------------------------------------------------------------------------------------------
// Strings - prefix, suffix, contains, find.

TEST_CASE(test_ckit_string_prefix_suffix)
{
	char* s = NULL;
	sset(s, "hello.txt");
	REQUIRE(sprefix(s, "hello"));
	REQUIRE(sprefix(s, "h"));
	REQUIRE(!sprefix(s, "world"));
	REQUIRE(ssuffix(s, ".txt"));
	REQUIRE(ssuffix(s, "t"));
	REQUIRE(!ssuffix(s, ".png"));
	REQUIRE(scontains(s, "llo.tx"));
	REQUIRE(!scontains(s, "xyz"));
	sfree(s);
	return true;
}

TEST_CASE(test_ckit_string_index_of)
{
	char* s = NULL;
	sset(s, "012330");
	REQUIRE(sfirst_index_of(s, '0') == 0);
	REQUIRE(sfirst_index_of(s, '3') == 3);
	REQUIRE(slast_index_of(s, '3') == 4);
	REQUIRE(slast_index_of(s, '0') == 5);
	REQUIRE(sfirst_index_of(s, 'z') == -1);
	REQUIRE(slast_index_of(s, 'z') == -1);
	sfree(s);

	REQUIRE(sfirst_index_of(NULL, 'a') == -1);
	REQUIRE(slast_index_of(NULL, 'a') == -1);
	return true;
}

TEST_CASE(test_ckit_string_find)
{
	char* s = NULL;
	sset(s, "hello world");
	REQUIRE(sfind(s, "world") != NULL);
	REQUIRE(sfind(s, "xyz") == NULL);
	sfree(s);
	return true;
}

//--------------------------------------------------------------------------------------------------
// Strings - transform.

TEST_CASE(test_ckit_string_case)
{
	char* s = NULL;
	sset(s, "Hello World");
	stoupper(s);
	REQUIRE(sequ(s, "HELLO WORLD"));
	stolower(s);
	REQUIRE(sequ(s, "hello world"));
	sfree(s);
	return true;
}

TEST_CASE(test_ckit_string_trim)
{
	char* a = NULL;
	sset(a, "   spac es  ");
	strim(a);
	REQUIRE(sequ(a, "spac es"));
	REQUIRE(slen(a) == 7);

	char* b = NULL;
	sset(b, "   spac es  ");
	sltrim(b);
	REQUIRE(sequ(b, "spac es  "));

	char* c = NULL;
	sset(c, "   spac es  ");
	srtrim(c);
	REQUIRE(sequ(c, "   spac es"));

	// Trim string with no whitespace.
	char* d = NULL;
	sset(d, "nospace");
	strim(d);
	REQUIRE(sequ(d, "nospace"));

	sfree(a);
	sfree(b);
	sfree(c);
	sfree(d);
	return true;
}

TEST_CASE(test_ckit_string_pad)
{
	char* s = NULL;
	sset(s, "pad");
	srpad(s, 'x', 3);
	REQUIRE(sequ(s, "padxxx"));
	slpad(s, 'y', 2);
	REQUIRE(sequ(s, "yypadxxx"));
	sfree(s);
	return true;
}

TEST_CASE(test_ckit_string_replace)
{
	char* s = NULL;
	sset(s, "hi hi hi");
	sreplace(s, "hi", "oof");
	REQUIRE(sequ(s, "oof oof oof"));
	sreplace(s, "oof", "hi");
	REQUIRE(sequ(s, "hi hi hi"));

	// Replace with shorter.
	sset(s, "aabbcc");
	sreplace(s, "bb", "x");
	REQUIRE(sequ(s, "aaxcc"));

	// Replace with longer.
	sset(s, "ab");
	sreplace(s, "ab", "abcdef");
	REQUIRE(sequ(s, "abcdef"));

	// No match.
	sset(s, "hello");
	sreplace(s, "xyz", "abc");
	REQUIRE(sequ(s, "hello"));

	sfree(s);
	return true;
}

TEST_CASE(test_ckit_string_dedup)
{
	char* s = NULL;
	sset(s, "aaa//bb///cc");
	sdedup(s, '/');
	REQUIRE(sequ(s, "aaa/bb/cc"));
	sfree(s);
	return true;
}

TEST_CASE(test_ckit_string_erase)
{
	char* s = NULL;
	sset(s, "erase me");
	serase(s, 0, 2);
	REQUIRE(sequ(s, "ase me"));

	sset(s, "erase.me.please");
	serase(s, 10, 100);
	REQUIRE(sequ(s, "erase.me.p"));

	// Erase past end.
	sset(s, "abc");
	serase(s, 30, 10);
	REQUIRE(sequ(s, "abc"));

	// Negative index.
	sset(s, "erase.me.please");
	serase(s, -5, 10);
	REQUIRE(sequ(s, ".me.please"));

	sfree(s);
	return true;
}

TEST_CASE(test_ckit_string_append_range)
{
	char* s = NULL;
	sset(s, "hello");
	const char* world = " world!";
	sappend_range(s, world, world + 6);
	REQUIRE(sequ(s, "hello world"));
	sfree(s);
	return true;
}

//--------------------------------------------------------------------------------------------------
// Strings - split.

TEST_CASE(test_ckit_string_split_once)
{
	char* s = NULL;
	sset(s, "split.here");
	char* left = ssplit_once(s, '.');
	REQUIRE(sequ(left, "split"));
	REQUIRE(sequ(s, "here"));
	sfree(left);
	sfree(s);
	return true;
}

TEST_CASE(test_ckit_string_split)
{
	char** parts = ssplit("a.b.c", '.');
	REQUIRE(asize(parts) == 3);
	REQUIRE(sequ(parts[0], "a"));
	REQUIRE(sequ(parts[1], "b"));
	REQUIRE(sequ(parts[2], "c"));
	for (int i = 0; i < asize(parts); ++i) sfree(parts[i]);
	afree(parts);

	// Split with no delimiter found.
	parts = ssplit("nosplit", '.');
	REQUIRE(asize(parts) == 1);
	REQUIRE(sequ(parts[0], "nosplit"));
	for (int i = 0; i < asize(parts); ++i) sfree(parts[i]);
	afree(parts);

	// Split many segments.
	const char* expected[] = { "split", "here", "in", "a", "loop" };
	parts = ssplit("split.here.in.a.loop", '.');
	REQUIRE(asize(parts) == 5);
	for (int i = 0; i < 5; ++i) {
		REQUIRE(sequ(parts[i], expected[i]));
		sfree(parts[i]);
	}
	afree(parts);
	return true;
}

//--------------------------------------------------------------------------------------------------
// Strings - conversions.

TEST_CASE(test_ckit_string_conversions)
{
	REQUIRE(stoint("42") == 42);
	REQUIRE(stoint("-7") == -7);
	REQUIRE(stouint("100") == 100);
	REQUIRE(stofloat("3.14") > 3.13f && stofloat("3.14") < 3.15f);
	REQUIRE(stodouble("2.718") > 2.717 && stodouble("2.718") < 2.719);
	REQUIRE(stobool("true"));
	REQUIRE(!stobool("false"));

	// Hex.
	REQUIRE(stohex("0xbaadf00d") == 0xbaadf00d);
	REQUIRE(stohex("#baadf0") == 0xbaadf0FF);
	REQUIRE(stohex("baadf0") == 0xbaadf0FF);
	return true;
}

TEST_CASE(test_ckit_string_from_primitives)
{
	char* s = NULL;
	sint(s, 42);
	REQUIRE(sequ(s, "42"));
	sfree(s);

	sfloat(s, 3.0);
	REQUIRE(sprefix(s, "3.0"));
	sfree(s);

	sbool(s, 1);
	REQUIRE(sequ(s, "true"));
	sfree(s);

	sbool(s, 0);
	REQUIRE(sequ(s, "false"));
	sfree(s);
	return true;
}

//--------------------------------------------------------------------------------------------------
// Strings - UTF-8.

TEST_CASE(test_ckit_string_utf8)
{
	// ASCII.
	char* s = NULL;
	sappend_UTF8(s, 'A');
	REQUIRE(slen(s) == 1);
	REQUIRE(s[0] == 'A');

	// 2-byte: Latin Small Letter E With Acute (U+00E9).
	sappend_UTF8(s, 0x00E9);
	REQUIRE(slen(s) == 3);

	// Decode back.
	int cp;
	const char* p = cf_decode_UTF8(s, &cp);
	REQUIRE(cp == 'A');
	p = cf_decode_UTF8(p, &cp);
	REQUIRE(cp == 0x00E9);
	(void)p;

	sfree(s);

	// 3-byte: Euro Sign (U+20AC).
	s = NULL;
	sappend_UTF8(s, 0x20AC);
	REQUIRE(slen(s) == 3);
	cf_decode_UTF8(s, &cp);
	REQUIRE(cp == 0x20AC);
	sfree(s);

	// 4-byte: Grinning Face (U+1F600).
	s = NULL;
	sappend_UTF8(s, 0x1F600);
	REQUIRE(slen(s) == 4);
	cf_decode_UTF8(s, &cp);
	REQUIRE(cp == 0x1F600);
	sfree(s);

	// Invalid codepoint -> replacement character.
	s = NULL;
	sappend_UTF8(s, 0x200000);
	cf_decode_UTF8(s, &cp);
	REQUIRE(cp == 0xFFFD);
	sfree(s);

	return true;
}

//--------------------------------------------------------------------------------------------------
// Strings - hash.

TEST_CASE(test_ckit_string_hash)
{
	char* a = NULL;
	sset(a, "hello");
	char* b = NULL;
	sset(b, "hello");
	char* c = NULL;
	sset(c, "world");
	REQUIRE(shash(a) == shash(b));
	REQUIRE(shash(a) != shash(c));
	sfree(a);
	sfree(b);
	sfree(c);
	return true;
}

//--------------------------------------------------------------------------------------------------
// Path utilities.

TEST_CASE(test_ckit_path_fname)
{
	char* s;

	s = spfname("../root/file.txt");
	REQUIRE(sequ(s, "file.txt"));
	sfree(s);

	s = spfname("./file.txt");
	REQUIRE(sequ(s, "file.txt"));
	sfree(s);

	s = spfname("file.txt");
	REQUIRE(sequ(s, "file.txt"));
	sfree(s);

	s = spfname("file");
	REQUIRE(sequ(s, "file"));
	sfree(s);

	s = spfname("/.txt");
	REQUIRE(sequ(s, ".txt"));
	sfree(s);

	s = spfname("/root/");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	return true;
}

TEST_CASE(test_ckit_path_fname_no_ext)
{
	char* s;

	s = spfname_no_ext("../root/file.txt");
	REQUIRE(sequ(s, "file"));
	sfree(s);

	s = spfname_no_ext("file.txt");
	REQUIRE(sequ(s, "file"));
	sfree(s);

	s = spfname_no_ext("/file");
	REQUIRE(sequ(s, "file"));
	sfree(s);

	s = spfname_no_ext("/root/");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	s = spfname_no_ext("/.txt");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	return true;
}

TEST_CASE(test_ckit_path_ext)
{
	char* s;

	s = spext("../root/file.txt");
	REQUIRE(sequ(s, ".txt"));
	sfree(s);

	s = spext("../root/file");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	REQUIRE(spext_equ("file.txt", ".txt"));
	REQUIRE(!spext_equ("file.txt", ".png"));
	REQUIRE(!spext_equ("file", ".txt"));

	return true;
}

TEST_CASE(test_ckit_path_pop)
{
	char* s;

	s = sppop("../root/file.txt");
	REQUIRE(sequ(s, "../root"));
	sfree(s);

	s = sppop("../root");
	REQUIRE(sequ(s, ".."));
	sfree(s);

	s = sppop("..");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	s = sppop("/folder/file.txt");
	REQUIRE(sequ(s, "/folder"));
	sfree(s);

	s = sppop("/folder");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	// sppopn.
	s = sppopn("/a/b/c/d", 2);
	REQUIRE(sequ(s, "/a/b"));
	sfree(s);

	s = sppopn("/a/b", 10);
	REQUIRE(sequ(s, "/"));
	sfree(s);

	return true;
}

TEST_CASE(test_ckit_path_compact)
{
	char* s;

	s = spcompact("/real_long_file_name.txt", 15);
	REQUIRE(sequ(s, "/real_long_f..."));
	sfree(s);

	s = spcompact("/folder/to/file.txt", 17);
	REQUIRE(sequ(s, "/fold.../file.txt"));
	sfree(s);

	// Short path that fits.
	s = spcompact("/a.txt", 20);
	REQUIRE(sequ(s, "/a.txt"));
	sfree(s);

	// n too small.
	s = spcompact("/a.txt", 5);
	REQUIRE(s == NULL);

	return true;
}

TEST_CASE(test_ckit_path_dir_of)
{
	char* s;

	s = spdir_of("/example/a/b/c/file.txt");
	REQUIRE(sequ(s, "/c"));
	sfree(s);

	s = spdir_of("/example/file.txt");
	REQUIRE(sequ(s, "/example"));
	sfree(s);

	s = spdir_of("/file.txt");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	s = spdir_of("../file.txt");
	REQUIRE(sequ(s, ".."));
	sfree(s);

	s = spdir_of("./file.txt");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	REQUIRE(spdir_of("..") == NULL);
	REQUIRE(spdir_of("../") == NULL);
	REQUIRE(spdir_of("./") == NULL);
	REQUIRE(spdir_of(".") == NULL);
	REQUIRE(spdir_of("/") == NULL);

	return true;
}

TEST_CASE(test_ckit_path_top_of)
{
	char* s;

	s = sptop_of("/example/a/b/c/file.txt");
	REQUIRE(sequ(s, "/example"));
	sfree(s);

	s = sptop_of("/example/file.txt");
	REQUIRE(sequ(s, "/example"));
	sfree(s);

	s = sptop_of("/file.txt");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	s = sptop_of("/a");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	s = sptop_of("/");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	REQUIRE(sptop_of(".") == NULL);
	REQUIRE(sptop_of("..") == NULL);

	return true;
}

TEST_CASE(test_ckit_path_norm)
{
	char* s;

	s = spnorm("C:\\Users\\foo");
	REQUIRE(sequ(s, "C:/Users/foo"));
	sfree(s);

	s = spnorm("/first.last/.hidden");
	REQUIRE(sequ(s, "/first.last/.hidden"));
	sfree(s);

	s = spnorm("C:\\Users\\..\\foo");
	REQUIRE(sequ(s, "C:/foo"));
	sfree(s);

	// Unix relative path gets leading /.
	s = spnorm("a/b/c");
	REQUIRE(sequ(s, "/a/b/c"));
	sfree(s);

	return true;
}

//--------------------------------------------------------------------------------------------------
// Map - typed.

TEST_CASE(test_ckit_map_int)
{
	CK_MAP(int) m = NULL;
	map_set(m, 1, 100);
	map_set(m, 2, 200);
	map_set(m, 3, 300);
	REQUIRE(map_get(m, 1) == 100);
	REQUIRE(map_get(m, 2) == 200);
	REQUIRE(map_get(m, 3) == 300);
	REQUIRE(map_get(m, 99) == 0);
	REQUIRE(map_has(m, 1));
	REQUIRE(map_has(m, 2));
	REQUIRE(map_has(m, 3));
	REQUIRE(!map_has(m, 99));
	REQUIRE(map_size(m) == 3);
	map_free(m);
	REQUIRE(m == NULL);
	return true;
}

TEST_CASE(test_ckit_map_get_ptr)
{
	CK_MAP(int) m = NULL;
	map_set(m, 10, 42);
	int* ptr = map_get_ptr(m, 10);
	REQUIRE(ptr != NULL);
	REQUIRE(*ptr == 42);
	REQUIRE(map_get_ptr(m, 99) == NULL);
	map_free(m);
	return true;
}

TEST_CASE(test_ckit_map_update)
{
	CK_MAP(int) m = NULL;
	map_set(m, 1, 100);
	REQUIRE(map_get(m, 1) == 100);
	map_set(m, 1, 999);
	REQUIRE(map_get(m, 1) == 999);
	REQUIRE(map_size(m) == 1);
	map_free(m);
	return true;
}

TEST_CASE(test_ckit_map_del)
{
	CK_MAP(int) m = NULL;
	map_set(m, 1, 100);
	map_set(m, 2, 200);
	map_set(m, 3, 300);
	REQUIRE(map_del(m, 2));
	REQUIRE(!map_has(m, 2));
	REQUIRE(map_size(m) == 2);
	REQUIRE(map_get(m, 1) == 100);
	REQUIRE(map_get(m, 3) == 300);

	// Delete non-existent.
	REQUIRE(!map_del(m, 99));
	map_free(m);
	return true;
}

TEST_CASE(test_ckit_map_clear)
{
	CK_MAP(int) m = NULL;
	for (int i = 0; i < 10; ++i) map_set(m, i, i * 10);
	REQUIRE(map_size(m) == 10);
	map_clear(m);
	REQUIRE(map_size(m) == 0);
	REQUIRE(!map_has(m, 0));
	map_free(m);
	return true;
}

TEST_CASE(test_ckit_map_iteration)
{
	CK_MAP(int) m = NULL;
	map_set(m, 10, 100);
	map_set(m, 20, 200);
	map_set(m, 30, 300);
	int sum_keys = 0;
	int sum_vals = 0;
	for (int i = 0; i < map_size(m); ++i) {
		sum_keys += (int)map_keys(m)[i];
		sum_vals += map_items(m)[i];
	}
	REQUIRE(sum_keys == 60);
	REQUIRE(sum_vals == 600);
	map_free(m);
	return true;
}

TEST_CASE(test_ckit_map_large)
{
	CK_MAP(int) m = NULL;
	for (int i = 0; i < 1000; ++i) {
		map_set(m, i, i * 3);
	}
	REQUIRE(map_size(m) == 1000);
	for (int i = 0; i < 1000; ++i) {
		REQUIRE(map_get(m, i) == i * 3);
	}
	// Delete every other.
	for (int i = 0; i < 1000; i += 2) {
		map_del(m, i);
	}
	REQUIRE(map_size(m) == 500);
	for (int i = 1; i < 1000; i += 2) {
		REQUIRE(map_has(m, i));
		REQUIRE(map_get(m, i) == i * 3);
	}
	map_free(m);
	return true;
}

typedef struct
{
	float x;
	float y;
} TestVec2;

TEST_CASE(test_ckit_map_struct_values)
{
	CK_MAP(TestVec2) m = NULL;
	map_set(m, 0, ((TestVec2){ 1.0f, 2.0f }));
	map_set(m, 1, ((TestVec2){ 3.0f, 4.0f }));
	map_set(m, 2, ((TestVec2){ -1.0f, -2.0f }));
	TestVec2 v = map_get(m, 0);
	REQUIRE(v.x == 1.0f && v.y == 2.0f);
	v = map_get(m, 1);
	REQUIRE(v.x == 3.0f && v.y == 4.0f);
	v = map_get(m, 2);
	REQUIRE(v.x == -1.0f && v.y == -2.0f);

	// Not found -> zero.
	v = map_get(m, 99);
	REQUIRE(v.x == 0.0f && v.y == 0.0f);

	// Get ptr.
	TestVec2* p = map_get_ptr(m, 1);
	REQUIRE(p && p->x == 3.0f && p->y == 4.0f);
	REQUIRE(map_get_ptr(m, 99) == NULL);

	map_free(m);
	return true;
}

//--------------------------------------------------------------------------------------------------
// Map - uint64_t convenience.

TEST_CASE(test_ckit_map_u64_convenience)
{
	CK_MAP(uint64_t) m = NULL;
	map_add(m, 10, 42);
	map_add(m, 20, 84);
	map_add(m, 30, 126);
	REQUIRE(map_find(m, 10) == 42);
	REQUIRE(map_find(m, 20) == 84);
	REQUIRE(map_find(m, 30) == 126);
	REQUIRE(map_find(m, 99) == 0);
	map_free(m);
	return true;
}

//--------------------------------------------------------------------------------------------------
// Map - swap and sort.

TEST_CASE(test_ckit_map_swap)
{
	CK_MAP(int) m = NULL;
	map_set(m, 1, 100);
	map_set(m, 2, 200);
	map_set(m, 3, 300);
	// Swap first and last.
	map_swap(m, 0, 2);
	// Values should be swapped in dense array but lookups still work.
	REQUIRE(map_get(m, 1) == 100);
	REQUIRE(map_get(m, 2) == 200);
	REQUIRE(map_get(m, 3) == 300);
	map_free(m);
	return true;
}

TEST_CASE(test_ckit_map_ssort)
{
	CK_MAP(int) m = NULL;
	map_set(m, sintern("eee"), 4);
	map_set(m, sintern("ddd"), 3);
	map_set(m, sintern("bbb"), 1);
	map_set(m, sintern("ccc"), 2);
	map_set(m, sintern("aaa"), 0);
	map_ssort(m, 1);
	// After sort, dense array should be in lexicographic order.
	for (int i = 0; i < map_size(m) - 1; ++i) {
		const char* a = (const char*)map_keys(m)[i];
		const char* b = (const char*)map_keys(m)[i + 1];
		REQUIRE(strcmp(a, b) < 0);
	}
	// Lookups still work.
	REQUIRE(map_get(m, sintern("aaa")) == 0);
	REQUIRE(map_get(m, sintern("eee")) == 4);
	map_free(m);
	sintern_nuke();
	return true;
}

//--------------------------------------------------------------------------------------------------
// String interning.

TEST_CASE(test_ckit_intern_basic)
{
	const char* a = sintern("hello");
	const char* b = sintern("hello");
	const char* c = sintern("world");
	REQUIRE(a == b);
	REQUIRE(a != c);
	REQUIRE(strcmp(a, "hello") == 0);
	REQUIRE(strcmp(c, "world") == 0);
	sintern_nuke();
	return true;
}

TEST_CASE(test_ckit_intern_range)
{
	const char* full = "hello world";
	const char* a = sintern_range(full, full + 5);
	const char* b = sintern("hello");
	REQUIRE(a == b);
	sintern_nuke();
	return true;
}

TEST_CASE(test_ckit_intern_constructed)
{
	// Intern strings built from different sources.
	char buf[64];
	strcpy(buf, "he");
	strcat(buf, "llo");
	const char* a = sintern("hello");
	const char* b = sintern(buf);
	REQUIRE(a == b);
	sintern_nuke();
	return true;
}

TEST_CASE(test_ckit_intern_as_map_key)
{
	CK_MAP(int) m = NULL;
	const char* k1 = sintern("x");
	const char* k2 = sintern("y");
	map_set(m, k1, 10);
	map_set(m, k2, 20);
	REQUIRE(map_get(m, sintern("x")) == 10);
	REQUIRE(map_get(m, sintern("y")) == 20);
	map_free(m);
	sintern_nuke();
	return true;
}

//--------------------------------------------------------------------------------------------------
// String edge cases and bug coverage.

TEST_CASE(test_ckit_string_slast_empty)
{
	// slast on single char.
	char* s = smake("x");
	REQUIRE(slast(s) == 'x');
	sfree(s);

	// slast on multi-char string.
	s = smake("abc");
	REQUIRE(slast(s) == 'c');
	sfree(s);

	// slast on NULL returns '\0'.
	char* null_str = NULL;
	char c = slast(null_str);
	REQUIRE(c == '\0');

	return true;
}

TEST_CASE(test_ckit_string_sfirst_empty)
{
	// sfirst on empty string.
	char* s = NULL;
	sset(s, "");
	char c = sfirst(s);
	REQUIRE(c == '\0');
	sfree(s);

	// sfirst on NULL returns '\0'.
	char* null_str = NULL;
	c = sfirst(null_str);
	REQUIRE(c == '\0');

	// sfirst on normal string.
	s = smake("abc");
	REQUIRE(sfirst(s) == 'a');
	sfree(s);

	return true;
}

TEST_CASE(test_ckit_string_scontains_edge)
{
	// scontains with empty substring matches.
	char* s = smake("hello");
	REQUIRE(scontains(s, ""));
	sfree(s);

	// scontains with matching substring.
	s = smake("hello world");
	REQUIRE(scontains(s, "world"));
	REQUIRE(!scontains(s, "xyz"));
	sfree(s);

	return true;
}

TEST_CASE(test_ckit_string_sappend_null)
{
	// sappend with empty string b.
	char* s = smake("hello");
	sappend(s, "");
	REQUIRE(sequ(s, "hello"));
	sfree(s);

	return true;
}

TEST_CASE(test_ckit_string_split_once_edge_cases)
{
	// Split when delimiter is at the end.
	char* s = NULL;
	sset(s, "abc,");
	char* left = ssplit_once(s, ',');
	// Should return "abc" and s should be "".
	REQUIRE(sequ(left, "abc"));
	REQUIRE(sequ(s, ""));
	sfree(left);
	sfree(s);

	// Split when delimiter is at the start.
	s = NULL;
	sset(s, ",abc");
	left = ssplit_once(s, ',');
	REQUIRE(sequ(left, ""));
	REQUIRE(sequ(s, "abc"));
	sfree(left);
	sfree(s);

	// Split when string is just the delimiter.
	s = NULL;
	sset(s, ",");
	left = ssplit_once(s, ',');
	REQUIRE(sequ(left, ""));
	REQUIRE(sequ(s, ""));
	sfree(left);
	sfree(s);

	// No delimiter found.
	s = NULL;
	sset(s, "nodelim");
	left = ssplit_once(s, ',');
	REQUIRE(left == NULL);
	REQUIRE(sequ(s, "nodelim"));
	sfree(s);

	return true;
}

TEST_CASE(test_ckit_string_sdedup_edge_cases)
{
	// Dedup on empty string.
	char* s = smake("");
	sdedup(s, '/');
	REQUIRE(sequ(s, ""));
	REQUIRE(slen(s) == 0);
	sfree(s);

	// Dedup on single character.
	s = smake("x");
	sdedup(s, 'x');
	REQUIRE(sequ(s, "x"));
	sfree(s);

	// Dedup on all same characters.
	s = smake("////");
	sdedup(s, '/');
	REQUIRE(sequ(s, "/"));
	sfree(s);

	// Dedup with no duplicates.
	s = smake("/a/b/c");
	sdedup(s, '/');
	REQUIRE(sequ(s, "/a/b/c"));
	sfree(s);

	return true;
}

TEST_CASE(test_ckit_string_utf8_truncated)
{
	// Truncated 2-byte sequence (starts with 0xC0-0xDF but only 1 byte).
	// This tests that cf_decode_UTF8 handles malformed input gracefully.
	char truncated2[] = { (char)0xC2, '\0' };
	int cp;
	cf_decode_UTF8(truncated2, &cp);
	// Should return replacement character for invalid sequence.
	REQUIRE(cp == 0xFFFD);

	// Truncated 3-byte sequence.
	char truncated3[] = { (char)0xE2, (char)0x82, '\0' };
	cf_decode_UTF8(truncated3, &cp);
	REQUIRE(cp == 0xFFFD);

	// Truncated 4-byte sequence.
	char truncated4[] = { (char)0xF0, (char)0x9F, (char)0x98, '\0' };
	cf_decode_UTF8(truncated4, &cp);
	REQUIRE(cp == 0xFFFD);

	return true;
}

TEST_CASE(test_ckit_string_stouint_large)
{
	// Test large unsigned values.
	REQUIRE(stouint("0") == 0);
	REQUIRE(stouint("4294967295") == 4294967295ULL);

	// Test int conversion boundaries.
	REQUIRE(stoint("2147483647") == 2147483647);
	REQUIRE(stoint("-2147483648") == -2147483648);

	return true;
}

TEST_CASE(test_ckit_string_trim_all_whitespace)
{
	// Trim string that is all whitespace.
	char* s = smake("   \t\n  ");
	strim(s);
	REQUIRE(sequ(s, ""));
	REQUIRE(slen(s) == 0);
	sfree(s);

	// Left trim all whitespace.
	s = smake("   \t  ");
	sltrim(s);
	REQUIRE(sequ(s, ""));
	sfree(s);

	// Right trim all whitespace.
	s = smake("   \n  ");
	srtrim(s);
	REQUIRE(sequ(s, ""));
	sfree(s);

	return true;
}

TEST_CASE(test_ckit_string_erase_edge_cases)
{
	// Erase with count=0.
	char* s = smake("hello");
	serase(s, 2, 0);
	REQUIRE(sequ(s, "hello"));
	sfree(s);

	// Erase entire string.
	s = smake("hello");
	serase(s, 0, 100);
	REQUIRE(sequ(s, ""));
	sfree(s);

	// Erase with negative index that makes count<=0.
	s = smake("hello");
	serase(s, -10, 5);
	REQUIRE(sequ(s, "hello"));
	sfree(s);

	return true;
}

TEST_CASE(test_ckit_string_replace_edge_cases)
{
	// Replace with same length.
	char* s = smake("aabbcc");
	sreplace(s, "bb", "xx");
	REQUIRE(sequ(s, "aaxxcc"));
	sfree(s);

	// Replace empty string (should do nothing).
	s = smake("hello");
	sreplace(s, "", "x");
	// Behavior: empty match at every position would cause issues.
	// Most implementations skip empty pattern.
	sfree(s);

	// Replace at very beginning.
	s = smake("hello");
	sreplace(s, "he", "XX");
	REQUIRE(sequ(s, "XXllo"));
	sfree(s);

	// Replace at very end.
	s = smake("hello");
	sreplace(s, "lo", "XX");
	REQUIRE(sequ(s, "helXX"));
	sfree(s);

	// Multiple consecutive replacements.
	s = smake("aaa");
	sreplace(s, "a", "bb");
	REQUIRE(sequ(s, "bbbbbb"));
	sfree(s);

	return true;
}

TEST_CASE(test_ckit_string_spush_spop_sequence)
{
	// Build string character by character.
	char* s = NULL;
	spush(s, 'h');
	spush(s, 'e');
	spush(s, 'l');
	spush(s, 'l');
	spush(s, 'o');
	REQUIRE(sequ(s, "hello"));
	REQUIRE(slen(s) == 5);

	// Pop all characters.
	spop(s);
	REQUIRE(sequ(s, "hell"));
	spop(s);
	REQUIRE(sequ(s, "hel"));
	spopn(s, 3);
	REQUIRE(sequ(s, ""));
	REQUIRE(slen(s) == 0);

	// Pop from empty should not crash.
	spop(s);
	REQUIRE(slen(s) == 0);

	sfree(s);
	return true;
}

TEST_CASE(test_ckit_string_prefix_suffix_edge_cases)
{
	// Empty prefix/suffix always matches.
	char* s = smake("hello");
	REQUIRE(sprefix(s, ""));
	REQUIRE(ssuffix(s, ""));
	sfree(s);

	// Prefix/suffix longer than string.
	s = smake("hi");
	REQUIRE(!sprefix(s, "hello"));
	REQUIRE(!ssuffix(s, "world"));
	sfree(s);

	// Exact match.
	s = smake("exact");
	REQUIRE(sprefix(s, "exact"));
	REQUIRE(ssuffix(s, "exact"));
	sfree(s);

	// NULL string.
	REQUIRE(!sprefix(NULL, "test"));
	REQUIRE(!ssuffix(NULL, "test"));

	return true;
}

TEST_CASE(test_ckit_string_append_range_edge_cases)
{
	// Append empty range.
	char* s = smake("hello");
	const char* empty = "";
	sappend_range(s, empty, empty);
	REQUIRE(sequ(s, "hello"));
	sfree(s);

	// Append to empty string.
	s = smake("");
	const char* world = "world";
	sappend_range(s, world, world + 5);
	REQUIRE(sequ(s, "world"));
	sfree(s);

	return true;
}

TEST_CASE(test_ckit_string_pad_edge_cases)
{
	// Pad with count=0.
	char* s = smake("test");
	slpad(s, 'x', 0);
	REQUIRE(sequ(s, "test"));
	srpad(s, 'y', 0);
	REQUIRE(sequ(s, "test"));
	sfree(s);

	// Pad empty string.
	s = smake("");
	slpad(s, 'L', 3);
	REQUIRE(sequ(s, "LLL"));
	sfree(s);

	s = smake("");
	srpad(s, 'R', 3);
	REQUIRE(sequ(s, "RRR"));
	sfree(s);

	return true;
}

TEST_CASE(test_ckit_string_ssplit_edge_cases)
{
	// Split empty string.
	char** parts = ssplit("", '.');
	REQUIRE(asize(parts) == 1);
	REQUIRE(sequ(parts[0], ""));
	for (int i = 0; i < asize(parts); ++i) sfree(parts[i]);
	afree(parts);

	// Split string with only delimiters.
	parts = ssplit("...", '.');
	REQUIRE(asize(parts) == 4);
	for (int i = 0; i < asize(parts); ++i) {
		REQUIRE(sequ(parts[i], ""));
		sfree(parts[i]);
	}
	afree(parts);

	// Split with delimiter at start and end.
	parts = ssplit(".a.b.", '.');
	REQUIRE(asize(parts) == 4);
	REQUIRE(sequ(parts[0], ""));
	REQUIRE(sequ(parts[1], "a"));
	REQUIRE(sequ(parts[2], "b"));
	REQUIRE(sequ(parts[3], ""));
	for (int i = 0; i < asize(parts); ++i) sfree(parts[i]);
	afree(parts);

	return true;
}

TEST_CASE(test_ckit_intern_empty_string)
{
	// Intern empty string.
	const char* a = sintern("");
	const char* b = sintern("");
	REQUIRE(a == b);
	REQUIRE(strcmp(a, "") == 0);
	sintern_nuke();
	return true;
}

TEST_CASE(test_ckit_intern_many_strings)
{
	// Intern many unique strings to test hash collisions.
	const char* ptrs[100];
	char buf[32];
	for (int i = 0; i < 100; ++i) {
		sprintf(buf, "string_%d", i);
		ptrs[i] = sintern(buf);
	}

	// Verify they're all unique.
	for (int i = 0; i < 100; ++i) {
		for (int j = i + 1; j < 100; ++j) {
			REQUIRE(ptrs[i] != ptrs[j]);
		}
	}

	// Verify lookup works.
	for (int i = 0; i < 100; ++i) {
		sprintf(buf, "string_%d", i);
		REQUIRE(sintern(buf) == ptrs[i]);
	}

	sintern_nuke();
	return true;
}

//--------------------------------------------------------------------------------------------------
// FNV-1a hash.

TEST_CASE(test_ckit_fnv1a)
{
	uint64_t h1 = ck_hash_fnv1a("hello", 5);
	uint64_t h2 = ck_hash_fnv1a("hello", 5);
	uint64_t h3 = ck_hash_fnv1a("world", 5);
	REQUIRE(h1 == h2);
	REQUIRE(h1 != h3);
	REQUIRE(h1 != 0);
	return true;
}

//--------------------------------------------------------------------------------------------------
// Suite.

TEST_SUITE(test_ckit)
{
	// Arrays.
	RUN_TEST_CASE(test_ckit_array_push_pop);
	RUN_TEST_CASE(test_ckit_array_fit_and_cap);
	RUN_TEST_CASE(test_ckit_array_clear);
	RUN_TEST_CASE(test_ckit_array_del);
	RUN_TEST_CASE(test_ckit_array_end);
	RUN_TEST_CASE(test_ckit_array_large);
	RUN_TEST_CASE(test_ckit_array_structs);

	// Strings - basic.
	RUN_TEST_CASE(test_ckit_string_set_append);
	RUN_TEST_CASE(test_ckit_string_push_pop);
	RUN_TEST_CASE(test_ckit_string_dup_make);
	RUN_TEST_CASE(test_ckit_string_clear);
	RUN_TEST_CASE(test_ckit_string_fit);

	// Strings - formatting.
	RUN_TEST_CASE(test_ckit_string_fmt);
	RUN_TEST_CASE(test_ckit_string_fmt_append);
	RUN_TEST_CASE(test_ckit_string_fmt_long);

	// Strings - compare.
	RUN_TEST_CASE(test_ckit_string_compare);

	// Strings - search.
	RUN_TEST_CASE(test_ckit_string_prefix_suffix);
	RUN_TEST_CASE(test_ckit_string_index_of);
	RUN_TEST_CASE(test_ckit_string_find);

	// Strings - transform.
	RUN_TEST_CASE(test_ckit_string_case);
	RUN_TEST_CASE(test_ckit_string_trim);
	RUN_TEST_CASE(test_ckit_string_pad);
	RUN_TEST_CASE(test_ckit_string_replace);
	RUN_TEST_CASE(test_ckit_string_dedup);
	RUN_TEST_CASE(test_ckit_string_erase);
	RUN_TEST_CASE(test_ckit_string_append_range);

	// Strings - split.
	RUN_TEST_CASE(test_ckit_string_split_once);
	RUN_TEST_CASE(test_ckit_string_split);

	// Strings - conversions.
	RUN_TEST_CASE(test_ckit_string_conversions);
	RUN_TEST_CASE(test_ckit_string_from_primitives);

	// Strings - UTF-8.
	RUN_TEST_CASE(test_ckit_string_utf8);

	// Strings - hash.
	RUN_TEST_CASE(test_ckit_string_hash);

	// Path utilities.
	RUN_TEST_CASE(test_ckit_path_fname);
	RUN_TEST_CASE(test_ckit_path_fname_no_ext);
	RUN_TEST_CASE(test_ckit_path_ext);
	RUN_TEST_CASE(test_ckit_path_pop);
	RUN_TEST_CASE(test_ckit_path_compact);
	RUN_TEST_CASE(test_ckit_path_dir_of);
	RUN_TEST_CASE(test_ckit_path_top_of);
	RUN_TEST_CASE(test_ckit_path_norm);

	// Map - typed.
	RUN_TEST_CASE(test_ckit_map_int);
	RUN_TEST_CASE(test_ckit_map_get_ptr);
	RUN_TEST_CASE(test_ckit_map_update);
	RUN_TEST_CASE(test_ckit_map_del);
	RUN_TEST_CASE(test_ckit_map_clear);
	RUN_TEST_CASE(test_ckit_map_iteration);
	RUN_TEST_CASE(test_ckit_map_large);
	RUN_TEST_CASE(test_ckit_map_struct_values);

	// Map - uint64_t convenience.
	RUN_TEST_CASE(test_ckit_map_u64_convenience);

	// Map - swap and sort.
	RUN_TEST_CASE(test_ckit_map_swap);
	RUN_TEST_CASE(test_ckit_map_ssort);

	// String interning.
	RUN_TEST_CASE(test_ckit_intern_basic);
	RUN_TEST_CASE(test_ckit_intern_range);
	RUN_TEST_CASE(test_ckit_intern_constructed);
	RUN_TEST_CASE(test_ckit_intern_as_map_key);
	RUN_TEST_CASE(test_ckit_intern_empty_string);
	RUN_TEST_CASE(test_ckit_intern_many_strings);

	// String edge cases and bug coverage.
	RUN_TEST_CASE(test_ckit_string_slast_empty);
	RUN_TEST_CASE(test_ckit_string_sfirst_empty);
	RUN_TEST_CASE(test_ckit_string_scontains_edge);
	RUN_TEST_CASE(test_ckit_string_sappend_null);
	RUN_TEST_CASE(test_ckit_string_split_once_edge_cases);
	RUN_TEST_CASE(test_ckit_string_sdedup_edge_cases);
	RUN_TEST_CASE(test_ckit_string_utf8_truncated);
	RUN_TEST_CASE(test_ckit_string_stouint_large);
	RUN_TEST_CASE(test_ckit_string_trim_all_whitespace);
	RUN_TEST_CASE(test_ckit_string_erase_edge_cases);
	RUN_TEST_CASE(test_ckit_string_replace_edge_cases);
	RUN_TEST_CASE(test_ckit_string_spush_spop_sequence);
	RUN_TEST_CASE(test_ckit_string_prefix_suffix_edge_cases);
	RUN_TEST_CASE(test_ckit_string_append_range_edge_cases);
	RUN_TEST_CASE(test_ckit_string_pad_edge_cases);
	RUN_TEST_CASE(test_ckit_string_ssplit_edge_cases);

	// Hash.
	RUN_TEST_CASE(test_ckit_fnv1a);
}
