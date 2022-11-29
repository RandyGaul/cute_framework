/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <cute.h>
using namespace cute;

CUTE_TEST_CASE(test_array_macros_simple, "Basic test of apush/afree etc.");
int test_array_macros_simple()
{
	// Some push/pop of integers.
	int* vec = NULL;
	apush(vec, 1);
	apush(vec, 2);
	apush(vec, 3);
	CUTE_TEST_ASSERT(vec[0] == 1);
	CUTE_TEST_ASSERT(vec[1] == 2);
	CUTE_TEST_ASSERT(vec[2] == 3);
	CUTE_TEST_ASSERT(apop(vec) == 3);
	CUTE_TEST_ASSERT(alen(vec) == 2);
	apop(vec);
	CUTE_TEST_ASSERT(apop(vec) == 1);
	CUTE_TEST_ASSERT(alen(vec) == 0);
	for (int i = 0; i < 32; ++i) {
		apush(vec, i);
	}
	for (int i = 0; i < 32; ++i) {
		CUTE_TEST_ASSERT(vec[i] == i);
	}
	int* vec2 = NULL;
	aset(vec2, vec);
	for (int i = 0; i < 32; ++i) {
		CUTE_TEST_ASSERT(vec[i] == vec2[i]);
	}
	afree(vec);
	afree(vec2);

	// Push/pop for v2 structs.
	v2* v = NULL;
	apush(v, V2(1, 2));
	apush(v, V2(5, 1));
	apush(v, V2(10, 3));
	v2 val = apop(v);
	CUTE_TEST_ASSERT(val.x == 10);
	CUTE_TEST_ASSERT(val.y == 3);
	val = apop(v);
	CUTE_TEST_ASSERT(val.x == 5);
	CUTE_TEST_ASSERT(val.y == 1);
	val = apop(v);
	CUTE_TEST_ASSERT(val.x == 1);
	CUTE_TEST_ASSERT(val.y == 2);
	afree(v);

	return 0;
}

CUTE_TEST_CASE(test_string_macros_simple, "Basic test of spush/sfree etc.");
int test_string_macros_simple()
{
	// Build a string, format it, and append formatted data.
	char* s = NULL;
	spush(s, 'a');
	spush(s, 'b');
	spush(s, 'c');
	CUTE_TEST_ASSERT(slen(s) == 3);
	CUTE_TEST_ASSERT(ssize(s) == 4);
	CUTE_TEST_ASSERT(sequ(s, "abc"));
	sfmt(s, "WELP %d", 1500);
	CUTE_TEST_ASSERT(sequ(s, "WELP 1500"));
	sfmt_append(s, "%s", ", append_me");
	CUTE_TEST_ASSERT(sequ(s, "WELP 1500, append_me"));
	CUTE_TEST_ASSERT(CUTE_STRLEN(s) == slen(s));
	CUTE_TEST_ASSERT(CUTE_STRLEN(s) + 1 == ssize(s));
	sfree(s);
	CUTE_TEST_ASSERT(s == NULL);
	const char* long_string = "some string longer than the default string length forcing an `sfit` call to test it's code path.";
	sfmt(s, "%s", long_string);
	CUTE_TEST_ASSERT(sequ(s, long_string));
	sfree(s);
	return 0;
}

CUTE_TEST_CASE(test_string_macros_advanced, "Test out basic use cases for all the advanced string macro functions.");
int test_string_macros_advanced()
{
	char* a = NULL;
	char* b = NULL;
	sset(a, "hi");
	sset(b, "yo");
	CUTE_TEST_ASSERT(sequ(a, "hi"));
	CUTE_TEST_ASSERT(sequ(b, "yo"));
	CUTE_TEST_ASSERT(slen(a) == 2);
	CUTE_TEST_ASSERT(slen(b) == 2);
	const char* long_string = "some string longer than the default string length forcing an `sfit` call to test it's code path.";
	sset(a, long_string);
	CUTE_TEST_ASSERT(sequ(a, long_string));
	sclear(a);
	sclear(b);
	sset(a, "dup me ok?");
	const char* dup_a = sdup(a);
	CUTE_TEST_ASSERT(sequ(a, dup_a) && a != b);
	sfree(dup_a);
	sset(a, "TeStInG cAsE");
	sset(b, "tEsTiNg CaSe");
	CUTE_TEST_ASSERT(siequ(a, b));
	sset(a, "hello world");
	CUTE_TEST_ASSERT(sprefix(a, "hello"));
	CUTE_TEST_ASSERT(ssuffix(a, "world"));
	CUTE_TEST_ASSERT(scontains(a, "llo wo"));
	stoupper(a);
	CUTE_TEST_ASSERT(sequ(a, "HELLO WORLD"));
	stolower(a);
	CUTE_TEST_ASSERT(sequ(a, "hello world"));
	sappend(a, " :)");
	CUTE_TEST_ASSERT(sequ(a, "hello world :)"));
	sset(a, "   spac es  ");
	sset(b, a);
	sltrim(a);
	srtrim(b);
	CUTE_TEST_ASSERT(sequ(a, "spac es  "));
	CUTE_TEST_ASSERT(slen(a) == CUTE_STRLEN("spac es  "));
	CUTE_TEST_ASSERT(sequ(b, "   spac es"));
	CUTE_TEST_ASSERT(slen(b) == CUTE_STRLEN("   spac es"));
	sset(a, "   spac es  ");
	strim(a);
	CUTE_TEST_ASSERT(sequ(a, "spac es"));
	CUTE_TEST_ASSERT(slen(a) == CUTE_STRLEN("spac es"));
	sset(a, "pad me");
	srpad(a, 'x', 3);
	CUTE_TEST_ASSERT(sequ(a, "pad mexxx"));
	CUTE_TEST_ASSERT(slen(a) == CUTE_STRLEN("pad mexxx"));
	slpad(a, 'x', 2);
	CUTE_TEST_ASSERT(sequ(a, "xxpad mexxx"));
	CUTE_TEST_ASSERT(slen(a) == CUTE_STRLEN("xxpad mexxx"));
	sset(a, "split.here");
	sfree(b);
	b = ssplit_once(a, '.');
	CUTE_TEST_ASSERT(sequ(a, "here"));
	CUTE_TEST_ASSERT(slen(a) == CUTE_STRLEN("here"));
	CUTE_TEST_ASSERT(sequ(b, "split"));
	CUTE_TEST_ASSERT(slen(b) == CUTE_STRLEN("split"));
	sset(a, "split.here.in.a.loop");
	const char* splits[] = {
		"split",
		"here",
		"in",
		"a",
		"loop",
	};
	char** c = ssplit(a, '.');
	for (int i = 0; i < alen(c); ++i) {
		CUTE_TEST_ASSERT(sequ(c[i], splits[i]));
		sfree(c[i]);
	}
	afree(c);
	sset(a, "012330");
	CUTE_TEST_ASSERT(sfirst_index_of(a, '0') == 0);
	CUTE_TEST_ASSERT(sfirst_index_of(a, '1') == 1);
	CUTE_TEST_ASSERT(sfirst_index_of(a, '2') == 2);
	CUTE_TEST_ASSERT(sfirst_index_of(a, '3') == 3);
	CUTE_TEST_ASSERT(slast_index_of(a, '3') == 4);
	CUTE_TEST_ASSERT(slast_index_of(a, '0') == 5);
	sset(a, "0xbadf00d");
	CUTE_TEST_ASSERT(stohex(a) == 0xbadf00d);
	shex(b, 0xbadf00d);
	CUTE_TEST_ASSERT(sequ(a, b));
	sset(a, "hi hi hi");
	sreplace(a, "hi", "oof");
	CUTE_TEST_ASSERT(sequ(a, "oof oof oof"));
	CUTE_TEST_ASSERT(slen(a) == CUTE_STRLEN("oof oof oof"));
	sreplace(a, "oof", "hi");
	CUTE_TEST_ASSERT(sequ(a, "hi hi hi"));
	CUTE_TEST_ASSERT(slen(a) == CUTE_STRLEN("hi hi hi"));
	sset(a, "erase me");
	serase(a, 0, 2);
	CUTE_TEST_ASSERT(sequ(a, "ase me"));
	CUTE_TEST_ASSERT(slen(a) == CUTE_STRLEN("ase me"));
	sset(a, "erase.me.please");
	serase(a, 10, 10);
	CUTE_TEST_ASSERT(sequ(a, "erase.me.p"));
	CUTE_TEST_ASSERT(slen(a) == CUTE_STRLEN("erase.me.p"));
	sset(a, "erase.me.please");
	serase(a, 30, 10);
	CUTE_TEST_ASSERT(sequ(a, "erase.me.please"));
	CUTE_TEST_ASSERT(slen(a) == CUTE_STRLEN("erase.me.please"));
	serase(a, -5, 10);
	CUTE_TEST_ASSERT(sequ(a, ".me.please"));
	CUTE_TEST_ASSERT(slen(a) == CUTE_STRLEN(".me.please"));
	sfree(a);
	sfree(b);
	return 0;
}

CUTE_TEST_CASE(test_string_interning, "Run the string interning API.");
int test_string_interning()
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
	CUTE_TEST_ASSERT(!CUTE_STRCMP(a, ia));
	CUTE_TEST_ASSERT(!CUTE_STRCMP(b, ib));
	CUTE_TEST_ASSERT(!CUTE_STRCMP(ic, ic2));
	CUTE_TEST_ASSERT(ia == ib);
	CUTE_TEST_ASSERT(ia != ic);
	CUTE_TEST_ASSERT(ic == ic2);
	CUTE_TEST_ASSERT(silen(ia) == (int)CUTE_STRLEN(ia));
	CUTE_TEST_ASSERT(silen(ib) == (int)CUTE_STRLEN(ib));
	CUTE_TEST_ASSERT(silen(ic) == (int)CUTE_STRLEN(ic));
	CUTE_TEST_ASSERT(silen(ic2) == (int)CUTE_STRLEN(ic2));
	CUTE_TEST_ASSERT(sivalid(ia));
	CUTE_TEST_ASSERT(sivalid(ib));
	CUTE_TEST_ASSERT(sivalid(ic));
	CUTE_TEST_ASSERT(sivalid(ic2));
	CUTE_TEST_ASSERT(!sivalid(a));
	CUTE_TEST_ASSERT(!sivalid(b));
	CUTE_TEST_ASSERT(!sivalid(c));
	sfree(a);
	sfree(b);
	return 0;
}

CUTE_TEST_CASE(test_dictionary_and_interning, "Run dictionary<T> API and sintern API");
int test_dictionary_and_interning()
{
	dictionary<const char*, int> h;
	const char* a = "test 1";
	const char* b = "test 2";
	const char* ia = sintern(a);
	const char* ib = sintern(b);
	h.insert(ia, 2);
	h.insert(ib, 3);
	auto find_ptr = h.try_find(ia);
	CUTE_TEST_ASSERT(find_ptr && *find_ptr == 2);
	find_ptr = h.try_find(ib);
	CUTE_TEST_ASSERT(find_ptr && *find_ptr == 3);
	return 0;
}
