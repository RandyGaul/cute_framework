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

// The new helper overloads: each unsuffixed C++ name must reach its 3d backend, without
// disturbing the 2d overloads that share the name (reflect, project).
TEST_CASE(test_helper_overloads_cpp) {
	// 3d reflect resolves alongside 2d reflect.
	REQUIRE(near_v3(cf_reflect(V3(1, -1, 0), V3(0, 1, 0)), V3(1, 1, 0)));
	v2 r2 = cf_reflect(V2(1, -1), V2(0, 1));
	REQUIRE(near_f(r2.x, 1.0f) && near_f(r2.y, 1.0f));
	REQUIRE(near_v3(reflect(V3(1, -1, 0), V3(0, 1, 0)), V3(1, 1, 0)));

	// project(v3, v3) overloads next to project(Halfspace, v2).
	REQUIRE(near_v3(cf_project(V3(2, 2, 0), V3(10, 0, 0)), V3(2, 0, 0)));
	REQUIRE(near_v3(project(V3(2, 2, 0), V3(10, 0, 0)), V3(2, 0, 0)));
	REQUIRE(near_v3(cf_slide(V3(1, -1, 0), V3(0, 1, 0)), V3(1, 0, 0)));
	REQUIRE(near_v3(slide(V3(1, -1, 0), V3(0, 1, 0)), V3(1, 0, 0)));

	v3 x, y;
	orthonormal_basis(V3(0, 1, 0), &x, &y);
	REQUIRE(near_v3(cross(x, y), V3(0, 1, 0)));

	quat q = quat_from_axis_angle(norm(V3(1, -2, 0.5f)), 0.9f);
	v3 p = V3(0.3f, -0.7f, 1.1f);
	REQUIRE(near_v3((q * inverse(q)) * p, p));
	REQUIRE(near_v3(nlerp(quat_identity(), q, 1.0f) * p, q * p));

	v3 axis;
	float angle;
	quat_to_axis_angle(q, &axis, &angle);
	REQUIRE(near_f(angle, 0.9f));

	m4 direct = m4_from_trs(V3(3, -4, 5), q, V3(2, 3, 4));
	m4 composed = m4_translate(V3(3, -4, 5)) * to_m4(q) * m4_scale(V3(2, 3, 4));
	for (int i = 0; i < 16; ++i) REQUIRE(near_f(direct.elements[i], composed.elements[i]));
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

// Rays against the three primitive shapes: impact distances, surface normals, misses, and
// the documented inside-the-shape behaviors.
TEST_CASE(test_geometry_raycasts_cpp) {
	// Ray straight down +z into a unit box centered at origin: hits the -z face at t = 4.
	CF_Ray3 ray = cf_make_ray3(V3(0, 0, -5), V3(0, 0, 5));
	CF_Aabb3 box = cf_make_aabb3_center(V3(0, 0, 0), V3(1, 1, 1));
	CF_Raycast3 hit = cf_ray3_to_aabb3(ray, box);
	REQUIRE(hit.hit);
	REQUIRE(near_f(hit.t, 4.0f));
	REQUIRE(near_v3(hit.n, V3(0, 0, -1)));

	// Same ray, box shifted out of the way: miss. Box behind the ray: miss.
	REQUIRE(!cf_ray3_to_aabb3(ray, cf_make_aabb3_center(V3(3, 0, 0), V3(1, 1, 1))).hit);
	REQUIRE(!cf_ray3_to_aabb3(ray, cf_make_aabb3_center(V3(0, 0, -10), V3(1, 1, 1))).hit);

	// Starting inside reports t = 0.
	CF_Raycast3 inside = cf_ray3_to_aabb3(cf_make_ray3(V3(0, 0, 0), V3(0, 0, 5)), box);
	REQUIRE(inside.hit && near_f(inside.t, 0.0f));

	// Sphere: hit distance and outward normal at the impact point.
	CF_Sphere sphere = cf_make_sphere(V3(0, 0, 0), 1.0f);
	hit = cf_ray3_to_sphere(cf_make_ray3(V3(0, 0, -5), V3(0, 0, 5)), sphere);
	REQUIRE(hit.hit);
	REQUIRE(near_f(hit.t, 4.0f));
	REQUIRE(near_v3(hit.n, V3(0, 0, -1)));
	REQUIRE(!cf_ray3_to_sphere(cf_make_ray3(V3(0, 3, -5), V3(0, 3, 5)), sphere).hit);
	// From inside: the exit point at t = 1 through the far surface.
	hit = cf_ray3_to_sphere(cf_make_ray3(V3(0, 0, 0), V3(0, 0, 5)), sphere);
	REQUIRE(hit.hit && near_f(hit.t, 1.0f));

	// Plane at y = 2: hit from above reports the up-facing normal; parallel rays miss.
	CF_Plane3 plane = cf_make_plane3(V3(0, 1, 0), 2.0f);
	hit = cf_ray3_to_plane3(cf_make_ray3(V3(0, 5, 0), V3(0, -1, 0)), plane);
	REQUIRE(hit.hit);
	REQUIRE(near_f(hit.t, 3.0f));
	REQUIRE(near_v3(hit.n, V3(0, 1, 0)));
	REQUIRE(!cf_ray3_to_plane3(cf_make_ray3(V3(0, 5, 0), V3(9, 5, 0)), plane).hit);
	REQUIRE(near_f(cf_distance_plane3(plane, V3(0, 5, 0)), 3.0f));
	REQUIRE(near_v3(cf_project_plane3(plane, V3(1, 5, 1)), V3(1, 2, 1)));
	return true;
}

// Overlap tests, closest points, bounds combining, and Arvo AABB transforms.
TEST_CASE(test_geometry_overlaps_cpp) {
	CF_Aabb3 a = cf_make_aabb3(V3(0, 0, 0), V3(2, 2, 2));
	CF_Aabb3 b = cf_make_aabb3(V3(1, 1, 1), V3(3, 3, 3));
	CF_Aabb3 c = cf_make_aabb3(V3(5, 5, 5), V3(6, 6, 6));
	REQUIRE(cf_aabb3_to_aabb3(a, b));
	REQUIRE(!cf_aabb3_to_aabb3(a, c));
	REQUIRE(cf_contains_point_aabb3(a, V3(1, 1, 1)));
	REQUIRE(!cf_contains_point_aabb3(a, V3(1, 1, 3)));
	CF_Aabb3 combined = cf_combine_aabb3(a, c);
	REQUIRE(near_v3(combined.min, V3(0, 0, 0)) && near_v3(combined.max, V3(6, 6, 6)));

	REQUIRE(cf_sphere_to_sphere(cf_make_sphere(V3(0, 0, 0), 1), cf_make_sphere(V3(1.5f, 0, 0), 1)));
	REQUIRE(!cf_sphere_to_sphere(cf_make_sphere(V3(0, 0, 0), 1), cf_make_sphere(V3(3, 0, 0), 1)));
	REQUIRE(cf_sphere_to_aabb3(cf_make_sphere(V3(3, 1, 1), 1.5f), a));
	REQUIRE(!cf_sphere_to_aabb3(cf_make_sphere(V3(4, 1, 1), 1.5f), a));
	REQUIRE(near_v3(cf_closest_point_on_aabb3(a, V3(5, 1, -1)), V3(2, 1, 0)));

	// A unit box rotated 45 degrees about y must bound to sqrt(2) on x/z, and translation
	// carries through.
	m4 rot = cf_quat_to_m4(cf_quat_from_axis_angle(V3(0, 1, 0), CF_PI / 4.0f));
	rot.elements[12] = 10.0f;
	CF_Aabb3 rotated = cf_transform_aabb3(rot, cf_make_aabb3_center(V3(0, 0, 0), V3(1, 1, 1)));
	float s = CF_SQRTF(2.0f);
	REQUIRE(near_f(rotated.min.x, 10.0f - s) && near_f(rotated.max.x, 10.0f + s));
	REQUIRE(near_f(rotated.min.y, -1.0f) && near_f(rotated.max.y, 1.0f));
	return true;
}

// Frustum extraction from the same matrices the 3d camera stacks consume, verified from
// first principles: points/boxes/spheres known to be inside or outside the camera's view.
TEST_CASE(test_geometry_frustum_cpp) {
	m4 proj = perspective(CF_PI / 2.0f, 1.0f, 1.0f, 100.0f); // 90 degree fov, square aspect.
	m4 view = look_at(V3(0, 0, 0), V3(0, 0, -1), V3(0, 1, 0)); // At origin, looking down -z.
	CF_Frustum f = cf_frustum_from_m4(proj * view);

	// Sphere checks: straight ahead is visible; behind, too near, too far, and far off to
	// the side are not. With a 90 degree fov the frustum boundary at depth 10 is |x| = 10.
	REQUIRE(cf_frustum_test_sphere(&f, cf_make_sphere(V3(0, 0, -10), 1)));
	REQUIRE(!cf_frustum_test_sphere(&f, cf_make_sphere(V3(0, 0, 10), 1)));    // Behind.
	REQUIRE(!cf_frustum_test_sphere(&f, cf_make_sphere(V3(0, 0, -0.2f), 0.1f))); // Nearer than znear.
	REQUIRE(!cf_frustum_test_sphere(&f, cf_make_sphere(V3(0, 0, -200), 1)));  // Past zfar.
	REQUIRE(!cf_frustum_test_sphere(&f, cf_make_sphere(V3(30, 0, -10), 1)));  // Off to the right.
	REQUIRE(cf_frustum_test_sphere(&f, cf_make_sphere(V3(9, 0, -10), 2)));    // Straddles the right plane.

	// AABB checks, including one straddling a plane (must never cull) and the p-vertex edge
	// case of a huge box surrounding the whole frustum.
	REQUIRE(cf_frustum_test_aabb3(&f, cf_make_aabb3_center(V3(0, 0, -50), V3(5, 5, 5))));
	REQUIRE(!cf_frustum_test_aabb3(&f, cf_make_aabb3_center(V3(0, 0, 50), V3(5, 5, 5))));
	REQUIRE(!cf_frustum_test_aabb3(&f, cf_make_aabb3_center(V3(-40, 0, -10), V3(2, 2, 2))));
	REQUIRE(cf_frustum_test_aabb3(&f, cf_make_aabb3_center(V3(10, 0, -10), V3(3, 3, 3))));
	REQUIRE(cf_frustum_test_aabb3(&f, cf_make_aabb3_center(V3(0, 0, 0), V3(1000, 1000, 1000))));

	// The camera position itself sits on the near-plane boundary side; nudge forward = inside.
	REQUIRE(cf_frustum_test_sphere(&f, cf_make_sphere(V3(0, 0, -2), 0.5f)));
	return true;
}

TEST_SUITE(test_math3d) {
	RUN_TEST_CASE(test_v3_overloads_cpp);
	RUN_TEST_CASE(test_v3_operators_cpp);
	RUN_TEST_CASE(test_m4_operators_cpp);
	RUN_TEST_CASE(test_quat_operators_cpp);
	RUN_TEST_CASE(test_helper_overloads_cpp);
	RUN_TEST_CASE(test_projection_cpp);
	RUN_TEST_CASE(test_geometry_raycasts_cpp);
	RUN_TEST_CASE(test_geometry_overlaps_cpp);
	RUN_TEST_CASE(test_geometry_frustum_cpp);
}
