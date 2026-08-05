/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Compiled as C on purpose: this is what proves the _Generic dispatch in cute_math3d.h works.
// The C++ overloads are covered separately by test_math3d.cpp, and a C++ build of these cases
// would silently exercise overload resolution instead.

#ifdef __cplusplus
#error "This code must be compiled as C, not C++"
#endif

#include "test_harness.h"

#include <cute_math3d.h>

static bool near_f(float a, float b) { return cf_abs_f(a - b) < 0.0001f; }
static bool near_v3(CF_V3 a, CF_V3 b) { return near_f(a.x, b.x) && near_f(a.y, b.y) && near_f(a.z, b.z); }

// --- Constructors and layout ---

TEST_CASE(test_v3_construct_c) {
	CF_V3 splat = cf_v3(2.0f);
	REQUIRE(near_v3(splat, cf_v3(2.0f, 2.0f, 2.0f)));

	CF_V3 a = cf_v3(1.0f, 2.0f, 3.0f);
	REQUIRE(a.x == 1.0f && a.y == 2.0f && a.z == 3.0f);

	CF_V4 b = cf_v4(1.0f, 2.0f, 3.0f, 4.0f);
	REQUIRE(b.x == 1.0f && b.y == 2.0f && b.z == 3.0f && b.w == 4.0f);
	REQUIRE(cf_v4(5.0f).w == 5.0f);

	CF_V4 c = cf_v4_from_v3(a, 1.0f);
	REQUIRE(c.x == 1.0f && c.w == 1.0f);
	REQUIRE(near_v3(cf_xyz(c), a));
	return true;
}

// CF_M4x4 is column-major so it can be handed to CF_UNIFORM_TYPE_MAT4 with no transpose.
// Translation must therefore live in elements[12..14], not [3],[7],[11].
TEST_CASE(test_m4_is_column_major_c) {
	CF_M4x4 m = cf_m4_translate(cf_v3(5.0f, 6.0f, 7.0f));
	REQUIRE(m.elements[12] == 5.0f);
	REQUIRE(m.elements[13] == 6.0f);
	REQUIRE(m.elements[14] == 7.0f);
	REQUIRE(m.elements[3] == 0.0f && m.elements[7] == 0.0f && m.elements[11] == 0.0f);
	REQUIRE(m.elements[15] == 1.0f);
	return true;
}

// --- _Generic dispatch: the 2d cases must keep working alongside the new 3d ones ---

TEST_CASE(test_generic_dispatch_c) {
	// 2d still dispatches correctly after cute_math3d.h redefines the selectors.
	CF_V2 a2 = cf_add(cf_v2(1.0f, 2.0f), cf_v2(3.0f, 4.0f));
	REQUIRE(a2.x == 4.0f && a2.y == 6.0f);
	REQUIRE(cf_dot(cf_v2(1.0f, 0.0f), cf_v2(1.0f, 0.0f)) == 1.0f);
	REQUIRE(near_f(cf_len(cf_v2(3.0f, 4.0f)), 5.0f));
	REQUIRE(cf_min(3, 5) == 3);
	REQUIRE(near_f(cf_min(3.0f, 5.0f), 3.0f));

	// 3d rows.
	CF_V3 a3 = cf_add(cf_v3(1.0f, 2.0f, 3.0f), cf_v3(4.0f, 5.0f, 6.0f));
	REQUIRE(near_v3(a3, cf_v3(5.0f, 7.0f, 9.0f)));
	REQUIRE(near_v3(cf_sub(a3, cf_v3(1.0f)), cf_v3(4.0f, 6.0f, 8.0f)));
	REQUIRE(near_v3(cf_neg(cf_v3(1.0f, -2.0f, 3.0f)), cf_v3(-1.0f, 2.0f, -3.0f)));
	REQUIRE(near_f(cf_dot(cf_v3(1.0f, 2.0f, 3.0f), cf_v3(4.0f, 5.0f, 6.0f)), 32.0f));
	REQUIRE(near_f(cf_len(cf_v3(2.0f, 3.0f, 6.0f)), 7.0f));
	REQUIRE(near_f(cf_len_sq(cf_v3(1.0f, 2.0f, 2.0f)), 9.0f));
	REQUIRE(near_f(cf_distance(cf_v3(0.0f), cf_v3(0.0f, 3.0f, 4.0f)), 5.0f));
	REQUIRE(near_v3(cf_min(cf_v3(1.0f, 5.0f, 3.0f), cf_v3(4.0f, 2.0f, 6.0f)), cf_v3(1.0f, 2.0f, 3.0f)));
	REQUIRE(near_v3(cf_max(cf_v3(1.0f, 5.0f, 3.0f), cf_v3(4.0f, 2.0f, 6.0f)), cf_v3(4.0f, 5.0f, 6.0f)));
	REQUIRE(near_v3(cf_abs(cf_v3(-1.0f, 2.0f, -3.0f)), cf_v3(1.0f, 2.0f, 3.0f)));
	REQUIRE(near_v3(cf_lerp(cf_v3(0.0f), cf_v3(10.0f), 0.5f), cf_v3(5.0f)));
	REQUIRE(near_v3(cf_norm(cf_v3(0.0f, 5.0f, 0.0f)), cf_v3(0.0f, 1.0f, 0.0f)));

	// cf_mul and cf_div dispatch on both arguments.
	REQUIRE(near_v3(cf_mul(cf_v3(1.0f, 2.0f, 3.0f), 2.0f), cf_v3(2.0f, 4.0f, 6.0f)));
	REQUIRE(near_v3(cf_mul(cf_v3(1.0f, 2.0f, 3.0f), cf_v3(2.0f)), cf_v3(2.0f, 4.0f, 6.0f)));
	REQUIRE(near_v3(cf_div(cf_v3(2.0f, 4.0f, 6.0f), 2.0f), cf_v3(1.0f, 2.0f, 3.0f)));

	// cf_cross keeps its 2d meanings and gains the 3d one.
	REQUIRE(near_f(cf_cross(cf_v2(1.0f, 0.0f), cf_v2(0.0f, 1.0f)), 1.0f));
	REQUIRE(near_v3(cf_cross(cf_v3(1.0f, 0.0f, 0.0f), cf_v3(0.0f, 1.0f, 0.0f)), cf_v3(0.0f, 0.0f, 1.0f)));
	return true;
}

TEST_CASE(test_safe_norm_zero_c) {
	REQUIRE(near_v3(cf_safe_norm(cf_v3(0.0f)), cf_v3(0.0f)));
	return true;
}

// --- Matrices ---

TEST_CASE(test_m4_mul_identity_c) {
	CF_M4x4 m = cf_mul(cf_m4_translate(cf_v3(1.0f, 2.0f, 3.0f)), cf_m4_rotate_y(0.7f));
	CF_M4x4 i = cf_m4_identity();
	CF_M4x4 a = cf_mul(m, i);
	CF_M4x4 b = cf_mul(i, m);
	for (int k = 0; k < 16; ++k) {
		REQUIRE(near_f(a.elements[k], m.elements[k]));
		REQUIRE(near_f(b.elements[k], m.elements[k]));
	}
	return true;
}

TEST_CASE(test_m4_invert_c) {
	CF_M4x4 m = cf_mul(cf_m4_translate(cf_v3(3.0f, -2.0f, 5.0f)),
	                   cf_mul(cf_m4_rotate_z(0.9f), cf_m4_scale(cf_v3(2.0f, 3.0f, 4.0f))));
	CF_M4x4 prod = cf_mul(m, cf_m4_invert(m));
	CF_M4x4 i = cf_m4_identity();
	for (int k = 0; k < 16; ++k) REQUIRE(near_f(prod.elements[k], i.elements[k]));

	// A singular matrix must not produce NaNs.
	CF_M4x4 singular = cf_m4_scale(cf_v3(0.0f));
	CF_M4x4 inv = cf_m4_invert(singular);
	for (int k = 0; k < 16; ++k) REQUIRE(near_f(inv.elements[k], i.elements[k]));
	return true;
}

// Matrix composition must be associative and must apply right-to-left, so that
// cf_mul(translate, rotate) rotates first and then translates.
TEST_CASE(test_m4_compose_order_c) {
	CF_M4x4 t = cf_m4_translate(cf_v3(10.0f, 0.0f, 0.0f));
	CF_M4x4 r = cf_m4_rotate_z(CF_PI * 0.5f);
	CF_V3 p = cf_m4_transform_point(cf_mul(t, r), cf_v3(1.0f, 0.0f, 0.0f));
	// Rotate (1,0,0) 90deg about z -> (0,1,0), then translate by +10x -> (10,1,0).
	REQUIRE(near_v3(p, cf_v3(10.0f, 1.0f, 0.0f)));

	CF_M4x4 s = cf_m4_scale(cf_v3(2.0f));
	CF_M4x4 lhs = cf_mul(cf_mul(t, r), s);
	CF_M4x4 rhs = cf_mul(t, cf_mul(r, s));
	for (int k = 0; k < 16; ++k) REQUIRE(near_f(lhs.elements[k], rhs.elements[k]));
	return true;
}

// Directions ignore translation; points do not.
TEST_CASE(test_m4_transform_point_vs_dir_c) {
	CF_M4x4 t = cf_m4_translate(cf_v3(5.0f, 5.0f, 5.0f));
	REQUIRE(near_v3(cf_m4_transform_point(t, cf_v3(1.0f, 0.0f, 0.0f)), cf_v3(6.0f, 5.0f, 5.0f)));
	REQUIRE(near_v3(cf_m4_transform_dir(t, cf_v3(1.0f, 0.0f, 0.0f)), cf_v3(1.0f, 0.0f, 0.0f)));
	return true;
}

// Non-uniform scale is exactly the case where transforming a normal by the model matrix
// is wrong; the inverse-transpose must keep it perpendicular to the surface.
TEST_CASE(test_m4_normal_matrix_c) {
	CF_M4x4 model = cf_m4_scale(cf_v3(2.0f, 1.0f, 1.0f));
	CF_M4x4 nm = cf_m4_normal_matrix(model);

	// A plane with in-surface tangent (1,1,0) has normal (1,-1,0) (normalized).
	CF_V3 tangent = cf_v3(1.0f, 1.0f, 0.0f);
	CF_V3 normal = cf_norm(cf_v3(1.0f, -1.0f, 0.0f));
	CF_V3 t2 = cf_m4_transform_dir(model, tangent);
	CF_V3 n2 = cf_norm(cf_m4_transform_dir(nm, normal));
	REQUIRE(near_f(cf_dot(t2, n2), 0.0f));

	// Naively using the model matrix does not stay perpendicular here.
	CF_V3 wrong = cf_norm(cf_m4_transform_dir(model, normal));
	REQUIRE(!near_f(cf_dot(t2, wrong), 0.0f));
	return true;
}

// --- Projection: the clip-convention regression test ---
//
// SDL_GPU normalizes clip-space z to [0, 1] on every backend. If cf_perspective or cf_ortho
// ever regress to the OpenGL [-1, 1] mapping, half the depth range lands outside the clip
// volume and geometry silently disappears. These two cases pin the convention down.

TEST_CASE(test_perspective_z_range_c) {
	float zn = 0.5f, zf = 100.0f;
	CF_M4x4 p = cf_perspective(CF_PI * 0.25f, 16.0f / 9.0f, zn, zf);

	// Right-handed: the camera looks down -z, so the near plane is at view-space z = -zn.
	CF_V4 at_near = cf_mul(p, cf_v4(0.0f, 0.0f, -zn, 1.0f));
	CF_V4 at_far = cf_mul(p, cf_v4(0.0f, 0.0f, -zf, 1.0f));
	REQUIRE(at_near.w > 0.0f && at_far.w > 0.0f);
	REQUIRE(near_f(at_near.z / at_near.w, 0.0f));
	REQUIRE(near_f(at_far.z / at_far.w, 1.0f));

	// A point between the planes must land strictly inside [0, 1] and increase with distance.
	CF_V4 mid = cf_mul(p, cf_v4(0.0f, 0.0f, -10.0f, 1.0f));
	float zmid = mid.z / mid.w;
	REQUIRE(zmid > 0.0f && zmid < 1.0f);
	return true;
}

TEST_CASE(test_ortho_z_range_c) {
	float zn = 1.0f, zf = 50.0f;
	CF_M4x4 p = cf_ortho(-10.0f, 10.0f, -10.0f, 10.0f, zn, zf);

	CF_V4 at_near = cf_mul(p, cf_v4(0.0f, 0.0f, -zn, 1.0f));
	CF_V4 at_far = cf_mul(p, cf_v4(0.0f, 0.0f, -zf, 1.0f));
	REQUIRE(near_f(at_near.z, 0.0f));
	REQUIRE(near_f(at_far.z, 1.0f));

	// x and y still map to [-1, 1].
	CF_V4 corner = cf_mul(p, cf_v4(10.0f, 10.0f, -zn, 1.0f));
	REQUIRE(near_f(corner.x, 1.0f) && near_f(corner.y, 1.0f));
	return true;
}

// Right-handed view space: things in front of the camera have NEGATIVE z, which is what
// cf_perspective/cf_ortho expect. A left-handed look_at here would render the scene inside-out.
TEST_CASE(test_look_at_handedness_c) {
	CF_M4x4 v = cf_look_at(cf_v3(0.0f, 0.0f, 5.0f), cf_v3(0.0f), cf_v3(0.0f, 1.0f, 0.0f));

	CF_V3 origin = cf_m4_transform_point(v, cf_v3(0.0f));
	REQUIRE(near_v3(origin, cf_v3(0.0f, 0.0f, -5.0f)));

	// Further from the camera means more negative z.
	CF_V3 farther = cf_m4_transform_point(v, cf_v3(0.0f, 0.0f, -5.0f));
	REQUIRE(farther.z < origin.z);

	// World +x stays view +x, world +y stays view +y for this camera.
	REQUIRE(near_v3(cf_m4_transform_dir(v, cf_v3(1.0f, 0.0f, 0.0f)), cf_v3(1.0f, 0.0f, 0.0f)));
	REQUIRE(near_v3(cf_m4_transform_dir(v, cf_v3(0.0f, 1.0f, 0.0f)), cf_v3(0.0f, 1.0f, 0.0f)));
	return true;
}

// The whole point of the convention: a near object must beat a far one under a LESS_THAN
// depth test with a 1.0 clear.
TEST_CASE(test_projection_depth_ordering_c) {
	CF_M4x4 p = cf_perspective(CF_PI * 0.25f, 1.0f, 0.1f, 1000.0f);
	CF_M4x4 v = cf_look_at(cf_v3(0.0f, 0.0f, 10.0f), cf_v3(0.0f), cf_v3(0.0f, 1.0f, 0.0f));
	CF_M4x4 vp = cf_mul(p, v);

	CF_V4 near_pt = cf_mul(vp, cf_v4(0.0f, 0.0f, 5.0f, 1.0f));   // closer to the camera at z=10
	CF_V4 far_pt = cf_mul(vp, cf_v4(0.0f, 0.0f, -5.0f, 1.0f));   // further away
	float zn = near_pt.z / near_pt.w;
	float zf = far_pt.z / far_pt.w;
	REQUIRE(zn >= 0.0f && zn <= 1.0f);
	REQUIRE(zf >= 0.0f && zf <= 1.0f);
	REQUIRE(zn < zf);
	return true;
}

// --- Quaternions ---

TEST_CASE(test_quat_rotation_c) {
	CF_Quat q = cf_quat_from_axis_angle(cf_v3(0.0f, 0.0f, 1.0f), CF_PI * 0.5f);
	REQUIRE(near_v3(cf_mul(q, cf_v3(1.0f, 0.0f, 0.0f)), cf_v3(0.0f, 1.0f, 0.0f)));

	// Rotating by a quaternion and by its matrix must agree.
	CF_M4x4 m = cf_quat_to_m4(q);
	CF_V3 p = cf_v3(0.3f, -0.7f, 1.1f);
	REQUIRE(near_v3(cf_mul(q, p), cf_m4_transform_dir(m, p)));

	// Composition matches matrix composition.
	CF_Quat r = cf_quat_from_axis_angle(cf_norm(cf_v3(1.0f, 1.0f, 0.0f)), 0.6f);
	CF_Quat qr = cf_mul(q, r);
	CF_V3 via_quat = cf_mul(qr, p);
	CF_V3 via_mats = cf_m4_transform_dir(cf_mul(cf_quat_to_m4(q), cf_quat_to_m4(r)), p);
	REQUIRE(near_v3(via_quat, via_mats));

	// Conjugate undoes the rotation.
	REQUIRE(near_v3(cf_mul(cf_mul(q, cf_quat_conjugate(q)), p), p));
	return true;
}

TEST_CASE(test_quat_slerp_c) {
	CF_Quat a = cf_quat_identity();
	CF_Quat b = cf_quat_from_axis_angle(cf_v3(0.0f, 1.0f, 0.0f), CF_PI * 0.5f);

	REQUIRE(near_v3(cf_mul(cf_quat_slerp(a, b, 0.0f), cf_v3(1.0f, 0.0f, 0.0f)), cf_v3(1.0f, 0.0f, 0.0f)));
	REQUIRE(near_v3(cf_mul(cf_quat_slerp(a, b, 1.0f), cf_v3(1.0f, 0.0f, 0.0f)), cf_mul(b, cf_v3(1.0f, 0.0f, 0.0f))));

	// Halfway is a 45deg rotation about y: (1,0,0) -> (cos45, 0, -sin45).
	CF_V3 half = cf_mul(cf_quat_slerp(a, b, 0.5f), cf_v3(1.0f, 0.0f, 0.0f));
	REQUIRE(near_v3(half, cf_v3(0.7071068f, 0.0f, -0.7071068f)));

	// Identical endpoints take the near-parallel branch and must not divide by zero.
	REQUIRE(near_v3(cf_mul(cf_quat_slerp(b, b, 0.5f), cf_v3(1.0f, 0.0f, 0.0f)), cf_mul(b, cf_v3(1.0f, 0.0f, 0.0f))));
	return true;
}

// --- Flat-expansion macros must agree with the functions ---

TEST_CASE(test_mul_macros_c) {
	CF_M4x4 a = cf_mul(cf_m4_translate(cf_v3(1.0f, 2.0f, 3.0f)), cf_m4_rotate_x(0.4f));
	CF_M4x4 b = cf_m4_scale(cf_v3(2.0f, 3.0f, 4.0f));

	CF_M4x4 macro_m;
	CF_MUL_M4_M4(macro_m, a, b);
	CF_M4x4 fn_m = cf_mul(a, b);
	for (int k = 0; k < 16; ++k) REQUIRE(near_f(macro_m.elements[k], fn_m.elements[k]));

	CF_V4 v = cf_v4(1.0f, -2.0f, 3.0f, 1.0f);
	CF_V4 macro_v;
	CF_MUL_M4_V4(macro_v, a, v);
	CF_V4 fn_v = cf_mul(a, v);
	REQUIRE(near_f(macro_v.x, fn_v.x) && near_f(macro_v.y, fn_v.y));
	REQUIRE(near_f(macro_v.z, fn_v.z) && near_f(macro_v.w, fn_v.w));
	return true;
}

// cf_quat_from_to: the aiming primitive. Must produce the shortest arc, survive parallel and
// exactly-opposed inputs, and not care about input length.
TEST_CASE(test_quat_from_to_c) {
	CF_V3 x = cf_v3(1, 0, 0), y = cf_v3(0, 1, 0), z = cf_v3(0, 0, 1);

	// Rotating x onto y actually lands on y, and takes z with it (shortest arc about +z).
	CF_Quat q = cf_quat_from_to(x, y);
	REQUIRE(near_v3(cf_mul(q, x), y));
	REQUIRE(near_v3(cf_mul(q, z), z));

	// Unnormalized inputs give the same answer.
	REQUIRE(near_v3(cf_mul(cf_quat_from_to(cf_mul(x, 7.0f), cf_mul(y, 0.25f)), x), y));

	// Parallel: identity, no NaNs.
	REQUIRE(near_v3(cf_mul(cf_quat_from_to(y, y), x), x));

	// Opposed: any 180-degree turn is valid, so assert the outcome, not the axis.
	CF_Quat flip = cf_quat_from_to(x, cf_mul(x, -1.0f));
	REQUIRE(near_v3(cf_mul(flip, x), cf_v3(-1, 0, 0)));
	CF_Quat flip_z = cf_quat_from_to(z, cf_mul(z, -1.0f));
	REQUIRE(near_v3(cf_mul(flip_z, z), cf_v3(0, 0, -1)));
	return true;
}

// cf_quat_from_m4 and cf_m4_decompose invert matrix composition.
TEST_CASE(test_m4_decompose_c) {
	CF_V3 t = cf_v3(3, -4, 5);
	CF_Quat r = cf_quat_from_axis_angle(cf_norm(cf_v3(1, 2, 3)), 1.1f);
	CF_V3 s = cf_v3(2, 3, 4);
	CF_M4x4 m = cf_mul(cf_mul(cf_m4_translate(t), cf_quat_to_m4(r)), cf_m4_scale(s));

	CF_V3 dt, ds;
	CF_Quat dr;
	cf_m4_decompose(m, &dt, &dr, &ds);
	REQUIRE(near_v3(dt, t));
	REQUIRE(near_v3(ds, s));
	// Quaternions double-cover rotations, so compare what they do, not their components.
	REQUIRE(near_v3(cf_mul(dr, cf_v3(1, 0, 0)), cf_mul(r, cf_v3(1, 0, 0))));
	REQUIRE(near_v3(cf_mul(dr, cf_v3(0, 0, 1)), cf_mul(r, cf_v3(0, 0, 1))));

	// Recomposing reproduces the original matrix.
	CF_M4x4 back = cf_mul(cf_mul(cf_m4_translate(dt), cf_quat_to_m4(dr)), cf_m4_scale(ds));
	for (int k = 0; k < 16; ++k) REQUIRE(near_f(back.elements[k], m.elements[k]));

	// Rotation-only extraction ignores scale.
	CF_Quat only = cf_quat_from_m4(m);
	REQUIRE(near_v3(cf_mul(only, cf_v3(0, 1, 0)), cf_mul(r, cf_v3(0, 1, 0))));

	// NULL outputs are allowed.
	cf_m4_decompose(m, NULL, NULL, NULL);

	// A mirrored transform reports the flip in x rather than corrupting the rotation.
	CF_M4x4 mirrored = cf_mul(cf_quat_to_m4(r), cf_m4_scale(cf_v3(-1, 1, 1)));
	cf_m4_decompose(mirrored, NULL, NULL, &ds);
	REQUIRE(ds.x < 0);
	return true;
}

// Reflect, project, and slide: reflect preserves length, and project + slide must
// reassemble the original vector (they are complementary components).
TEST_CASE(test_v3_reflect_project_slide_c) {
	CF_V3 n = cf_v3(0, 1, 0);
	CF_V3 v = cf_v3(1, -1, 0);
	REQUIRE(near_v3(cf_reflect_v3(v, n), cf_v3(1, 1, 0)));
	REQUIRE(near_f(cf_len_v3(cf_reflect_v3(cf_v3(0.3f, -0.7f, 1.1f), n)), cf_len_v3(cf_v3(0.3f, -0.7f, 1.1f))));

	// Sliding along a flat floor keeps the horizontal motion and drops the vertical.
	REQUIRE(near_v3(cf_slide_v3(v, n), cf_v3(1, 0, 0)));

	// Project onto an unnormalized direction still lands on that direction.
	CF_V3 p = cf_project_v3(cf_v3(2, 2, 0), cf_v3(10.0f, 0, 0));
	REQUIRE(near_v3(p, cf_v3(2, 0, 0)));
	REQUIRE(near_v3(cf_project_v3(v, cf_v3(0.0f)), cf_v3(0.0f)));

	// project + slide = original.
	CF_V3 arbitrary = cf_v3(0.3f, -0.7f, 1.1f);
	CF_V3 axis = cf_norm(cf_v3(1, 2, -3));
	REQUIRE(near_v3(cf_add(cf_project_v3(arbitrary, axis), cf_slide_v3(arbitrary, axis)), arbitrary));
	return true;
}

// cf_orthonormal_basis: (x, y, n) must be right-handed and orthonormal for any unit n,
// including the branch-hazard normals straight up and straight down.
TEST_CASE(test_orthonormal_basis_c) {
	CF_V3 normals[] = {
		{ 0, 0, 1 }, { 0, 0, -1 }, { 1, 0, 0 }, { 0, -1, 0 },
		{ 0.5345225f, 0.2672612f, -0.8017837f },
	};
	for (int i = 0; i < 5; ++i) {
		CF_V3 n = normals[i];
		CF_V3 x, y;
		cf_orthonormal_basis(n, &x, &y);
		REQUIRE(near_f(cf_len_v3(x), 1.0f));
		REQUIRE(near_f(cf_len_v3(y), 1.0f));
		REQUIRE(near_f(cf_dot(x, y), 0.0f));
		REQUIRE(near_f(cf_dot(x, n), 0.0f));
		REQUIRE(near_f(cf_dot(y, n), 0.0f));
		REQUIRE(near_v3(cf_cross(x, y), n));
	}
	return true;
}

// cf_quat_inverse must undo a rotation even for non-unit quaternions, where the conjugate alone
// is off by the squared length.
TEST_CASE(test_quat_inverse_c) {
	CF_Quat q = cf_quat_from_axis_angle(cf_norm(cf_v3(1, -2, 0.5f)), 0.9f);
	CF_V3 p = cf_v3(0.3f, -0.7f, 1.1f);
	REQUIRE(near_v3(cf_mul(cf_mul_q(q, cf_quat_inverse(q)), p), p));

	// Scale the quaternion: rotation is unchanged, and the inverse must still cancel exactly.
	CF_Quat big = cf_quat(q.x * 3.0f, q.y * 3.0f, q.z * 3.0f, q.w * 3.0f);
	CF_Quat prod = cf_mul_q(big, cf_quat_inverse(big));
	REQUIRE(near_f(prod.x, 0.0f) && near_f(prod.y, 0.0f) && near_f(prod.z, 0.0f) && near_f(prod.w, 1.0f));

	// Zero quaternion has no inverse; identity beats NaNs.
	CF_Quat zero_inv = cf_quat_inverse(cf_quat(0, 0, 0, 0));
	REQUIRE(near_f(zero_inv.w, 1.0f));
	return true;
}

// cf_quat_nlerp shares slerp's endpoints and its shortest-arc choice; only the pacing differs.
TEST_CASE(test_quat_nlerp_c) {
	CF_Quat a = cf_quat_identity();
	CF_Quat b = cf_quat_from_axis_angle(cf_v3(0, 1, 0), CF_PI * 0.5f);
	CF_V3 x = cf_v3(1, 0, 0);

	REQUIRE(near_v3(cf_mul(cf_quat_nlerp(a, b, 0.0f), x), x));
	REQUIRE(near_v3(cf_mul(cf_quat_nlerp(a, b, 1.0f), x), cf_mul(b, x)));

	// Halfway between identity and a 90deg turn is exactly 45deg for nlerp too (symmetry).
	REQUIRE(near_v3(cf_mul(cf_quat_nlerp(a, b, 0.5f), x), cf_v3(0.7071068f, 0.0f, -0.7071068f)));

	// The double-cover: nlerp against -b must take the same short arc, not spin the long way.
	CF_Quat neg_b = cf_quat(-b.x, -b.y, -b.z, -b.w);
	REQUIRE(near_v3(cf_mul(cf_quat_nlerp(a, neg_b, 0.5f), x), cf_mul(cf_quat_nlerp(a, b, 0.5f), x)));
	return true;
}

// cf_quat_to_axis_angle inverts cf_quat_from_axis_angle, and survives the identity.
TEST_CASE(test_quat_to_axis_angle_c) {
	CF_V3 axis_in = cf_norm(cf_v3(1, 2, -1));
	float angle_in = 1.3f;
	CF_V3 axis;
	float angle;
	cf_quat_to_axis_angle(cf_quat_from_axis_angle(axis_in, angle_in), &axis, &angle);
	REQUIRE(near_v3(axis, axis_in));
	REQUIRE(near_f(angle, angle_in));

	cf_quat_to_axis_angle(cf_quat_identity(), &axis, &angle);
	REQUIRE(near_f(angle, 0.0f));
	REQUIRE(near_f(cf_len_v3(axis), 1.0f));

	// NULL outputs are allowed.
	cf_quat_to_axis_angle(cf_quat_from_axis_angle(axis_in, angle_in), NULL, NULL);
	return true;
}

// cf_m4_from_trs must match composing the three matrices by hand, and round-trip
// through cf_m4_decompose.
TEST_CASE(test_m4_from_trs_c) {
	CF_V3 t = cf_v3(3, -4, 5);
	CF_Quat r = cf_quat_from_axis_angle(cf_norm(cf_v3(1, 2, 3)), 1.1f);
	CF_V3 s = cf_v3(2, 3, 4);

	CF_M4x4 direct = cf_m4_from_trs(t, r, s);
	CF_M4x4 composed = cf_mul(cf_mul(cf_m4_translate(t), cf_quat_to_m4(r)), cf_m4_scale(s));
	for (int k = 0; k < 16; ++k) REQUIRE(near_f(direct.elements[k], composed.elements[k]));

	CF_V3 dt, ds;
	CF_Quat dr;
	cf_m4_decompose(direct, &dt, &dr, &ds);
	REQUIRE(near_v3(dt, t));
	REQUIRE(near_v3(ds, s));
	REQUIRE(near_v3(cf_mul(dr, cf_v3(1, 0, 0)), cf_mul(r, cf_v3(1, 0, 0))));
	return true;
}

// cf_quat_from_basis round-trips against cf_quat_to_m4's columns.
TEST_CASE(test_quat_from_basis_c) {
	CF_Quat r = cf_quat_from_axis_angle(cf_norm(cf_v3(-2, 1, 0.5f)), 2.3f);
	CF_M4x4 m = cf_quat_to_m4(r);
	CF_Quat back = cf_quat_from_basis(
		cf_v3(m.elements[0], m.elements[1], m.elements[2]),
		cf_v3(m.elements[4], m.elements[5], m.elements[6]),
		cf_v3(m.elements[8], m.elements[9], m.elements[10]));
	REQUIRE(near_v3(cf_mul(back, cf_v3(1, 0, 0)), cf_mul(r, cf_v3(1, 0, 0))));
	REQUIRE(near_v3(cf_mul(back, cf_v3(0, 1, 0)), cf_mul(r, cf_v3(0, 1, 0))));
	REQUIRE(near_v3(cf_mul(back, cf_v3(0, 0, 1)), cf_mul(r, cf_v3(0, 0, 1))));
	return true;
}

/* The 3d stateless collision kit (routed through Box3D's geometry layer): booleans,
   manifold direction/depth, raycasts, gjk closest points, and toi. */
TEST_CASE(test_collision3_booleans_c) {
	CF_Sphere s = cf_make_sphere(cf_v3(0.0f), 1.0f);
	CF_Capsule3 cap = cf_make_capsule3(cf_v3(2.0f, -1.0f, 0.0f), cf_v3(2.0f, 1.0f, 0.0f), 0.5f);
	REQUIRE(!cf_sphere_to_capsule3(s, cap));
	REQUIRE(cf_sphere_to_capsule3(cf_make_sphere(cf_v3(1.2f, 0.0f, 0.0f), 1.0f), cap));

	REQUIRE(cf_capsule3_to_capsule3(cap, cf_make_capsule3(cf_v3(2.8f, -1.0f, 0.0f), cf_v3(2.8f, 1.0f, 0.0f), 0.5f)));
	REQUIRE(!cf_capsule3_to_capsule3(cap, cf_make_capsule3(cf_v3(4.0f, -1.0f, 0.0f), cf_v3(4.0f, 1.0f, 0.0f), 0.5f)));

	CF_Aabb3 box = cf_make_aabb3_center(cf_v3(0.0f), cf_v3(1.0f, 1.0f, 1.0f));
	REQUIRE(cf_aabb3_to_capsule3(box, cf_make_capsule3(cf_v3(1.2f, -1.0f, 0.0f), cf_v3(1.2f, 1.0f, 0.0f), 0.5f)));
	REQUIRE(!cf_aabb3_to_capsule3(box, cf_make_capsule3(cf_v3(3.0f, -1.0f, 0.0f), cf_v3(3.0f, 1.0f, 0.0f), 0.5f)));

	/* Triangle in the xz plane, counter-clockwise from +y (face normal +y). */
	CF_Triangle3 tri = cf_make_triangle3(cf_v3(-2, 0, -2), cf_v3(-2, 0, 2), cf_v3(2, 0, 0));
	REQUIRE(cf_sphere_to_triangle3(cf_make_sphere(cf_v3(-1.0f, 0.8f, 0.0f), 1.0f), tri));
	REQUIRE(!cf_sphere_to_triangle3(cf_make_sphere(cf_v3(-1.0f, 3.0f, 0.0f), 1.0f), tri));
	REQUIRE(cf_capsule3_to_triangle3(cf_make_capsule3(cf_v3(-1, 0.8f, 0), cf_v3(-1, 2.8f, 0), 1.0f), tri));
	REQUIRE(cf_aabb3_to_triangle3(cf_make_aabb3_center(cf_v3(-1, 0.5f, 0), cf_v3(1, 1, 1)), tri));
	REQUIRE(!cf_aabb3_to_triangle3(cf_make_aabb3_center(cf_v3(-1, 5, 0), cf_v3(1, 1, 1)), tri));
	return true;
}

TEST_CASE(test_collision3_manifolds_c) {
	/* Two overlapping spheres: one point, normal A to B, depth = overlap. */
	CF_Sphere a = cf_make_sphere(cf_v3(0.0f), 1.0f);
	CF_Sphere b = cf_make_sphere(cf_v3(1.5f, 0, 0), 1.0f);
	CF_Manifold3 m = cf_sphere_to_sphere_manifold(a, b);
	REQUIRE(m.count == 1);
	REQUIRE(cf_abs(m.n.x - 1.0f) < 1e-3f && cf_abs(m.n.y) < 1e-3f && cf_abs(m.n.z) < 1e-3f);
	REQUIRE(cf_abs(m.depths[0] - 0.5f) < 1e-3f);

	/* Swapped order flips the normal: still A to B. */
	m = cf_sphere_to_sphere_manifold(b, a);
	REQUIRE(m.count == 1);
	REQUIRE(cf_abs(m.n.x + 1.0f) < 1e-3f);

	/* Separated spheres: empty manifold (speculative points must not leak through). */
	m = cf_sphere_to_sphere_manifold(a, cf_make_sphere(cf_v3(2.5f, 0, 0), 1.0f));
	REQUIRE(m.count == 0);

	/* Boxes overlapping along +x: contact points with depth = overlap. */
	CF_Aabb3 box_a = cf_make_aabb3_center(cf_v3(0.0f), cf_v3(1, 1, 1));
	CF_Aabb3 box_b = cf_make_aabb3_center(cf_v3(1.5f, 0, 0), cf_v3(1, 1, 1));
	m = cf_aabb3_to_aabb3_manifold(box_a, box_b);
	REQUIRE(m.count > 0);
	REQUIRE(cf_abs(m.n.x - 1.0f) < 1e-2f);
	for (int i = 0; i < m.count; ++i) REQUIRE(m.depths[i] >= 0);
	REQUIRE(cf_abs(m.depths[0] - 0.5f) < 1e-2f);

	/* Sphere resting into a +y-facing triangle: normal from the sphere toward the
	   triangle (A to B) is -y, depth is the penetration. */
	CF_Triangle3 tri = cf_make_triangle3(cf_v3(-2, 0, -2), cf_v3(-2, 0, 2), cf_v3(2, 0, 0));
	CF_Sphere s = cf_make_sphere(cf_v3(-1.0f, 0.8f, 0.0f), 1.0f);
	m = cf_sphere_to_triangle3_manifold(s, tri);
	REQUIRE(m.count == 1);
	REQUIRE(cf_abs(m.n.y + 1.0f) < 1e-2f);
	REQUIRE(cf_abs(m.depths[0] - 0.2f) < 1e-2f);

	/* Generic dispatch agrees with the typed pair. */
	CF_Manifold3 generic;
	cf_collide3(&s, CF_SHAPE3_TYPE_SPHERE, &tri, CF_SHAPE3_TYPE_TRIANGLE, &generic);
	REQUIRE(generic.count == m.count);
	REQUIRE(cf_abs(generic.n.y - m.n.y) < 1e-4f);
	REQUIRE(cf_collided3(&s, CF_SHAPE3_TYPE_SPHERE, &tri, CF_SHAPE3_TYPE_TRIANGLE));
	return true;
}

TEST_CASE(test_collision3_raycasts_c) {
	/* Vertical capsule at x=5: a +x ray hits its side at t = 5 - r, normal -x. */
	CF_Capsule3 cap = cf_make_capsule3(cf_v3(5, -1, 0), cf_v3(5, 1, 0), 0.5f);
	CF_Ray3 ray = cf_make_ray3(cf_v3(0.0f), cf_v3(100, 0, 0));
	CF_Raycast3 rc = cf_ray3_to_capsule3(ray, cap);
	REQUIRE(rc.hit);
	REQUIRE(cf_abs(rc.t - 4.5f) < 1e-3f);
	REQUIRE(cf_abs(rc.n.x + 1.0f) < 1e-3f);

	/* Miss cases stay zeroed, like every other cf_ray3_* cast. */
	rc = cf_ray3_to_capsule3(cf_make_ray3(cf_v3(0, 5, 0), cf_v3(100, 5, 0)), cap);
	REQUIRE(!rc.hit && rc.t == 0 && rc.n.x == 0);

	/* Ray straight down onto a triangle: t is the distance, normal faces the ray. */
	CF_Triangle3 tri = cf_make_triangle3(cf_v3(-2, 0, -2), cf_v3(-2, 0, 2), cf_v3(2, 0, 0));
	rc = cf_ray3_to_triangle3(cf_make_ray3(cf_v3(-1, 5, 0), cf_v3(-1, -95, 0)), tri);
	REQUIRE(rc.hit);
	REQUIRE(cf_abs(rc.t - 5.0f) < 1e-3f);
	REQUIRE(cf_abs(rc.n.y - 1.0f) < 1e-3f);

	/* Two-sided: from below it also hits, normal flipped to face the ray. */
	rc = cf_ray3_to_triangle3(cf_make_ray3(cf_v3(-1, -5, 0), cf_v3(-1, 95, 0)), tri);
	REQUIRE(rc.hit);
	REQUIRE(cf_abs(rc.n.y + 1.0f) < 1e-3f);

	/* Outside the triangle: miss. A short ray: miss. */
	REQUIRE(!cf_ray3_to_triangle3(cf_make_ray3(cf_v3(5, 5, 5), cf_v3(5, -95, 5)), tri).hit);
	REQUIRE(!cf_ray3_to_triangle3(cf_make_ray3(cf_v3(-1, 5, 0), cf_v3(-1, 2, 0)), tri).hit);

	/* Generic entry point. */
	CF_Raycast3 out;
	REQUIRE(cf_cast_ray3(ray, &cap, CF_SHAPE3_TYPE_CAPSULE, &out));
	REQUIRE(cf_abs(out.t - 4.5f) < 1e-3f);
	return true;
}

TEST_CASE(test_collision3_gjk_toi_c) {
	/* Distance between two spheres with radii on: the surface gap, closest points on the
	   line between centers. */
	CF_Sphere a = cf_make_sphere(cf_v3(0.0f), 1.0f);
	CF_Sphere b = cf_make_sphere(cf_v3(5, 0, 0), 1.0f);
	CF_V3 pa, pb;
	float d = cf_gjk3(&a, CF_SHAPE3_TYPE_SPHERE, &b, CF_SHAPE3_TYPE_SPHERE, &pa, &pb, true, NULL, NULL);
	REQUIRE(cf_abs(d - 3.0f) < 1e-3f);
	REQUIRE(cf_abs(pa.x - 1.0f) < 1e-3f);
	REQUIRE(cf_abs(pb.x - 4.0f) < 1e-3f);

	/* Radii off: center-to-center. */
	d = cf_gjk3(&a, CF_SHAPE3_TYPE_SPHERE, &b, CF_SHAPE3_TYPE_SPHERE, NULL, NULL, false, NULL, NULL);
	REQUIRE(cf_abs(d - 5.0f) < 1e-3f);

	/* Warm cache accepted, same answer both calls. */
	CF_GjkCache3 cache = { 0 };
	d = cf_gjk3(&a, CF_SHAPE3_TYPE_SPHERE, &b, CF_SHAPE3_TYPE_SPHERE, &pa, &pb, true, NULL, &cache);
	REQUIRE(cf_abs(d - 3.0f) < 1e-3f);
	d = cf_gjk3(&a, CF_SHAPE3_TYPE_SPHERE, &b, CF_SHAPE3_TYPE_SPHERE, &pa, &pb, true, NULL, &cache);
	REQUIRE(cf_abs(d - 3.0f) < 1e-3f);

	/* Sphere-vs-aabb distance: gap from surface to face. */
	CF_Aabb3 box = cf_make_aabb3_center(cf_v3(5, 0, 0), cf_v3(1, 1, 1));
	d = cf_gjk3(&a, CF_SHAPE3_TYPE_SPHERE, &box, CF_SHAPE3_TYPE_AABB, &pa, &pb, true, NULL, NULL);
	REQUIRE(cf_abs(d - 3.0f) < 1e-3f);

	/* Time of impact: b closes the 3-unit gap moving 6 left -> toi 0.5, normal A to B. */
	CF_ToiResult3 toi = cf_toi3(&a, CF_SHAPE3_TYPE_SPHERE, cf_v3(0.0f), &b, CF_SHAPE3_TYPE_SPHERE, cf_v3(-6, 0, 0), true);
	REQUIRE(toi.hit);
	REQUIRE(cf_abs(toi.toi - 0.5f) < 1e-2f);
	REQUIRE(cf_abs(toi.n.x - 1.0f) < 1e-2f);

	/* Moving apart: miss, toi 1. */
	toi = cf_toi3(&a, CF_SHAPE3_TYPE_SPHERE, cf_v3(0.0f), &b, CF_SHAPE3_TYPE_SPHERE, cf_v3(6, 0, 0), true);
	REQUIRE(!toi.hit);
	REQUIRE(toi.toi == 1.0f);
	return true;
}

TEST_SUITE(test_math3d_c) {
	RUN_TEST_CASE(test_v3_construct_c);
	RUN_TEST_CASE(test_m4_is_column_major_c);
	RUN_TEST_CASE(test_generic_dispatch_c);
	RUN_TEST_CASE(test_safe_norm_zero_c);
	RUN_TEST_CASE(test_m4_mul_identity_c);
	RUN_TEST_CASE(test_m4_invert_c);
	RUN_TEST_CASE(test_m4_compose_order_c);
	RUN_TEST_CASE(test_m4_transform_point_vs_dir_c);
	RUN_TEST_CASE(test_m4_normal_matrix_c);
	RUN_TEST_CASE(test_perspective_z_range_c);
	RUN_TEST_CASE(test_ortho_z_range_c);
	RUN_TEST_CASE(test_look_at_handedness_c);
	RUN_TEST_CASE(test_projection_depth_ordering_c);
	RUN_TEST_CASE(test_quat_rotation_c);
	RUN_TEST_CASE(test_quat_slerp_c);
	RUN_TEST_CASE(test_mul_macros_c);
	RUN_TEST_CASE(test_quat_from_to_c);
	RUN_TEST_CASE(test_m4_decompose_c);
	RUN_TEST_CASE(test_v3_reflect_project_slide_c);
	RUN_TEST_CASE(test_orthonormal_basis_c);
	RUN_TEST_CASE(test_quat_inverse_c);
	RUN_TEST_CASE(test_quat_nlerp_c);
	RUN_TEST_CASE(test_quat_to_axis_angle_c);
	RUN_TEST_CASE(test_m4_from_trs_c);
	RUN_TEST_CASE(test_quat_from_basis_c);
	RUN_TEST_CASE(test_collision3_booleans_c);
	RUN_TEST_CASE(test_collision3_manifolds_c);
	RUN_TEST_CASE(test_collision3_raycasts_c);
	RUN_TEST_CASE(test_collision3_gjk_toi_c);
}
