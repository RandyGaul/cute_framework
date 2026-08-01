/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifdef __cplusplus
#error "This code must be compiled as C, not C++"
#endif

#include "test_harness.h"

#include <cute_math.h>

TEST_CASE(test_make_translation_v2_c) {
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

TEST_CASE(test_make_translation_floats_c) {
	CF_M3x2 m = cf_make_translation(3.0f, 4.0f);

	REQUIRE(m.p.x == 3.0f);
	REQUIRE(m.p.y == 4.0f);
	REQUIRE(m.m.x.x == 1.0f);
	REQUIRE(m.m.x.y == 0.0f);
	REQUIRE(m.m.y.x == 0.0f);
	REQUIRE(m.m.y.y == 1.0f);

	return true;
}

TEST_CASE(test_make_translation_negative_c) {
	CF_M3x2 m1 = cf_make_translation(-5.0f, -10.0f);
	REQUIRE(m1.p.x == -5.0f);
	REQUIRE(m1.p.y == -10.0f);

	CF_V2 p = cf_v2(-7, -3);
	CF_M3x2 m2 = cf_make_translation(p);
	REQUIRE(m2.p.x == -7.0f);
	REQUIRE(m2.p.y == -3.0f);

	return true;
}

TEST_CASE(test_make_translation_mixed_c) {
	/* Test with variable arguments */
	float x = 10.0f;
	float y = 20.0f;
	CF_M3x2 m1 = cf_make_translation(x, y);
	REQUIRE(m1.p.x == 10.0f);
	REQUIRE(m1.p.y == 20.0f);

	/* Test with CF_V2 variable */
	CF_V2 pos = cf_v2(15, 25);
	CF_M3x2 m2 = cf_make_translation(pos);
	REQUIRE(m2.p.x == 15.0f);
	REQUIRE(m2.p.y == 25.0f);

	return true;
}

TEST_CASE(test_atan2_360_floats_c) {
	float angle = cf_atan2_360(0.0f, 1.0f);
	REQUIRE(angle >= 0.0f && angle <= CF_PI * 2.0f);

	float angle2 = cf_atan2_360(1.0f, 0.0f);
	REQUIRE(angle2 >= 0.0f && angle2 <= CF_PI * 2.0f);

	return true;
}

TEST_CASE(test_atan2_360_v2_c) {
	CF_V2 v = cf_v2(1.0f, 0.0f);
	float angle = cf_atan2_360(v);
	REQUIRE(angle >= 0.0f && angle <= CF_PI * 2.0f);

	CF_V2 v2 = cf_v2(0.0f, 1.0f);
	float angle2 = cf_atan2_360(v2);
	REQUIRE(angle2 >= 0.0f && angle2 <= CF_PI * 2.0f);

	return true;
}

TEST_CASE(test_atan2_360_sincos_c) {
	CF_SinCos sc = cf_sincos(CF_PI / 4.0f);
	float angle = cf_atan2_360(sc);
	REQUIRE(angle >= 0.0f && angle <= CF_PI * 2.0f);

	return true;
}

TEST_CASE(test_atan2_360_mixed_c) {
	/* Test with float variables */
	float y = 1.0f;
	float x = 1.0f;
	float angle1 = cf_atan2_360(y, x);
	REQUIRE(angle1 >= 0.0f && angle1 <= CF_PI * 2.0f);

	/* Test with CF_V2 variable */
	CF_V2 vec = cf_v2(1.0f, 1.0f);
	float angle2 = cf_atan2_360(vec);
	REQUIRE(angle2 >= 0.0f && angle2 <= CF_PI * 2.0f);

	/* Test with CF_SinCos variable */
	CF_SinCos rotation = cf_sincos(CF_PI / 2.0f);
	float angle3 = cf_atan2_360(rotation);
	REQUIRE(angle3 >= 0.0f && angle3 <= CF_PI * 2.0f);

	return true;
}

/* Fill the stack region reused by the next call's frame with non-zero
   garbage, so stale values can't accidentally pass as zeros. */
static float dirty_stack(void)
{
	volatile float garbage[128];
	float sum = 0;
	for (int i = 0; i < 128; ++i) garbage[i] = 123.456f + (float)i;
	for (int i = 0; i < 128; ++i) sum += garbage[i];
	return sum;
}


TEST_CASE(test_cube_in_out_c) {
	REQUIRE(cf_cube_in_out(0.0f) == 0.0f);
	REQUIRE(cf_cube_in_out(0.5f) == 0.5f);
	REQUIRE(cf_cube_in_out(1.0f) == 1.0f);
	REQUIRE(cf_abs(cf_cube_in_out(0.75f) - 0.9375f) < 1e-6f);
	/* Continuous across the midpoint. */
	REQUIRE(cf_abs(cf_cube_in_out(0.5001f) - 0.5f) < 1e-3f);
	return true;
}

TEST_CASE(test_shortest_arc_no_nan_c) {
	/* Near-parallel normalized vectors: dot(a, b) can round above 1.0f,
	   and acosf of that is NaN. */
	for (int i = 0; i < 1000; ++i) {
		float angle = (float)i * (CF_PI * 2.0f / 1000.0f);
		CF_V2 u = cf_norm(cf_v2(CF_COSF(angle) * 3.7f, CF_SINF(angle) * 3.7f));
		float arc = cf_shortest_arc(u, u);
		REQUIRE(arc == arc); /* not NaN */
		REQUIRE(cf_abs(arc) < 1e-3f);
	}
	/* Sanity: quarter turn CCW is +pi/2. */
	float q = cf_shortest_arc(cf_v2(1.0f, 0.0f), cf_v2(0.0f, 1.0f));
	REQUIRE(cf_abs(q - CF_PI / 2.0f) < 1e-5f);
	return true;
}

TEST_CASE(test_mod_floored_c) {
	/* Scalar overloads must match the CF_V2 overload's floored convention. */
	CF_V2 mv = cf_mod(cf_v2(-1.0f, -1.0f), cf_v2(3.0f, 3.0f));
	REQUIRE(mv.x == 2.0f);
	REQUIRE(cf_mod(-1.0f, 3.0f) == 2.0f);
	REQUIRE(cf_mod(-1.0, 3.0) == 2.0);
	REQUIRE(cf_mod(7.0f, 3.0f) == 1.0f);
	/* Guards against the old (int) cast, which is UB for x/m > INT_MAX and
	   returned garbage here. (The floored formula still loses float precision
	   for huge ratios with m != 1, matching the CF_V2 overload.) */
	REQUIRE(cf_mod(1.0e10f, 1.0f) == 0.0f);
	return true;
}

TEST_CASE(test_raycast_miss_zeroed_c) {
	/* Documented contract (CF_Raycast): when hit is false, t and n are zero'd out. */
	CF_Ray miss = cf_make_ray(cf_v2(-10.0f, 5.0f), cf_v2(1.0f, 0.0f), 1.0f);

	CF_Circle circ;
	circ.p = cf_v2(0, 0);
	circ.r = 1.0f;
	dirty_stack();
	CF_Raycast rc = cf_ray_to_circle(miss, circ);
	REQUIRE(!rc.hit);
	REQUIRE(rc.t == 0.0f);
	REQUIRE(rc.n.x == 0.0f && rc.n.y == 0.0f);

	CF_Aabb box = cf_make_aabb(cf_v2(-1, -1), cf_v2(1, 1));
	dirty_stack();
	rc = cf_ray_to_aabb(miss, box);
	REQUIRE(!rc.hit);
	REQUIRE(rc.t == 0.0f);
	REQUIRE(rc.n.x == 0.0f && rc.n.y == 0.0f);

	CF_Capsule cap;
	cap.a = cf_v2(0, -1);
	cap.b = cf_v2(0, 1);
	cap.r = 0.5f;
	dirty_stack();
	rc = cf_ray_to_capsule(miss, cap);
	REQUIRE(!rc.hit);
	REQUIRE(rc.t == 0.0f);
	REQUIRE(rc.n.x == 0.0f && rc.n.y == 0.0f);

	CF_Poly poly;
	poly.count = 4;
	poly.verts[0] = cf_v2(-1, -1);
	poly.verts[1] = cf_v2(1, -1);
	poly.verts[2] = cf_v2(1, 1);
	poly.verts[3] = cf_v2(-1, 1);
	cf_make_poly(&poly);
	dirty_stack();
	rc = cf_ray_to_poly(miss, &poly);
	REQUIRE(!rc.hit);
	REQUIRE(rc.t == 0.0f);
	REQUIRE(rc.n.x == 0.0f && rc.n.y == 0.0f);

	dirty_stack();
	CF_Raycast out;
	bool hit = cf_cast_ray(miss, &circ, CF_SHAPE_TYPE_CIRCLE, &out);
	REQUIRE(!hit);
	REQUIRE(out.t == 0.0f);
	REQUIRE(out.n.x == 0.0f && out.n.y == 0.0f);

	return true;
}

TEST_CASE(test_ray_to_halfspace_c) {
	/* t must be a distance along the normalized ray direction, matching every
	   other cf_ray_to_* cast, so cf_impact(ray, t) lands on the plane. */
	CF_Halfspace plane = cf_plane(cf_v2(0.0f, 1.0f), 0.0f);
	CF_Ray ray = cf_make_ray(cf_v2(0.0f, 10.0f), cf_v2(0.0f, -1.0f), 100.0f);
	CF_Raycast rc = cf_ray_to_halfspace(ray, plane);
	REQUIRE(rc.hit);
	REQUIRE(cf_abs(rc.t - 10.0f) < 1e-3f);
	CF_V2 hitp = cf_impact(ray, rc.t);
	REQUIRE(cf_abs(hitp.y) < 1e-3f);

	/* A ray lying exactly in the plane must hit at t == 0, not NaN. */
	CF_Ray inplane = cf_make_ray(cf_v2(0.0f, 0.0f), cf_v2(1.0f, 0.0f), 10.0f);
	CF_Raycast rc2 = cf_ray_to_halfspace(inplane, plane);
	REQUIRE(rc2.t == rc2.t); /* not NaN */
	REQUIRE(rc2.hit);
	REQUIRE(rc2.t == 0.0f);

	return true;
}

TEST_CASE(test_slice_on_plane_verts_c) {
	CF_Halfspace plane = cf_plane(cf_v2(0.0f, 1.0f), 0.0f);

	/* Convex hexagon with two vertices exactly on the slice plane. Both
	   halves are trapezoids of area 3; front + back must tile the input. */
	CF_Poly hex;
	hex.count = 6;
	hex.verts[0] = cf_v2(2, 0);
	hex.verts[1] = cf_v2(1, 1);
	hex.verts[2] = cf_v2(-1, 1);
	hex.verts[3] = cf_v2(-2, 0);
	hex.verts[4] = cf_v2(-1, -1);
	hex.verts[5] = cf_v2(1, -1);
	cf_norms(hex.verts, hex.norms, hex.count);
	CF_SliceOutput out = cf_slice(plane, hex, 1e-4f);
	REQUIRE(cf_abs(cf_calc_area(out.front) - 3.0f) < 1e-4f);
	REQUIRE(cf_abs(cf_calc_area(out.back) - 3.0f) < 1e-4f);

	/* Square whose top edge lies exactly in the plane: everything is
	   behind-or-on, so back must be the entire square. */
	CF_Poly sq;
	sq.count = 4;
	sq.verts[0] = cf_v2(-1, -1);
	sq.verts[1] = cf_v2(1, -1);
	sq.verts[2] = cf_v2(1, 0);
	sq.verts[3] = cf_v2(-1, 0);
	cf_norms(sq.verts, sq.norms, sq.count);
	out = cf_slice(plane, sq, 1e-4f);
	REQUIRE(cf_abs(cf_calc_area(out.back) - 2.0f) < 1e-4f);

	return true;
}

TEST_CASE(test_slice_nonconvex_no_overflow_c) {
	/* cf_slice documents convex input, but must stay memory-safe when handed
	   a non-convex (CCW) polygon anyway. A zigzag "comb" whose every edge
	   crosses the plane generates one intersection point per edge — more clip
	   output than CF_POLY_MAX_VERTS+1 — and must not smash the stack. */
	CF_Halfspace plane = cf_plane(cf_v2(0.0f, 1.0f), 0.0f);
	CF_Poly zig;
	zig.count = CF_POLY_MAX_VERTS;
	for (int i = 0; i < CF_POLY_MAX_VERTS; ++i) {
		zig.verts[i] = cf_v2((float)i, (i & 1) ? -0.5f : 0.5f);
	}
	cf_norms(zig.verts, zig.norms, zig.count);
	CF_SliceOutput out = cf_slice(plane, zig, 1e-4f);
	REQUIRE(out.front.count <= CF_POLY_MAX_VERTS);
	REQUIRE(out.back.count <= CF_POLY_MAX_VERTS);
	return true;
}

TEST_SUITE(test_math_c) {
	RUN_TEST_CASE(test_make_translation_v2_c);
	RUN_TEST_CASE(test_make_translation_floats_c);
	RUN_TEST_CASE(test_make_translation_negative_c);
	RUN_TEST_CASE(test_make_translation_mixed_c);
	RUN_TEST_CASE(test_atan2_360_floats_c);
	RUN_TEST_CASE(test_atan2_360_v2_c);
	RUN_TEST_CASE(test_atan2_360_sincos_c);
	RUN_TEST_CASE(test_atan2_360_mixed_c);
	RUN_TEST_CASE(test_cube_in_out_c);
	RUN_TEST_CASE(test_shortest_arc_no_nan_c);
	RUN_TEST_CASE(test_mod_floored_c);
	RUN_TEST_CASE(test_raycast_miss_zeroed_c);
	RUN_TEST_CASE(test_ray_to_halfspace_c);
	RUN_TEST_CASE(test_slice_on_plane_verts_c);
	RUN_TEST_CASE(test_slice_nonconvex_no_overflow_c);
}
