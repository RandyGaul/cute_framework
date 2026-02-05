/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
using namespace Cute;

/* Basic test of apush/afree etc. */
TEST_CASE(test_array_macros_simple)
{
	// Some push/pop of integers.
	int* vec = NULL;
	apush(vec, 1);
	apush(vec, 2);
	apush(vec, 3);
	REQUIRE(vec[0] == 1);
	REQUIRE(vec[1] == 2);
	REQUIRE(vec[2] == 3);
	REQUIRE(apop(vec) == 3);
	REQUIRE(asize(vec) == 2);
	CF_UNUSED(apop(vec));
	REQUIRE(apop(vec) == 1);
	REQUIRE(asize(vec) == 0);
	for (int i = 0; i < 32; ++i) {
		apush(vec, i);
	}
	for (int i = 0; i < 32; ++i) {
		REQUIRE(vec[i] == i);
	}
	int* vec2 = NULL;
	aset(vec2, vec);
	for (int i = 0; i < 32; ++i) {
		REQUIRE(vec[i] == vec2[i]);
	}
	afree(vec);
	afree(vec2);

	// Push/pop for v2 structs.
	v2* v = NULL;
	apush(v, V2(1, 2));
	apush(v, V2(5, 1));
	apush(v, V2(10, 3));
	v2 val = apop(v);
	REQUIRE(val.x == 10);
	REQUIRE(val.y == 3);
	val = apop(v);
	REQUIRE(val.x == 5);
	REQUIRE(val.y == 1);
	val = apop(v);
	REQUIRE(val.x == 1);
	REQUIRE(val.y == 2);
	afree(v);

	return true;
}

/* Basic test of spush/sfree etc. */
TEST_CASE(test_string_macros_simple)
{
	// Build a string, format it, and append formatted data.
	char* s = NULL;
	spush(s, 'a');
	spush(s, 'b');
	spush(s, 'c');
	REQUIRE(slen(s) == 3);
	REQUIRE(scount(s) == 4);
	REQUIRE(sequ(s, "abc"));
	sfmt(s, "WELP %d", 1500);
	REQUIRE(sequ(s, "WELP 1500"));
	sfmt_append(s, "%s", ", append_me");
	REQUIRE(sequ(s, "WELP 1500, append_me"));
	REQUIRE(CF_STRLEN(s) == slen(s));
	REQUIRE(CF_STRLEN(s) + 1 == scount(s));
	sfree(s);
	REQUIRE(s == NULL);
	const char* long_string = "some string longer than the default string length forcing an `sfit` call to test it's code path.";
	sfmt(s, "%s", long_string);
	REQUIRE(sequ(s, long_string));
	sfree(s);

	// Test out sfmt and variants.
	sfmt(s, "%d,%d,%d,", 0,1,2);
	REQUIRE(slen(s) == 6);
	REQUIRE(scount(s) == 7);
	REQUIRE(sequ(s, "0,1,2,"));
	sfmt(s, "%d,%d,%d,", 0,1,2);
	REQUIRE(slen(s) == 6);
	REQUIRE(scount(s) == 7);
	REQUIRE(sequ(s, "0,1,2,"));
	sfree(s);
	REQUIRE(slen(s) == 0);
	sfmt_append(s, "%d,%d,%d,", 0,1,2);
	REQUIRE(slen(s) == 6);
	REQUIRE(scount(s) == 7);
	REQUIRE(sequ(s, "0,1,2,"));
	sfmt_append(s, "%d,%d,%d,", 0,1,2);
	REQUIRE(slen(s) == 12);
	REQUIRE(scount(s) == 13);
	REQUIRE(sequ(s, "0,1,2,0,1,2,"));
	sclear(s);
	REQUIRE(slen(s) == 0);

	return true;
}

/* Test out basic use cases for all the advanced string macro functions. */
TEST_CASE(test_string_macros_advanced)
{
	char* a = NULL;
	char* b = NULL;
	sset(a, "hi");
	sset(b, "yo");
	REQUIRE(sequ(a, "hi"));
	REQUIRE(sequ(b, "yo"));
	REQUIRE(slen(a) == 2);
	REQUIRE(slen(b) == 2);
	const char* long_string = "some string longer than the default string length forcing an `sfit` call to test it's code path.";
	sset(a, long_string);
	REQUIRE(sequ(a, long_string));
	sclear(a);
	sclear(b);
	sset(a, "dup me ok?");
	const char* dup_a = sdup(a);
	REQUIRE(sequ(a, dup_a) && a != b);
	sfree(dup_a);
	sset(a, "TeStInG cAsE");
	sset(b, "tEsTiNg CaSe");
	REQUIRE(siequ(a, b));
	sset(a, "hello world");
	REQUIRE(sprefix(a, "hello"));
	REQUIRE(ssuffix(a, "world"));
	REQUIRE(scontains(a, "llo wo"));
	stoupper(a);
	REQUIRE(sequ(a, "HELLO WORLD"));
	stolower(a);
	REQUIRE(sequ(a, "hello world"));
	sappend(a, " :)");
	REQUIRE(sequ(a, "hello world :)"));
	sset(a, "   spac es  ");
	sset(b, a);
	sltrim(a);
	srtrim(b);
	REQUIRE(sequ(a, "spac es  "));
	REQUIRE(slen(a) == CF_STRLEN("spac es  "));
	REQUIRE(sequ(b, "   spac es"));
	REQUIRE(slen(b) == CF_STRLEN("   spac es"));
	sset(a, "   spac es  ");
	strim(a);
	REQUIRE(sequ(a, "spac es"));
	REQUIRE(slen(a) == CF_STRLEN("spac es"));
	sset(a, "pad me");
	srpad(a, 'x', 3);
	REQUIRE(sequ(a, "pad mexxx"));
	REQUIRE(slen(a) == CF_STRLEN("pad mexxx"));
	slpad(a, 'x', 2);
	REQUIRE(sequ(a, "xxpad mexxx"));
	REQUIRE(slen(a) == CF_STRLEN("xxpad mexxx"));
	sset(a, "split.here");
	sfree(b);
	b = ssplit_once(a, '.');
	REQUIRE(sequ(a, "here"));
	REQUIRE(slen(a) == CF_STRLEN("here"));
	REQUIRE(sequ(b, "split"));
	REQUIRE(slen(b) == CF_STRLEN("split"));
	sset(a, "split.here.in.a.loop");
	const char* splits[] = {
		"split",
		"here",
		"in",
		"a",
		"loop",
	};
	char** c = ssplit(a, '.');
	for (int i = 0; i < asize(c); ++i) {
		REQUIRE(sequ(c[i], splits[i]));
		sfree(c[i]);
	}
	afree(c);
	sset(a, "012330");
	REQUIRE(sfirst_index_of(a, '0') == 0);
	REQUIRE(sfirst_index_of(a, '1') == 1);
	REQUIRE(sfirst_index_of(a, '2') == 2);
	REQUIRE(sfirst_index_of(a, '3') == 3);
	REQUIRE(slast_index_of(a, '3') == 4);
	REQUIRE(slast_index_of(a, '0') == 5);
	REQUIRE(stohex("0xbaadf0") == 0xbaadf0FF);
	sset(a, "0xbaadf00d");
	REQUIRE(stohex(a) == 0xbaadf00d);
	shex(b, 0xbaadf00d);
	REQUIRE(sequ(a, b));
	sset(a, "hi hi hi");
	sreplace(a, "hi", "oof");
	REQUIRE(sequ(a, "oof oof oof"));
	REQUIRE(slen(a) == CF_STRLEN("oof oof oof"));
	sreplace(a, "oof", "hi");
	REQUIRE(sequ(a, "hi hi hi"));
	REQUIRE(slen(a) == CF_STRLEN("hi hi hi"));
	sset(a, "erase me");
	serase(a, 0, 2);
	REQUIRE(sequ(a, "ase me"));
	REQUIRE(slen(a) == CF_STRLEN("ase me"));
	sset(a, "erase.me.please");
	serase(a, 10, 10);
	REQUIRE(sequ(a, "erase.me.p"));
	REQUIRE(slen(a) == CF_STRLEN("erase.me.p"));
	sset(a, "erase.me.please");
	serase(a, 30, 10);
	REQUIRE(sequ(a, "erase.me.please"));
	REQUIRE(slen(a) == CF_STRLEN("erase.me.please"));
	serase(a, -5, 10);
	REQUIRE(sequ(a, ".me.please"));
	REQUIRE(slen(a) == CF_STRLEN(".me.please"));
	sfree(a);
	sfree(b);

	return true;
}

/* Run the string interning API. */
TEST_CASE(test_string_interning)
{
	char* a = NULL;
	char* b = NULL;
	sset(a, "string 1");
	sset(b, "string 1");
	const char* c = "string 2";
	const char* ia = sintern(a);
	const char* ib = sintern(b);
	const char* ic = sintern(c);
	const char* ic2 = sintern(c);
	REQUIRE(!CF_STRCMP(a, ia));
	REQUIRE(!CF_STRCMP(b, ib));
	REQUIRE(!CF_STRCMP(ic, ic2));
	REQUIRE(ia == ib);
	REQUIRE(ia != ic);
	REQUIRE(ic == ic2);
	REQUIRE(silen(ia) == (int)CF_STRLEN(ia));
	REQUIRE(silen(ib) == (int)CF_STRLEN(ib));
	REQUIRE(silen(ic) == (int)CF_STRLEN(ic));
	REQUIRE(silen(ic2) == (int)CF_STRLEN(ic2));
	REQUIRE(sivalid(ia));
	REQUIRE(sivalid(ib));
	REQUIRE(sivalid(ic));
	REQUIRE(sivalid(ic2));
	REQUIRE(!sivalid(a));
	REQUIRE(!sivalid(b));
	REQUIRE(!sivalid(c));
	sfree(a);
	sfree(b);

	return true;
}

/* Run Map<T> API and sintern API */
TEST_CASE(test_dictionary_and_interning)
{
	Map<int> h;
	const char* a = "test 1";
	const char* b = "test 2";
	const char* ia = sintern(a);
	const char* ib = sintern(b);
	h.insert(ia, 2);
	h.insert(ib, 3);
	auto find_ptr = h.try_find(ia);
	REQUIRE(find_ptr && *find_ptr == 2);
	find_ptr = h.try_find(ib);
	REQUIRE(find_ptr && *find_ptr == 3);

	return true;
}

TEST_CASE(test_split_for_memleaks)
{
	const char* data = "asdf,fa;sdf,a,f,q2,1,d,afs;d,a,1,,a,sa,d,v,,";
	Array<String> a = String::split(data, ';');
	for (int i = 0; i < a.size(); ++i) {
		Array<String> b = a[i].split(',');
	}

	return true;
}

TEST_SUITE(test_string)
{
	RUN_TEST_CASE(test_array_macros_simple);
	RUN_TEST_CASE(test_string_macros_simple);
 	RUN_TEST_CASE(test_string_macros_advanced);
	RUN_TEST_CASE(test_string_interning);
	RUN_TEST_CASE(test_dictionary_and_interning);
	RUN_TEST_CASE(test_split_for_memleaks);
}
