/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// The C++ side of cute_math3d.h: overload resolution and operators. The _Generic dispatch that
// backs the same names in C is covered by test_math3d.c.

#include "test_harness.h"

#include <cute.h>
using namespace Cute;

static bool near_f(float a, float b) { return cf_abs(a - b) < 0.0001f; }
static bool near_v3(v3 a, v3 b) { return near_f(a.x, b.x) && near_f(a.y, b.y) && near_f(a.z, b.z); }

TEST_CASE(test_v3_overloads_cpp) {
	// The unsuffixed names must resolve to the 3d overloads without ambiguity, and the
	// pre-existing 2d overloads must still be reachable from the same names.
	REQUIRE(near_v3(cf_add(V3(1, 2, 3), V3(4, 5, 6)), V3(5, 7, 9)));
	REQUIRE(near_f(cf_dot(V3(1, 2, 3), V3(4, 5, 6)), 32.0f));
	REQUIRE(near_f(cf_dot(V2(1, 0), V2(1, 0)), 1.0f));
	REQUIRE(near_f(cf_len(V3(2, 3, 6)), 7.0f));
	REQUIRE(near_f(cf_len(V2(3, 4)), 5.0f));
	REQUIRE(near_v3(cf_cross(V3(1, 0, 0), V3(0, 1, 0)), V3(0, 0, 1)));
	REQUIRE(near_f(cf_cross(V2(1, 0), V2(0, 1)), 1.0f));
	REQUIRE(near_v3(cf_norm(V3(0, 5, 0)), V3(0, 1, 0)));
	REQUIRE(near_v3(cf_lerp(V3(0, 0, 0), V3(10, 10, 10), 0.5f), V3(5, 5, 5)));
	REQUIRE(near_v3(cf_min(V3(1, 5, 3), V3(4, 2, 6)), V3(1, 2, 3)));
	return true;
}

TEST_CASE(test_v3_operators_cpp) {
	v3 a = V3(1, 2, 3);
	v3 b = V3(4, 5, 6);

	REQUIRE(near_v3(a + b, V3(5, 7, 9)));
	REQUIRE(near_v3(b - a, V3(3, 3, 3)));
	REQUIRE(near_v3(a * 2.0f, V3(2, 4, 6)));
	REQUIRE(near_v3(2.0f * a, V3(2, 4, 6)));
	REQUIRE(near_v3(a * b, V3(4, 10, 18)));
	REQUIRE(near_v3(b / 2.0f, V3(2, 2.5f, 3)));
	REQUIRE(near_v3(-a, V3(-1, -2, -3)));

	v3 c = a;
	c += b;
	REQUIRE(near_v3(c, V3(5, 7, 9)));
	c -= b;
	REQUIRE(near_v3(c, a));
	c *= 2.0f;
	REQUIRE(near_v3(c, V3(2, 4, 6)));
	c /= 2.0f;
	REQUIRE(near_v3(c, a));

	REQUIRE(a == V3(1, 2, 3));
	REQUIRE(a != b);

	v4 d = V4(1, 2, 3, 4);
	REQUIRE((d + d).w == 8.0f);
	REQUIRE((d * 2.0f).x == 2.0f);
	REQUIRE(d == V4(1, 2, 3, 4));
	return true;
}

TEST_CASE(test_m4_operators_cpp) {
	m4 t = m4_translate(V3(10, 0, 0));
	m4 r = m4_rotate_z(CF_PI * 0.5f);

	// operator* must match cf_mul exactly.
	m4 via_op = t * r;
	m4 via_fn = cf_mul(t, r);
	for (int i = 0; i < 16; ++i) REQUIRE(near_f(via_op.elements[i], via_fn.elements[i]));

	v4 p = via_op * V4(1, 0, 0, 1);
	REQUIRE(near_f(p.x, 10.0f) && near_f(p.y, 1.0f));

	m4 prod = t * invert(t);
	m4 i4 = m4_identity();
	for (int i = 0; i < 16; ++i) REQUIRE(near_f(prod.elements[i], i4.elements[i]));
	return true;
}

TEST_CASE(test_quat_operators_cpp) {
	quat q = quat_from_axis_angle(V3(0, 0, 1), CF_PI * 0.5f);
	REQUIRE(near_v3(q * V3(1, 0, 0), V3(0, 1, 0)));

	quat r = quat_from_axis_angle(V3(0, 1, 0), 0.4f);
	v3 p = V3(0.3f, -0.7f, 1.1f);
	REQUIRE(near_v3((q * r) * p, transform_dir(to_m4(q) * to_m4(r), p)));
	return true;
}

// Same convention checks as the C suite, through the C++ names -- these are what a C++ user
// actually calls, and a wrapper could plausibly be wired to the wrong function.
TEST_CASE(test_projection_cpp) {
	float zn = 0.5f, zf = 100.0f;
	m4 p = perspective(CF_PI * 0.25f, 16.0f / 9.0f, zn, zf);
	v4 at_near = p * V4(0, 0, -zn, 1);
	v4 at_far = p * V4(0, 0, -zf, 1);
	REQUIRE(near_f(at_near.z / at_near.w, 0.0f));
	REQUIRE(near_f(at_far.z / at_far.w, 1.0f));

	m4 o = ortho(-10, 10, -10, 10, 1.0f, 50.0f);
	REQUIRE(near_f((o * V4(0, 0, -1.0f, 1)).z, 0.0f));
	REQUIRE(near_f((o * V4(0, 0, -50.0f, 1)).z, 1.0f));

	m4 v = look_at(V3(0, 0, 5), V3(0, 0, 0), V3(0, 1, 0));
	REQUIRE(near_v3(transform_point(v, V3(0, 0, 0)), V3(0, 0, -5)));
	return true;
}

TEST_SUITE(test_math3d) {
	RUN_TEST_CASE(test_v3_overloads_cpp);
	RUN_TEST_CASE(test_v3_operators_cpp);
	RUN_TEST_CASE(test_m4_operators_cpp);
	RUN_TEST_CASE(test_quat_operators_cpp);
	RUN_TEST_CASE(test_projection_cpp);
}
