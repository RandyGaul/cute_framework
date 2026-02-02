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

TEST_CASE(test_atan2_360_floats) {
	float angle = cf_atan2_360(0.0f, 1.0f);
	REQUIRE(angle >= 0.0f && angle <= CF_PI * 2.0f);

	float angle2 = cf_atan2_360(1.0f, 0.0f);
	REQUIRE(angle2 >= 0.0f && angle2 <= CF_PI * 2.0f);

	return true;
}

TEST_CASE(test_atan2_360_v2) {
	CF_V2 v = cf_v2(1.0f, 0.0f);
	float angle = cf_atan2_360(v);
	REQUIRE(angle >= 0.0f && angle <= CF_PI * 2.0f);

	CF_V2 v2 = cf_v2(0.0f, 1.0f);
	float angle2 = cf_atan2_360(v2);
	REQUIRE(angle2 >= 0.0f && angle2 <= CF_PI * 2.0f);

	return true;
}

TEST_CASE(test_atan2_360_sincos) {
	CF_SinCos sc = cf_sincos(CF_PI / 4.0f);
	float angle = cf_atan2_360(sc);
	REQUIRE(angle >= 0.0f && angle <= CF_PI * 2.0f);

	return true;
}

TEST_SUITE(test_math) {
	RUN_TEST_CASE(test_make_translation_v2);
	RUN_TEST_CASE(test_make_translation_floats);
	RUN_TEST_CASE(test_make_translation_negative);
	RUN_TEST_CASE(test_atan2_360_floats);
	RUN_TEST_CASE(test_atan2_360_v2);
	RUN_TEST_CASE(test_atan2_360_sincos);
}
