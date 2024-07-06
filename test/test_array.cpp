/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

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

TEST_CASE(test_array_list_init)
{
		Array<String> a = {
		"Hello",
		"Goodbye",
	};

	Array<String> b = {
		"1",
		"2",
	};

	REQUIRE(!CF_STRCMP(a[0].c_str(), "Hello"));
	REQUIRE(!CF_STRCMP(b[0].c_str(), "1"));
	REQUIRE(!CF_STRCMP(b[1].c_str(), "2"));

	Array<Array<String>> c = {
		a,
		b
	};

	REQUIRE(!CF_STRCMP(c[0][0].c_str(), "Hello"));
	REQUIRE(!CF_STRCMP(c[0][1].c_str(), "Goodbye"));
	REQUIRE(!CF_STRCMP(c[1][0].c_str(), "1"));
	REQUIRE(!CF_STRCMP(c[1][1].c_str(), "2"));

	Array<String> d = s_get_array_of_strings();
	REQUIRE(!CF_STRCMP(d[0].c_str(), "a"));
	REQUIRE(!CF_STRCMP(d[1].c_str(), "b"));
	REQUIRE(!CF_STRCMP(d[2].c_str(), "c"));

	return true;
}

TEST_SUITE(test_array)
{
	RUN_TEST_CASE(test_array_list_init);
}