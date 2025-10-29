/*
	Cute Framework
	Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "cute_color.h"
#include "test_harness.h"

#include <cute.h>
using namespace Cute;

TEST_CASE(test_make_color_hex)
{
	CF_Color c = cf_make_color_hex(0xFFC41F);
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 255.0f / 255.0f);

	return true;
}

TEST_CASE(test_make_color_hex_string)
{
	CF_Color c = cf_make_color_hex_string("#ffc41f");
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 255.0f / 255.0f);

	c = cf_make_color_hex_string("#ffc41f");
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 255.0f / 255.0f);

	c = cf_make_color_hex_string("#ffc41f18");
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 24.0f / 255.0f);
	
	c = cf_make_color_hex_string("0xffc41f");
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 255.0f / 255.0f);

	c = cf_make_color_hex_string("0xffc41fff");
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 255.0f / 255.0f);

	c = cf_make_color_hex_string("0xffc41f18");
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 24.0f / 255.0f);

	return true;
}

TEST_SUITE(test_color)
{
	RUN_TEST_CASE(test_make_color_hex);
	RUN_TEST_CASE(test_make_color_hex_string);
}
