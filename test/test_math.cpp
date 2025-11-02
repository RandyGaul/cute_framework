/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>

using namespace Cute;

TEST_CASE(test_make_translation_v2) {
	CF_V2 p = cf_v2(1, 2);
	CF_M3x2 m = cf_make_translation(p);

	REQUIRE(m.p.x == 1.0f);
	REQUIRE(m.p.y == 2.0f);
	REQUIRE(m.m.x.x == 1.0f);
	REQUIRE(m.m.x.y == 0.0f);
	REQUIRE(m.m.y.x == 0.0f);
	REQUIRE(m.m.y.y == 1.0f);

	return true;
}

TEST_CASE(test_make_translation_floats) {
	CF_M3x2 m = cf_make_translation(3.0f, 4.0f);

	REQUIRE(m.p.x == 3.0f);
	REQUIRE(m.p.y == 4.0f);
	REQUIRE(m.m.x.x == 1.0f);
	REQUIRE(m.m.x.y == 0.0f);
	REQUIRE(m.m.y.x == 0.0f);
	REQUIRE(m.m.y.y == 1.0f);

	return true;
}

TEST_CASE(test_make_translation_negative) {
	CF_M3x2 m1 = cf_make_translation(-5.0f, -10.0f);
	REQUIRE(m1.p.x == -5.0f);
	REQUIRE(m1.p.y == -10.0f);

	CF_V2 p = cf_v2(-7, -3);
	CF_M3x2 m2 = cf_make_translation(p);
	REQUIRE(m2.p.x == -7.0f);
	REQUIRE(m2.p.y == -3.0f);

	return true;
}

TEST_SUITE(test_math) {
	RUN_TEST_CASE(test_make_translation_v2);
	RUN_TEST_CASE(test_make_translation_floats);
	RUN_TEST_CASE(test_make_translation_negative);
}
