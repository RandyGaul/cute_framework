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
	b = sdup(a);
	CUTE_TEST_ASSERT(sequ(a, b));
	sset(a, "TeStInG cAsE");
	sset(b, "tEsTiNg CaSe");
	CUTE_TEST_ASSERT(siequ(a, b));
	sset(a, "hello world");
	CUTE_TEST_ASSERT(sprefix(a, "hello"));
	CUTE_TEST_ASSERT(ssuffix(a, "world"));
	CUTE_TEST_ASSERT(scontains(a, "llo wo"));
	supper(a);
	CUTE_TEST_ASSERT(sequ(a, "HELLO WORLD"));
	slower(a);
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
	b = ssplit(a, '.');
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
	char* c = NULL;
	int i = 0;
	while ((c = ssplit(a, '.'))) {
		CUTE_TEST_ASSERT(sequ(c, splits[i++]));
		sfree(c);
	}
	CUTE_TEST_ASSERT(sequ(a, "loop"));
	sset(a, "012330");
	CUTE_TEST_ASSERT(sfirst_index_of(a, '0') == 0);
	CUTE_TEST_ASSERT(sfirst_index_of(a, '1') == 1);
	CUTE_TEST_ASSERT(sfirst_index_of(a, '2') == 2);
	CUTE_TEST_ASSERT(sfirst_index_of(a, '3') == 3);
	CUTE_TEST_ASSERT(slast_index_of(a, '3') == 4);
	CUTE_TEST_ASSERT(slast_index_of(a, '0') == 5);
	sset(a, "0xbadf00d");
	CUTE_TEST_ASSERT(stohex(a) == 0xbadf00d);
	sset(b, shex(0xbadf00d));
	CUTE_TEST_ASSERT(sequ(a, b));
	sfree(a);
	sfree(b);

	return 0;
}
