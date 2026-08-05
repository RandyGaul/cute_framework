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

TEST_CASE(test_polygon_degenerate_c) {
	/* Zero-area (collinear) polygon: results must be finite, falling back
	   to the vertex average. */
	CF_Poly line;
	line.count = 3;
	line.verts[0] = cf_v2(0, 0);
	line.verts[1] = cf_v2(1, 0);
	line.verts[2] = cf_v2(2, 0);
	CF_V2 com = cf_center_of_mass(line);
	REQUIRE(com.x == com.x && com.y == com.y); /* not NaN */
	REQUIRE(cf_abs(com.x - 1.0f) < 1e-4f);

	CF_V2 pts[3];
	pts[0] = cf_v2(0, 0);
	pts[1] = cf_v2(1, 0);
	pts[2] = cf_v2(2, 0);
	CF_V2 cen = cf_centroid(pts, 3);
	REQUIRE(cen.x == cen.x && cen.y == cen.y); /* not NaN */
	REQUIRE(cf_abs(cen.x - 1.0f) < 1e-4f);

	/* count == 0 must not divide by zero (nor read verts[0]). */
	CF_Poly empty;
	empty.count = 0;
	CF_V2 z = cf_center_of_mass(empty);
	REQUIRE(z.x == 0.0f && z.y == 0.0f);

	/* count == 1 and count == 2: the point and the segment midpoint. */
	CF_Poly point;
	point.count = 1;
	point.verts[0] = cf_v2(3, 4);
	CF_V2 one = cf_center_of_mass(point);
	REQUIRE(one.x == 3.0f && one.y == 4.0f);

	CF_Poly seg;
	seg.count = 2;
	seg.verts[0] = cf_v2(0, 0);
	seg.verts[1] = cf_v2(2, 6);
	CF_V2 mid = cf_center_of_mass(seg);
	REQUIRE(mid.x == 1.0f && mid.y == 3.0f);

	return true;
}

TEST_CASE(test_distance_sq_degenerate_c) {
	/* Zero-length segment: squared distance to the point a == b. */
	float d = cf_distance_sq(cf_v2(1, 1), cf_v2(1, 1), cf_v2(5, 5));
	REQUIRE(d == d); /* not NaN */
	REQUIRE(cf_abs(d - 32.0f) < 1e-4f);
	return true;
}

TEST_CASE(test_make_aabb_verts_empty_c) {
	/* count == 0 must not read verts[0]. */
	CF_Aabb bb = cf_make_aabb_verts(NULL, 0);
	REQUIRE(bb.min.x == 0.0f && bb.min.y == 0.0f);
	REQUIRE(bb.max.x == 0.0f && bb.max.y == 0.0f);
	return true;
}

/* Hit-path semantics of the collision queries (now routed through Box2D's geometry
   layer): boolean overlaps, manifold direction and depth, raycast t/n, gjk closest
   points, and toi -- pinned so the routing preserves cute_c2-era contracts. */
TEST_CASE(test_collision_booleans_c) {
	CF_Circle a = cf_make_circle(cf_v2(0, 0), 1.0f);
	CF_Circle b = cf_make_circle(cf_v2(1.5f, 0), 1.0f);
	CF_Circle c = cf_make_circle(cf_v2(3.0f, 0), 1.0f);
	REQUIRE(cf_circle_to_circle(a, b));
	REQUIRE(!cf_circle_to_circle(a, c));

	CF_Aabb box = cf_make_aabb(cf_v2(-1, -1), cf_v2(1, 1));
	REQUIRE(cf_circle_to_aabb(cf_make_circle(cf_v2(1.5f, 0), 1.0f), box));
	REQUIRE(!cf_circle_to_aabb(cf_make_circle(cf_v2(3.0f, 0), 1.0f), box));
	REQUIRE(cf_aabb_to_aabb(box, cf_make_aabb(cf_v2(0, 0), cf_v2(2, 2))));
	REQUIRE(!cf_aabb_to_aabb(box, cf_make_aabb(cf_v2(2, 2), cf_v2(3, 3))));

	CF_Capsule cap = cf_make_capsule(cf_v2(0, -1), cf_v2(0, 1), 0.5f);
	REQUIRE(cf_circle_to_capsule(cf_make_circle(cf_v2(1.0f, 0), 0.6f), cap));
	REQUIRE(!cf_circle_to_capsule(cf_make_circle(cf_v2(3.0f, 0), 0.6f), cap));
	REQUIRE(cf_capsule_to_capsule(cap, cf_make_capsule(cf_v2(0.8f, -1), cf_v2(0.8f, 1), 0.5f)));
	REQUIRE(!cf_capsule_to_capsule(cap, cf_make_capsule(cf_v2(3, -1), cf_v2(3, 1), 0.5f)));
	REQUIRE(cf_aabb_to_capsule(box, cf_make_capsule(cf_v2(1.2f, -1), cf_v2(1.2f, 1), 0.5f)));

	CF_Poly poly;
	poly.count = 4;
	poly.verts[0] = cf_v2(2, -1);
	poly.verts[1] = cf_v2(4, -1);
	poly.verts[2] = cf_v2(4, 1);
	poly.verts[3] = cf_v2(2, 1);
	cf_make_poly(&poly);
	REQUIRE(cf_circle_to_poly(cf_make_circle(cf_v2(1.5f, 0), 1.0f), &poly));
	REQUIRE(!cf_circle_to_poly(cf_make_circle(cf_v2(0, 0), 1.0f), &poly));
	REQUIRE(cf_aabb_to_poly(cf_make_aabb(cf_v2(1, -1), cf_v2(3, 1)), &poly));
	REQUIRE(!cf_aabb_to_poly(cf_make_aabb(cf_v2(-1, -1), cf_v2(1, 1)), &poly));
	REQUIRE(cf_capsule_to_poly(cf_make_capsule(cf_v2(1.7f, -1), cf_v2(1.7f, 1), 0.5f), &poly));
	REQUIRE(cf_poly_to_poly(&poly, &poly));

	/* Generic dispatch agrees with the typed pair. */
	REQUIRE(cf_collided(&box, CF_SHAPE_TYPE_AABB, &poly, CF_SHAPE_TYPE_POLY) == 0);
	CF_Aabb touching = cf_make_aabb(cf_v2(1, -1), cf_v2(3, 1));
	REQUIRE(cf_collided(&touching, CF_SHAPE_TYPE_AABB, &poly, CF_SHAPE_TYPE_POLY) != 0);
	return true;
}

TEST_CASE(test_collision_manifolds_c) {
	/* Two overlapping circles: one contact point, normal from A to B, positive depth
	   equal to the overlap amount. */
	CF_Circle a = cf_make_circle(cf_v2(0, 0), 1.0f);
	CF_Circle b = cf_make_circle(cf_v2(1.5f, 0), 1.0f);
	CF_Manifold m = cf_circle_to_circle_manifold(a, b);
	REQUIRE(m.count == 1);
	REQUIRE(cf_abs(m.n.x - 1.0f) < 1e-3f && cf_abs(m.n.y) < 1e-3f);
	REQUIRE(cf_abs(m.depths[0] - 0.5f) < 1e-3f);

	/* Swapped order flips the normal: still A to B. */
	m = cf_circle_to_circle_manifold(b, a);
	REQUIRE(m.count == 1);
	REQUIRE(cf_abs(m.n.x + 1.0f) < 1e-3f);

	/* Separated shapes yield an empty manifold -- Box2D's speculative points must
	   never leak through. */
	m = cf_circle_to_circle_manifold(a, cf_make_circle(cf_v2(2.01f, 0), 1.0f));
	REQUIRE(m.count == 0);

	/* Overlapping boxes: two contact points along the +x face, depth = overlap. */
	CF_Aabb box_a = cf_make_aabb(cf_v2(-1, -1), cf_v2(1, 1));
	CF_Aabb box_b = cf_make_aabb(cf_v2(0.5f, -1), cf_v2(2.5f, 1));
	m = cf_aabb_to_aabb_manifold(box_a, box_b);
	REQUIRE(m.count == 2);
	REQUIRE(cf_abs(m.n.x - 1.0f) < 1e-3f && cf_abs(m.n.y) < 1e-3f);
	REQUIRE(cf_abs(m.depths[0] - 0.5f) < 1e-2f);
	REQUIRE(cf_abs(m.depths[1] - 0.5f) < 1e-2f);

	/* Circle vs poly through the generic entry point: same result either path. */
	CF_Poly poly;
	poly.count = 4;
	poly.verts[0] = cf_v2(2, -1);
	poly.verts[1] = cf_v2(4, -1);
	poly.verts[2] = cf_v2(4, 1);
	poly.verts[3] = cf_v2(2, 1);
	cf_make_poly(&poly);
	CF_Circle probe = cf_make_circle(cf_v2(1.5f, 0), 1.0f);
	CF_Manifold via_pair = cf_circle_to_poly_manifold(probe, &poly);
	CF_Manifold via_generic;
	cf_collide(&probe, CF_SHAPE_TYPE_CIRCLE, &poly, CF_SHAPE_TYPE_POLY, &via_generic);
	REQUIRE(via_pair.count == 1);
	REQUIRE(via_generic.count == 1);
	REQUIRE(cf_abs(via_pair.n.x - via_generic.n.x) < 1e-4f);
	REQUIRE(cf_abs(via_pair.depths[0] - via_generic.depths[0]) < 1e-4f);
	REQUIRE(cf_abs(via_pair.n.x - 1.0f) < 1e-3f); /* From the circle toward the poly. */
	return true;
}

TEST_CASE(test_raycast_hit_c) {
	/* Ray marching +x into a unit box 5 units away: t is a world-space distance along
	   the (normalized) direction, and the normal faces back at the ray. */
	CF_Ray ray = cf_make_ray(cf_v2(-6.0f, 0), cf_v2(1, 0), 100.0f);
	CF_Aabb box = cf_make_aabb(cf_v2(-1, -1), cf_v2(1, 1));
	CF_Raycast rc = cf_ray_to_aabb(ray, box);
	REQUIRE(rc.hit);
	REQUIRE(cf_abs(rc.t - 5.0f) < 1e-3f);
	REQUIRE(cf_abs(rc.n.x + 1.0f) < 1e-3f && cf_abs(rc.n.y) < 1e-3f);
	CF_V2 impact = cf_impact(ray, rc.t);
	REQUIRE(cf_abs(impact.x + 1.0f) < 1e-3f);

	CF_Circle circ = cf_make_circle(cf_v2(0, 0), 1.0f);
	rc = cf_ray_to_circle(ray, circ);
	REQUIRE(rc.hit);
	REQUIRE(cf_abs(rc.t - 5.0f) < 1e-3f);
	REQUIRE(cf_abs(rc.n.x + 1.0f) < 1e-3f);

	CF_Capsule cap = cf_make_capsule(cf_v2(0, -1), cf_v2(0, 1), 1.0f);
	rc = cf_ray_to_capsule(ray, cap);
	REQUIRE(rc.hit);
	REQUIRE(cf_abs(rc.t - 5.0f) < 1e-3f);

	/* A ray too short to reach must miss. */
	CF_Ray shorty = cf_make_ray(cf_v2(-6.0f, 0), cf_v2(1, 0), 3.0f);
	REQUIRE(!cf_ray_to_aabb(shorty, box).hit);
	return true;
}

TEST_CASE(test_gjk_toi_c) {
	/* Distance between two circles with radii on: the surface gap. Closest points sit
	   on the line between centers. */
	CF_Circle a = cf_make_circle(cf_v2(0, 0), 1.0f);
	CF_Circle b = cf_make_circle(cf_v2(5, 0), 1.0f);
	CF_V2 pa, pb;
	int iterations = 0;
	float d = cf_gjk(&a, CF_SHAPE_TYPE_CIRCLE, &b, CF_SHAPE_TYPE_CIRCLE, &pa, &pb, true, &iterations, NULL);
	REQUIRE(cf_abs(d - 3.0f) < 1e-3f);
	REQUIRE(cf_abs(pa.x - 1.0f) < 1e-3f);
	REQUIRE(cf_abs(pb.x - 4.0f) < 1e-3f);

	/* Radii off: center-to-center distance for point clouds. */
	d = cf_gjk(&a, CF_SHAPE_TYPE_CIRCLE, &b, CF_SHAPE_TYPE_CIRCLE, &pa, &pb, false, NULL, NULL);
	REQUIRE(cf_abs(d - 5.0f) < 1e-3f);

	/* A warm-started cache is accepted and gives the same answer. */
	CF_GjkCache cache = { 0 };
	d = cf_gjk(&a, CF_SHAPE_TYPE_CIRCLE, &b, CF_SHAPE_TYPE_CIRCLE, &pa, &pb, true, NULL, &cache);
	REQUIRE(cf_abs(d - 3.0f) < 1e-3f);
	d = cf_gjk(&a, CF_SHAPE_TYPE_CIRCLE, &b, CF_SHAPE_TYPE_CIRCLE, &pa, &pb, true, NULL, &cache);
	REQUIRE(cf_abs(d - 3.0f) < 1e-3f);

	/* Time of impact: b closes the 3-unit surface gap moving 6 units left, so impact
	   lands at toi 0.5 with the normal from A toward B. */
	CF_ToiResult toi = cf_toi(&a, CF_SHAPE_TYPE_CIRCLE, cf_v2(0, 0), &b, CF_SHAPE_TYPE_CIRCLE, cf_v2(-6.0f, 0), true);
	REQUIRE(toi.hit);
	REQUIRE(cf_abs(toi.toi - 0.5f) < 1e-2f);
	REQUIRE(cf_abs(toi.n.x - 1.0f) < 1e-2f);

	/* Moving apart: no hit, toi 1. */
	toi = cf_toi(&a, CF_SHAPE_TYPE_CIRCLE, cf_v2(0, 0), &b, CF_SHAPE_TYPE_CIRCLE, cf_v2(6.0f, 0), true);
	REQUIRE(!toi.hit);
	REQUIRE(toi.toi == 1.0f);
	return true;
}

TEST_CASE(test_hull_inflate_c) {
	/* Hull of a square plus an interior point: four verts survive, CCW winding with
	   outward normals from cf_make_poly. */
	CF_Poly poly;
	poly.count = 5;
	poly.verts[0] = cf_v2(-1, -1);
	poly.verts[1] = cf_v2(1, -1);
	poly.verts[2] = cf_v2(1, 1);
	poly.verts[3] = cf_v2(-1, 1);
	poly.verts[4] = cf_v2(0, 0); /* Interior; the hull must drop it. */
	cf_make_poly(&poly);
	REQUIRE(poly.count == 4);
	REQUIRE(cf_calc_area(poly) > 0); /* Positive area == CCW winding. */
	for (int i = 0; i < poly.count; ++i) {
		/* Every normal points away from the centroid. */
		CF_V2 to_vert = cf_sub_v2(poly.verts[i], cf_v2(0, 0));
		REQUIRE(cf_dot(poly.norms[i], to_vert) > 0);
	}

	/* Inflation grows every shape type; negative inflation shrinks the poly back. */
	CF_Circle circle = cf_make_circle(cf_v2(0, 0), 1.0f);
	cf_inflate(&circle, CF_SHAPE_TYPE_CIRCLE, 0.5f);
	REQUIRE(circle.r == 1.5f);

	CF_Aabb box = cf_make_aabb(cf_v2(-1, -1), cf_v2(1, 1));
	cf_inflate(&box, CF_SHAPE_TYPE_AABB, 0.5f);
	REQUIRE(box.min.x == -1.5f && box.max.y == 1.5f);

	float area = cf_calc_area(poly);
	cf_inflate(&poly, CF_SHAPE_TYPE_POLY, 0.1f);
	float grown = cf_calc_area(poly);
	REQUIRE(grown > area);
	cf_inflate(&poly, CF_SHAPE_TYPE_POLY, -0.1f);
	float shrunk = cf_calc_area(poly);
	REQUIRE(shrunk < grown);
	REQUIRE(cf_abs(shrunk - area) < 0.05f);
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
	RUN_TEST_CASE(test_collision_booleans_c);
	RUN_TEST_CASE(test_collision_manifolds_c);
	RUN_TEST_CASE(test_raycast_hit_c);
	RUN_TEST_CASE(test_gjk_toi_c);
	RUN_TEST_CASE(test_hull_inflate_c);
	RUN_TEST_CASE(test_slice_on_plane_verts_c);
	RUN_TEST_CASE(test_slice_nonconvex_no_overflow_c);
	RUN_TEST_CASE(test_polygon_degenerate_c);
	RUN_TEST_CASE(test_distance_sq_degenerate_c);
	RUN_TEST_CASE(test_make_aabb_verts_empty_c);
}
