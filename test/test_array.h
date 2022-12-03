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

using namespace Cute;

static Array<String> s_get_array_of_strings()
{
	Array<String> b = {
		"a",
		"b",
		"c",
	};

	return b;
}

CUTE_TEST_CASE(test_array_list_init, "Array list initializers and strings.");
int test_array_list_init()
{
	Array<String> a = {
		"Hello",
		"Goodbye",
	};

	Array<String> b = {
		"1",
		"2",
	};

	CUTE_TEST_ASSERT(!CUTE_STRCMP(a[0].c_str(), "Hello"));
	CUTE_TEST_ASSERT(!CUTE_STRCMP(b[0].c_str(), "1"));
	CUTE_TEST_ASSERT(!CUTE_STRCMP(b[1].c_str(), "2"));

	Array<Array<String>> c = {
		a,
		b
	};

	CUTE_TEST_ASSERT(!CUTE_STRCMP(c[0][0].c_str(), "Hello"));
	CUTE_TEST_ASSERT(!CUTE_STRCMP(c[0][1].c_str(), "Goodbye"));
	CUTE_TEST_ASSERT(!CUTE_STRCMP(c[1][0].c_str(), "1"));
	CUTE_TEST_ASSERT(!CUTE_STRCMP(c[1][1].c_str(), "2"));

	Array<String> d = s_get_array_of_strings();
	CUTE_TEST_ASSERT(!CUTE_STRCMP(d[0].c_str(), "a"));
	CUTE_TEST_ASSERT(!CUTE_STRCMP(d[1].c_str(), "b"));
	CUTE_TEST_ASSERT(!CUTE_STRCMP(d[2].c_str(), "c"));

	return 0;
}
