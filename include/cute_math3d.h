/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_MATH3D_H
#define CF_MATH3D_H

#include "cute_math.h"

#ifndef CF_TANF
	#include <math.h>
	#define CF_TANF tanf
#endif

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_V3
 * @category math
 * @brief    A 3d vector.
 * @remarks  Construct one with `cf_v3`, which splats a single argument or takes all three:
 *
 *           ```c
 *           CF_V3 a = cf_v3(1.0f);            // { 1, 1, 1 }
 *           CF_V3 b = cf_v3(1.0f, 2.0f, 3.0f);
 *           ```
 *
 *           The C++ API uses `V3(x, y, z)`. Most operators (`cf_add`, `cf_dot`, `cf_len`, ...)
 *           are shared with `CF_V2` and dispatch on the argument type.
 * @related  CF_V3 CF_V4 CF_M4x4 cf_v3 cf_dot cf_cross cf_len cf_norm
 */
typedef struct CF_V3
{
	/* @member The x component. */
	float x;

	/* @member The y component. */
	float y;

	/* @member The z component. */
	float z;
} CF_V3;
// @end

/**
 * @struct   CF_V4
 * @category math
 * @brief    A 4d vector, typically a homogeneous point or a color.
 * @remarks  Construct one with `cf_v4`, which splats a single argument or takes all four:
 *
 *           ```c
 *           CF_V4 a = cf_v4(0.0f);                        // { 0, 0, 0, 0 }
 *           CF_V4 b = cf_v4(1.0f, 2.0f, 3.0f, 1.0f);
 *           ```
 *
 *           The C++ API uses `V4(x, y, z, w)`. Use `cf_v4_from_v3` to extend a `CF_V3` with an
 *           explicit w, and `cf_xyz` to truncate back down.
 * @related  CF_V3 CF_V4 CF_M4x4 cf_v4 cf_v4_from_v3 cf_mul cf_dot
 */
typedef struct CF_V4
{
	/* @member The x component. */
	float x;

	/* @member The y component. */
	float y;

	/* @member The z component. */
	float z;

	/* @member The w component. */
	float w;
} CF_V4;
// @end

/**
 * @struct   CF_Quat
 * @category math
 * @brief    A quaternion, representing a rotation in 3d.
 * @remarks  Construct one component-wise with `cf_quat(x, y, z, w)`, or from an axis and angle with
 *           `cf_quat_from_axis_angle`.
 *
 *           Stored as the vector part `(x, y, z)` plus the scalar part `w`. Quaternions passed to
 *           `cf_quat_to_m4` or used to rotate a vector are assumed to be unit length; normalize with
 *           `cf_quat_norm` after accumulating many multiplications.
 * @related  CF_Quat cf_quat cf_quat_identity cf_quat_from_axis_angle cf_quat_to_m4 cf_quat_slerp
 */
typedef struct CF_Quat
{
	/* @member The x component of the vector part. */
	float x;

	/* @member The y component of the vector part. */
	float y;

	/* @member The z component of the vector part. */
	float z;

	/* @member The scalar part. */
	float w;
} CF_Quat;
// @end

/**
 * @struct   CF_M4x4
 * @category math
 * @brief    A 4x4 matrix, stored **column-major**.
 * @remarks  Element `(row, col)` lives at `elements[col * 4 + row]`, matching GLSL/MSL's default
 *           `mat4` layout. This means a `CF_M4x4` can be handed straight to
 *           `cf_material_set_uniform_vs` as `CF_UNIFORM_TYPE_MAT4` with no transpose:
 *
 *           ```c
 *           CF_M4x4 mvp = cf_mul(projection, cf_mul(view, model));
 *           cf_material_set_uniform_vs(material, "u_mvp", &mvp, CF_UNIFORM_TYPE_MAT4, 1);
 *           ```
 *
 *           Note that Metal only supports `mat4` inside a uniform block -- a `mat3` normal matrix
 *           must be widened to a `CF_M4x4` before upload.
 * @related  CF_M4x4 cf_m4_identity cf_mul cf_perspective cf_ortho cf_look_at CF_UniformType
 */
typedef struct CF_M4x4
{
	/* @member The 16 elements, in column-major order. `(row, col)` is at `elements[col * 4 + row]`. */
	float elements[16];
} CF_M4x4;
// @end

// cf_v3 is documented in the CF_V3 struct's remarks rather than as its own documented symbol: the
// docs parser lowercases titles into filenames, so a `cf_v3` page would collide with `CF_V3`'s.
// cute_math.h leaves cf_v2 undocumented for the same reason. Note that tools/docs_parser.c scans
// every token in the file, so a doc tag written in an ordinary comment aborts the docs build.
#define cf_v3(...)
#undef cf_v3
// Implemented this way (rather than as a function) to force-inline the initializer even in
// unoptimized debug builds, exactly as `cf_v2` does. Expands to a braced initializer in C++
// and a compound literal in C, and supports splatting: cf_v3(a) == { a, a, a }.
#define _CF_V3_SELECT(_1, _2, _3, NAME, ...) NAME
#ifdef __cplusplus
#	define _CF_V3_1(a) CF_V3{ (a), (a), (a) }
#	define _CF_V3_3(a, b, c) CF_V3{ (a), (b), (c) }
#	define cf_v3(...) CF_EXPAND(_CF_V3_SELECT(__VA_ARGS__, _CF_V3_3, _CF_V3_E2, _CF_V3_1)(__VA_ARGS__))
#	define V3 cf_v3
#else
#	define _CF_V3_1(a) (CF_V3){ .x = (a), .y = (a), .z = (a) }
#	define _CF_V3_3(a, b, c) (CF_V3){ .x = (a), .y = (b), .z = (c) }
#	define cf_v3(...) CF_EXPAND(_CF_V3_SELECT(__VA_ARGS__, _CF_V3_3, _CF_V3_E2, _CF_V3_1)(__VA_ARGS__))
#endif

// See the note on cf_v3 above -- documented in the CF_V4 struct to avoid a page-name collision.
#define cf_v4(...)
#undef cf_v4
#define _CF_V4_SELECT(_1, _2, _3, _4, NAME, ...) NAME
#ifdef __cplusplus
#	define _CF_V4_1(a) CF_V4{ (a), (a), (a), (a) }
#	define _CF_V4_4(a, b, c, d) CF_V4{ (a), (b), (c), (d) }
#	define cf_v4(...) CF_EXPAND(_CF_V4_SELECT(__VA_ARGS__, _CF_V4_4, _CF_V4_E3, _CF_V4_E2, _CF_V4_1)(__VA_ARGS__))
#	define V4 cf_v4
#else
#	define _CF_V4_1(a) (CF_V4){ .x = (a), .y = (a), .z = (a), .w = (a) }
#	define _CF_V4_4(a, b, c, d) (CF_V4){ .x = (a), .y = (b), .z = (c), .w = (d) }
#	define cf_v4(...) CF_EXPAND(_CF_V4_SELECT(__VA_ARGS__, _CF_V4_4, _CF_V4_E3, _CF_V4_E2, _CF_V4_1)(__VA_ARGS__))
#endif

/**
 * @function cf_v4_from_v3
 * @category math
 * @brief    Extends a `CF_V3` into a `CF_V4` with an explicit w component.
 * @param    a  The 3d vector.
 * @param    w  The w component. Use 1 for a point, 0 for a direction.
 * @return   Returns `{ a.x, a.y, a.z, w }`.
 * @related  CF_V4 CF_V3 cf_v4 cf_xyz
 */
CF_INLINE CF_V4 cf_v4_from_v3(CF_V3 a, float w) { return cf_v4(a.x, a.y, a.z, w); }

/**
 * @function cf_xyz
 * @category math
 * @brief    Truncates a `CF_V4` to its xyz components.
 * @param    a  The 4d vector.
 * @return   Returns `{ a.x, a.y, a.z }`.
 * @related  CF_V3 CF_V4 cf_v4_from_v3
 */
CF_INLINE CF_V3 cf_xyz(CF_V4 a) { return cf_v3(a.x, a.y, a.z); }

// Documented in the CF_Quat struct -- see the note on cf_v3 above about page-name collisions.
CF_INLINE CF_Quat cf_quat(float x, float y, float z, float w) { CF_Quat q; q.x = x; q.y = y; q.z = z; q.w = w; return q; }

//--------------------------------------------------------------------------------------------------
// Suffixed backends. These are shared by the C++ overloads and the C _Generic dispatch below.

CF_INLINE CF_V3 cf_add_v3(CF_V3 a, CF_V3 b) { return cf_v3(a.x + b.x, a.y + b.y, a.z + b.z); }
CF_INLINE CF_V4 cf_add_v4(CF_V4 a, CF_V4 b) { return cf_v4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
CF_INLINE CF_V3 cf_sub_v3(CF_V3 a, CF_V3 b) { return cf_v3(a.x - b.x, a.y - b.y, a.z - b.z); }
CF_INLINE CF_V4 cf_sub_v4(CF_V4 a, CF_V4 b) { return cf_v4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
CF_INLINE CF_V3 cf_neg_v3(CF_V3 a) { return cf_v3(-a.x, -a.y, -a.z); }
CF_INLINE CF_V4 cf_neg_v4(CF_V4 a) { return cf_v4(-a.x, -a.y, -a.z, -a.w); }

CF_INLINE CF_V3 cf_mul_v3(CF_V3 a, CF_V3 b) { return cf_v3(a.x * b.x, a.y * b.y, a.z * b.z); }
CF_INLINE CF_V3 cf_mul_v3_f(CF_V3 a, float b) { return cf_v3(a.x * b, a.y * b, a.z * b); }
CF_INLINE CF_V4 cf_mul_v4(CF_V4 a, CF_V4 b) { return cf_v4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }
CF_INLINE CF_V4 cf_mul_v4_f(CF_V4 a, float b) { return cf_v4(a.x * b, a.y * b, a.z * b, a.w * b); }
CF_INLINE CF_V3 cf_div_v3(CF_V3 a, CF_V3 b) { return cf_v3(a.x / b.x, a.y / b.y, a.z / b.z); }
CF_INLINE CF_V3 cf_div_v3_f(CF_V3 a, float b) { return cf_v3(a.x / b, a.y / b, a.z / b); }
CF_INLINE CF_V4 cf_div_v4(CF_V4 a, CF_V4 b) { return cf_v4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w); }
CF_INLINE CF_V4 cf_div_v4_f(CF_V4 a, float b) { return cf_v4(a.x / b, a.y / b, a.z / b, a.w / b); }

CF_INLINE float cf_dot_v3(CF_V3 a, CF_V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
CF_INLINE float cf_dot_v4(CF_V4 a, CF_V4 b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
CF_INLINE CF_V3 cf_cross_v3(CF_V3 a, CF_V3 b) { return cf_v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }

CF_INLINE float cf_len_sq_v3(CF_V3 a) { return cf_dot_v3(a, a); }
CF_INLINE float cf_len_sq_v4(CF_V4 a) { return cf_dot_v4(a, a); }
CF_INLINE float cf_len_v3(CF_V3 a) { return CF_SQRTF(cf_dot_v3(a, a)); }
CF_INLINE float cf_len_v4(CF_V4 a) { return CF_SQRTF(cf_dot_v4(a, a)); }
CF_INLINE float cf_distance_v3(CF_V3 a, CF_V3 b) { CF_V3 d = cf_sub_v3(b, a); return CF_SQRTF(cf_dot_v3(d, d)); }
CF_INLINE CF_V3 cf_norm_v3(CF_V3 a) { return cf_div_v3_f(a, cf_len_v3(a)); }
CF_INLINE CF_V4 cf_norm_v4(CF_V4 a) { return cf_div_v4_f(a, cf_len_v4(a)); }
CF_INLINE CF_V3 cf_safe_norm_v3(CF_V3 a) { float sq = cf_dot_v3(a, a); return sq != 0.0f ? cf_div_v3_f(a, CF_SQRTF(sq)) : cf_v3(0.0f); }
CF_INLINE CF_V4 cf_safe_norm_v4(CF_V4 a) { float sq = cf_dot_v4(a, a); return sq != 0.0f ? cf_div_v4_f(a, CF_SQRTF(sq)) : cf_v4(0.0f); }

// Spelled with plain ternaries rather than cf_min_f/cf_max_f/cf_abs_f: those suffixed helpers only
// exist in cute_math.h's C branch, where C++ gets overloads on the unsuffixed names instead.
CF_INLINE CF_V3 cf_min_v3(CF_V3 a, CF_V3 b) { return cf_v3(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z); }
CF_INLINE CF_V3 cf_max_v3(CF_V3 a, CF_V3 b) { return cf_v3(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z); }
CF_INLINE CF_V3 cf_abs_v3(CF_V3 a) { return cf_v3(a.x < 0 ? -a.x : a.x, a.y < 0 ? -a.y : a.y, a.z < 0 ? -a.z : a.z); }
CF_INLINE CF_V3 cf_lerp_v3(CF_V3 a, CF_V3 b, float t) { return cf_add_v3(a, cf_mul_v3_f(cf_sub_v3(b, a), t)); }
CF_INLINE CF_V4 cf_lerp_v4(CF_V4 a, CF_V4 b, float t) { return cf_add_v4(a, cf_mul_v4_f(cf_sub_v4(b, a), t)); }

#ifdef __cplusplus
} // extern "C"
CF_INLINE CF_V3 cf_min(CF_V3 a, CF_V3 b) { return cf_min_v3(a, b); }
CF_INLINE CF_V3 cf_max(CF_V3 a, CF_V3 b) { return cf_max_v3(a, b); }
CF_INLINE CF_V3 cf_abs(CF_V3 a) { return cf_abs_v3(a); }
CF_INLINE CF_V3 cf_add(CF_V3 a, CF_V3 b) { return cf_add_v3(a, b); }
CF_INLINE CF_V4 cf_add(CF_V4 a, CF_V4 b) { return cf_add_v4(a, b); }
CF_INLINE CF_V3 cf_sub(CF_V3 a, CF_V3 b) { return cf_sub_v3(a, b); }
CF_INLINE CF_V4 cf_sub(CF_V4 a, CF_V4 b) { return cf_sub_v4(a, b); }
CF_INLINE CF_V3 cf_neg(CF_V3 a) { return cf_neg_v3(a); }
CF_INLINE CF_V4 cf_neg(CF_V4 a) { return cf_neg_v4(a); }
CF_INLINE float cf_dot(CF_V3 a, CF_V3 b) { return cf_dot_v3(a, b); }
CF_INLINE float cf_dot(CF_V4 a, CF_V4 b) { return cf_dot_v4(a, b); }
CF_INLINE CF_V3 cf_cross(CF_V3 a, CF_V3 b) { return cf_cross_v3(a, b); }
CF_INLINE float cf_len(CF_V3 a) { return cf_len_v3(a); }
CF_INLINE float cf_len(CF_V4 a) { return cf_len_v4(a); }
CF_INLINE float cf_len_sq(CF_V3 a) { return cf_len_sq_v3(a); }
CF_INLINE float cf_len_sq(CF_V4 a) { return cf_len_sq_v4(a); }
CF_INLINE float cf_distance(CF_V3 a, CF_V3 b) { return cf_distance_v3(a, b); }
CF_INLINE CF_V3 cf_norm(CF_V3 a) { return cf_norm_v3(a); }
CF_INLINE CF_V4 cf_norm(CF_V4 a) { return cf_norm_v4(a); }
CF_INLINE CF_V3 cf_safe_norm(CF_V3 a) { return cf_safe_norm_v3(a); }
CF_INLINE CF_V4 cf_safe_norm(CF_V4 a) { return cf_safe_norm_v4(a); }
CF_INLINE CF_V3 cf_lerp(CF_V3 a, CF_V3 b, float t) { return cf_lerp_v3(a, b, t); }
CF_INLINE CF_V4 cf_lerp(CF_V4 a, CF_V4 b, float t) { return cf_lerp_v4(a, b, t); }
CF_INLINE CF_V3 cf_mul(CF_V3 a, CF_V3 b) { return cf_mul_v3(a, b); }
CF_INLINE CF_V3 cf_mul(CF_V3 a, float b) { return cf_mul_v3_f(a, b); }
CF_INLINE CF_V4 cf_mul(CF_V4 a, CF_V4 b) { return cf_mul_v4(a, b); }
CF_INLINE CF_V4 cf_mul(CF_V4 a, float b) { return cf_mul_v4_f(a, b); }
CF_INLINE CF_V3 cf_div(CF_V3 a, CF_V3 b) { return cf_div_v3(a, b); }
CF_INLINE CF_V3 cf_div(CF_V3 a, float b) { return cf_div_v3_f(a, b); }
CF_INLINE CF_V4 cf_div(CF_V4 a, CF_V4 b) { return cf_div_v4(a, b); }
CF_INLINE CF_V4 cf_div(CF_V4 a, float b) { return cf_div_v4_f(a, b); }
extern "C" {
#else
// Redefine the shared selectors with the 3d rows appended. The 2d and scalar rows come from
// the CF_*_CASES macros in cute_math.h, so they are never duplicated here.
#define CF_MIN_CASES_3D CF_V3: cf_min_v3
#define CF_MAX_CASES_3D CF_V3: cf_max_v3
#define CF_ABS_CASES_3D CF_V3: cf_abs_v3
#define CF_ADD_CASES_3D CF_V3: cf_add_v3, CF_V4: cf_add_v4
#define CF_SUB_CASES_3D CF_V3: cf_sub_v3, CF_V4: cf_sub_v4
#define CF_NEG_CASES_3D CF_V3: cf_neg_v3, CF_V4: cf_neg_v4
#define CF_DOT_CASES_3D CF_V3: cf_dot_v3, CF_V4: cf_dot_v4
#define CF_LEN_CASES_3D CF_V3: cf_len_v3, CF_V4: cf_len_v4
#define CF_LEN_SQ_CASES_3D CF_V3: cf_len_sq_v3, CF_V4: cf_len_sq_v4
#define CF_DISTANCE_CASES_3D CF_V3: cf_distance_v3
#define CF_NORM_CASES_3D CF_V3: cf_norm_v3, CF_V4: cf_norm_v4
#define CF_SAFE_NORM_CASES_3D CF_V3: cf_safe_norm_v3, CF_V4: cf_safe_norm_v4
#define CF_LERP_CASES_3D CF_V3: cf_lerp_v3, CF_V4: cf_lerp_v4

#undef cf_min
#define cf_min(a, b) _Generic((a), CF_MIN_CASES, CF_MIN_CASES_3D, default: cf_min_i32)((a), (b))
#undef cf_max
#define cf_max(a, b) _Generic((a), CF_MAX_CASES, CF_MAX_CASES_3D, default: cf_max_i32)((a), (b))
#undef cf_abs
#define cf_abs(x) _Generic((x), CF_ABS_CASES, CF_ABS_CASES_3D, default: cf_abs_i32)(x)
#undef cf_add
#define cf_add(a, b) _Generic((a), CF_ADD_CASES, CF_ADD_CASES_3D, default: cf_add_v2)((a), (b))
#undef cf_sub
#define cf_sub(a, b) _Generic((a), CF_SUB_CASES, CF_SUB_CASES_3D, default: cf_sub_v2)((a), (b))
#undef cf_neg
#define cf_neg(a) _Generic((a), CF_NEG_CASES, CF_NEG_CASES_3D, default: cf_neg_v2)(a)
#undef cf_dot
#define cf_dot(a, b) _Generic((a), CF_DOT_CASES, CF_DOT_CASES_3D, default: cf_dot_v2)((a), (b))
#undef cf_len
#define cf_len(a) _Generic((a), CF_LEN_CASES, CF_LEN_CASES_3D, default: cf_len_v2)(a)
#undef cf_len_sq
#define cf_len_sq(a) _Generic((a), CF_LEN_SQ_CASES, CF_LEN_SQ_CASES_3D, default: cf_len_sq_v2)(a)
#undef cf_distance
#define cf_distance(a, b) _Generic((a), CF_DISTANCE_CASES, CF_DISTANCE_CASES_3D, default: cf_distance_v2)((a), (b))
#undef cf_norm
#define cf_norm(a) _Generic((a), CF_NORM_CASES, CF_NORM_CASES_3D, default: cf_norm_v2)(a)
#undef cf_safe_norm
#define cf_safe_norm(a) _Generic((a), CF_SAFE_NORM_CASES, CF_SAFE_NORM_CASES_3D, default: cf_safe_norm_v2)(a)
#undef cf_lerp
#define cf_lerp(a, b, t) _Generic((a), CF_LERP_CASES, CF_LERP_CASES_3D, default: cf_lerp_f)((a), (b), (t))
#undef cf_cross
#define cf_cross(a, b) _Generic((a), CF_CROSS_CASES((b)), CF_V3: cf_cross_v3, default: cf_cross_v2)((a), (b))
#endif

//--------------------------------------------------------------------------------------------------
// Quaternions.

/**
 * @function cf_quat_identity
 * @category math
 * @brief    Returns the identity quaternion, representing no rotation.
 * @related  CF_Quat cf_quat cf_quat_from_axis_angle cf_quat_to_m4
 */
CF_INLINE CF_Quat cf_quat_identity(void) { return cf_quat(0, 0, 0, 1.0f); }

/**
 * @function cf_quat_from_axis_angle
 * @category math
 * @brief    Builds a quaternion rotating about `axis` by `radians`.
 * @param    axis     The axis to rotate about. Should be unit length.
 * @param    radians  The rotation angle, counter-clockwise when looking down the axis toward the origin.
 * @return   Returns a unit quaternion.
 * @related  CF_Quat cf_quat_identity cf_quat_to_m4 cf_quat_norm cf_quat_slerp
 */
CF_INLINE CF_Quat cf_quat_from_axis_angle(CF_V3 axis, float radians)
{
	float h = radians * 0.5f;
	float s = CF_SINF(h);
	return cf_quat(axis.x * s, axis.y * s, axis.z * s, CF_COSF(h));
}

/**
 * @function cf_quat_norm
 * @category math
 * @brief    Normalizes a quaternion to unit length.
 * @related  CF_Quat cf_quat_from_axis_angle cf_quat_conjugate cf_quat_slerp
 */
CF_INLINE CF_Quat cf_quat_norm(CF_Quat q)
{
	float d = CF_SQRTF(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
	if (d == 0.0f) return cf_quat_identity();
	d = 1.0f / d;
	return cf_quat(q.x * d, q.y * d, q.z * d, q.w * d);
}

/**
 * @function cf_quat_conjugate
 * @category math
 * @brief    Returns the conjugate of a quaternion, which for a unit quaternion is its inverse.
 * @related  CF_Quat cf_quat_norm cf_quat_from_axis_angle
 */
CF_INLINE CF_Quat cf_quat_conjugate(CF_Quat q) { return cf_quat(-q.x, -q.y, -q.z, q.w); }

CF_INLINE CF_Quat cf_mul_q(CF_Quat a, CF_Quat b)
{
	return cf_quat(
		a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
		a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
		a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
		a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
}

CF_INLINE CF_V3 cf_mul_q_v3(CF_Quat q, CF_V3 v)
{
	// v + 2 * cross(q.xyz, cross(q.xyz, v) + q.w * v)
	CF_V3 u = cf_v3(q.x, q.y, q.z);
	CF_V3 t = cf_add_v3(cf_cross_v3(u, v), cf_mul_v3_f(v, q.w));
	return cf_add_v3(v, cf_mul_v3_f(cf_cross_v3(u, t), 2.0f));
}

/**
 * @function cf_quat_slerp
 * @category math
 * @brief    Spherically interpolates between two rotations.
 * @param    a  The rotation at `t == 0`.
 * @param    b  The rotation at `t == 1`.
 * @param    t  The interpolant, from 0 to 1.
 * @return   Returns a unit quaternion along the shortest arc from `a` to `b`.
 * @related  CF_Quat cf_quat_from_axis_angle cf_quat_norm cf_lerp
 */
CF_INLINE CF_Quat cf_quat_slerp(CF_Quat a, CF_Quat b, float t)
{
	float d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	if (d < 0) { b = cf_quat(-b.x, -b.y, -b.z, -b.w); d = -d; }
	if (d > 0.9995f) {
		// Nearly parallel -- lerp and renormalize to dodge a division by ~zero.
		return cf_quat_norm(cf_quat(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t));
	}
	float theta = CF_ACOSF(d);
	float st = CF_SINF(theta);
	float wa = CF_SINF((1.0f - t) * theta) / st;
	float wb = CF_SINF(t * theta) / st;
	return cf_quat(a.x * wa + b.x * wb, a.y * wa + b.y * wb, a.z * wa + b.z * wb, a.w * wa + b.w * wb);
}

//--------------------------------------------------------------------------------------------------
// 4x4 matrices.

/**
 * @function cf_m4_identity
 * @category math
 * @brief    Returns the 4x4 identity matrix.
 * @related  CF_M4x4 cf_mul cf_m4_translate cf_m4_scale cf_m4_transpose cf_m4_invert
 */
CF_INLINE CF_M4x4 cf_m4_identity(void)
{
	CF_M4x4 m;
	for (int i = 0; i < 16; ++i) m.elements[i] = 0;
	m.elements[0] = m.elements[5] = m.elements[10] = m.elements[15] = 1.0f;
	return m;
}

CF_INLINE CF_M4x4 cf_mul_m4(CF_M4x4 a, CF_M4x4 b)
{
	CF_M4x4 r;
	for (int c = 0; c < 4; ++c) {
		for (int row = 0; row < 4; ++row) {
			float sum = 0;
			for (int k = 0; k < 4; ++k) sum += a.elements[k * 4 + row] * b.elements[c * 4 + k];
			r.elements[c * 4 + row] = sum;
		}
	}
	return r;
}

CF_INLINE CF_V4 cf_mul_m4_v4(CF_M4x4 a, CF_V4 b)
{
	return cf_v4(
		a.elements[0] * b.x + a.elements[4] * b.y + a.elements[8]  * b.z + a.elements[12] * b.w,
		a.elements[1] * b.x + a.elements[5] * b.y + a.elements[9]  * b.z + a.elements[13] * b.w,
		a.elements[2] * b.x + a.elements[6] * b.y + a.elements[10] * b.z + a.elements[14] * b.w,
		a.elements[3] * b.x + a.elements[7] * b.y + a.elements[11] * b.z + a.elements[15] * b.w);
}

CF_INLINE CF_M4x4 cf_mul_m4_f(CF_M4x4 a, float b)
{
	CF_M4x4 r;
	for (int i = 0; i < 16; ++i) r.elements[i] = a.elements[i] * b;
	return r;
}

/**
 * @function cf_m4_translate
 * @category math
 * @brief    Builds a translation matrix.
 * @param    t  The translation.
 * @related  CF_M4x4 cf_m4_identity cf_m4_scale cf_m4_rotate cf_mul
 */
CF_INLINE CF_M4x4 cf_m4_translate(CF_V3 t)
{
	CF_M4x4 m = cf_m4_identity();
	m.elements[12] = t.x;
	m.elements[13] = t.y;
	m.elements[14] = t.z;
	return m;
}

/**
 * @function cf_m4_scale
 * @category math
 * @brief    Builds a non-uniform scale matrix.
 * @param    s  The per-axis scale.
 * @related  CF_M4x4 cf_m4_identity cf_m4_translate cf_m4_rotate cf_mul
 */
CF_INLINE CF_M4x4 cf_m4_scale(CF_V3 s)
{
	CF_M4x4 m = cf_m4_identity();
	m.elements[0] = s.x;
	m.elements[5] = s.y;
	m.elements[10] = s.z;
	return m;
}

/**
 * @function cf_m4_rotate_x
 * @category math
 * @brief    Builds a matrix rotating about the x axis.
 * @param    radians  The rotation angle.
 * @related  CF_M4x4 cf_m4_rotate_y cf_m4_rotate_z cf_m4_rotate cf_quat_to_m4
 */
CF_INLINE CF_M4x4 cf_m4_rotate_x(float radians)
{
	CF_M4x4 m = cf_m4_identity();
	float c = CF_COSF(radians), s = CF_SINF(radians);
	m.elements[5] = c; m.elements[6] = s;
	m.elements[9] = -s; m.elements[10] = c;
	return m;
}

/**
 * @function cf_m4_rotate_y
 * @category math
 * @brief    Builds a matrix rotating about the y axis.
 * @param    radians  The rotation angle.
 * @related  CF_M4x4 cf_m4_rotate_x cf_m4_rotate_z cf_m4_rotate cf_quat_to_m4
 */
CF_INLINE CF_M4x4 cf_m4_rotate_y(float radians)
{
	CF_M4x4 m = cf_m4_identity();
	float c = CF_COSF(radians), s = CF_SINF(radians);
	m.elements[0] = c; m.elements[2] = -s;
	m.elements[8] = s; m.elements[10] = c;
	return m;
}

/**
 * @function cf_m4_rotate_z
 * @category math
 * @brief    Builds a matrix rotating about the z axis.
 * @param    radians  The rotation angle.
 * @related  CF_M4x4 cf_m4_rotate_x cf_m4_rotate_y cf_m4_rotate cf_quat_to_m4
 */
CF_INLINE CF_M4x4 cf_m4_rotate_z(float radians)
{
	CF_M4x4 m = cf_m4_identity();
	float c = CF_COSF(radians), s = CF_SINF(radians);
	m.elements[0] = c; m.elements[1] = s;
	m.elements[4] = -s; m.elements[5] = c;
	return m;
}

/**
 * @function cf_quat_to_m4
 * @category math
 * @brief    Converts a unit quaternion into a rotation matrix.
 * @param    q  The rotation. Should be unit length -- see `cf_quat_norm`.
 * @related  CF_Quat CF_M4x4 cf_quat_from_axis_angle cf_quat_norm cf_m4_rotate
 */
CF_INLINE CF_M4x4 cf_quat_to_m4(CF_Quat q)
{
	CF_M4x4 m = cf_m4_identity();
	float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
	float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
	float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
	m.elements[0] = 1.0f - 2.0f * (yy + zz);
	m.elements[1] = 2.0f * (xy + wz);
	m.elements[2] = 2.0f * (xz - wy);
	m.elements[4] = 2.0f * (xy - wz);
	m.elements[5] = 1.0f - 2.0f * (xx + zz);
	m.elements[6] = 2.0f * (yz + wx);
	m.elements[8] = 2.0f * (xz + wy);
	m.elements[9] = 2.0f * (yz - wx);
	m.elements[10] = 1.0f - 2.0f * (xx + yy);
	return m;
}

/**
 * @function cf_m4_rotate
 * @category math
 * @brief    Builds a matrix rotating about an arbitrary axis.
 * @param    axis     The axis to rotate about. Should be unit length.
 * @param    radians  The rotation angle.
 * @related  CF_M4x4 cf_m4_rotate_x cf_m4_rotate_y cf_m4_rotate_z cf_quat_to_m4
 */
CF_INLINE CF_M4x4 cf_m4_rotate(CF_V3 axis, float radians) { return cf_quat_to_m4(cf_quat_from_axis_angle(axis, radians)); }

/**
 * @function cf_m4_transpose
 * @category math
 * @brief    Returns the transpose of a matrix.
 * @related  CF_M4x4 cf_m4_invert cf_m4_normal_matrix cf_mul
 */
CF_INLINE CF_M4x4 cf_m4_transpose(CF_M4x4 a)
{
	CF_M4x4 r;
	for (int c = 0; c < 4; ++c)
		for (int row = 0; row < 4; ++row)
			r.elements[c * 4 + row] = a.elements[row * 4 + c];
	return r;
}

/**
 * @function cf_m4_invert
 * @category math
 * @brief    Returns the inverse of a matrix.
 * @param    a  The matrix to invert.
 * @return   Returns the inverse, or the identity matrix if `a` is singular.
 * @remarks  This is a general 4x4 inverse. If the matrix is a rigid transform (rotation plus
 *           translation with no scale), transposing the upper 3x3 and negating the translation is
 *           considerably cheaper.
 * @related  CF_M4x4 cf_m4_transpose cf_m4_normal_matrix cf_mul
 */
CF_INLINE CF_M4x4 cf_m4_invert(CF_M4x4 a)
{
	const float* m = a.elements;
	CF_M4x4 out;
	float* inv = out.elements;

	inv[0]  =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
	inv[4]  = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
	inv[8]  =  m[4]*m[9]*m[15]  - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
	inv[12] = -m[4]*m[9]*m[14]  + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
	inv[1]  = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
	inv[5]  =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
	inv[9]  = -m[0]*m[9]*m[15]  + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
	inv[13] =  m[0]*m[9]*m[14]  - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
	inv[2]  =  m[1]*m[6]*m[15]  - m[1]*m[7]*m[14]  - m[5]*m[2]*m[15] + m[5]*m[3]*m[14] + m[13]*m[2]*m[7]  - m[13]*m[3]*m[6];
	inv[6]  = -m[0]*m[6]*m[15]  + m[0]*m[7]*m[14]  + m[4]*m[2]*m[15] - m[4]*m[3]*m[14] - m[12]*m[2]*m[7]  + m[12]*m[3]*m[6];
	inv[10] =  m[0]*m[5]*m[15]  - m[0]*m[7]*m[13]  - m[4]*m[1]*m[15] + m[4]*m[3]*m[13] + m[12]*m[1]*m[7]  - m[12]*m[3]*m[5];
	inv[14] = -m[0]*m[5]*m[14]  + m[0]*m[6]*m[13]  + m[4]*m[1]*m[14] - m[4]*m[2]*m[13] - m[12]*m[1]*m[6]  + m[12]*m[2]*m[5];
	inv[3]  = -m[1]*m[6]*m[11]  + m[1]*m[7]*m[10]  + m[5]*m[2]*m[11] - m[5]*m[3]*m[10] - m[9]*m[2]*m[7]   + m[9]*m[3]*m[6];
	inv[7]  =  m[0]*m[6]*m[11]  - m[0]*m[7]*m[10]  - m[4]*m[2]*m[11] + m[4]*m[3]*m[10] + m[8]*m[2]*m[7]   - m[8]*m[3]*m[6];
	inv[11] = -m[0]*m[5]*m[11]  + m[0]*m[7]*m[9]   + m[4]*m[1]*m[11] - m[4]*m[3]*m[9]  - m[8]*m[1]*m[7]   + m[8]*m[3]*m[5];
	inv[15] =  m[0]*m[5]*m[10]  - m[0]*m[6]*m[9]   - m[4]*m[1]*m[10] + m[4]*m[2]*m[9]  + m[8]*m[1]*m[6]   - m[8]*m[2]*m[5];

	float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
	if (det == 0.0f) return cf_m4_identity();
	det = 1.0f / det;
	for (int i = 0; i < 16; ++i) inv[i] *= det;
	return out;
}

/**
 * @function cf_m4_normal_matrix
 * @category math
 * @brief    Returns the matrix for transforming normals: the inverse-transpose of `model`.
 * @param    model  The model (or model-view) matrix.
 * @remarks  Normals must not be transformed by the model matrix directly when it contains non-uniform
 *           scale -- doing so leaves them non-perpendicular to the surface. The translation column is
 *           irrelevant to normals and is left as-is; upload the result as a `mat4` and use its upper
 *           3x3 in the shader. Metal only supports `mat4` inside a uniform block, so a `mat3` normal
 *           matrix cannot be uploaded directly.
 * @related  CF_M4x4 cf_m4_invert cf_m4_transpose cf_m4_transform_dir
 */
CF_INLINE CF_M4x4 cf_m4_normal_matrix(CF_M4x4 model) { return cf_m4_transpose(cf_m4_invert(model)); }

/**
 * @function cf_m4_transform_point
 * @category math
 * @brief    Transforms a point by a matrix, with the perspective divide applied.
 * @param    m  The matrix.
 * @param    p  The point. Treated as `w = 1`, so translation applies.
 * @related  CF_M4x4 CF_V3 cf_m4_transform_dir cf_mul
 */
CF_INLINE CF_V3 cf_m4_transform_point(CF_M4x4 m, CF_V3 p)
{
	CF_V4 r = cf_mul_m4_v4(m, cf_v4(p.x, p.y, p.z, 1.0f));
	if (r.w != 0.0f && r.w != 1.0f) { float iw = 1.0f / r.w; r.x *= iw; r.y *= iw; r.z *= iw; }
	return cf_v3(r.x, r.y, r.z);
}

/**
 * @function cf_m4_transform_dir
 * @category math
 * @brief    Transforms a direction by a matrix, ignoring translation.
 * @param    m  The matrix.
 * @param    d  The direction. Treated as `w = 0`, so translation does not apply.
 * @related  CF_M4x4 CF_V3 cf_m4_transform_point cf_m4_normal_matrix
 */
CF_INLINE CF_V3 cf_m4_transform_dir(CF_M4x4 m, CF_V3 d)
{
	CF_V4 r = cf_mul_m4_v4(m, cf_v4(d.x, d.y, d.z, 0.0f));
	return cf_v3(r.x, r.y, r.z);
}

//--------------------------------------------------------------------------------------------------
// Projection and view.

/**
 * @function cf_ortho
 * @category math
 * @brief    Builds a right-handed orthographic projection matrix.
 * @param    left    The left clip plane.
 * @param    right   The right clip plane.
 * @param    bottom  The bottom clip plane.
 * @param    top     The top clip plane.
 * @param    znear   Distance to the near clip plane. Positive, measured along -z.
 * @param    zfar    Distance to the far clip plane. Positive, measured along -z.
 * @return   Returns a matrix mapping the view volume to clip space.
 * @remarks  Right-handed: the camera looks down **-z**, so visible geometry has negative view-space z
 *           while `znear` and `zfar` are given as positive distances.
 *
 *           **The z convention matters.** SDL_GPU normalizes clip-space z to **[0, 1]** on every
 *           backend (the D3D/Metal/Vulkan convention), not OpenGL's [-1, 1]. This matrix maps
 *           `znear` to 0 and `zfar` to 1, which pairs with `CF_COMPARE_FUNCTION_LESS_THAN` and a
 *           depth clear of 1.0. The textbook `glOrtho` formula compiles and produces a plausible
 *           image, but sends half the depth range to negative NDC z where the GPU clips it away --
 *           the classic "my model has holes and I can see inside it" bug.
 *
 *           Depth testing also requires a canvas created with `depth_stencil_enable` set to true;
 *           `cf_canvas_defaults` leaves it off, and without it all depth render state is ignored.
 * @related  CF_M4x4 cf_perspective cf_look_at cf_ortho_2d cf_mul
 */
CF_INLINE CF_M4x4 cf_ortho(float left, float right, float bottom, float top, float znear, float zfar)
{
	CF_M4x4 m;
	for (int i = 0; i < 16; ++i) m.elements[i] = 0;
	m.elements[0] = 2.0f / (right - left);
	m.elements[5] = 2.0f / (top - bottom);
	m.elements[10] = -1.0f / (zfar - znear);
	m.elements[12] = -(right + left) / (right - left);
	m.elements[13] = -(top + bottom) / (top - bottom);
	m.elements[14] = -znear / (zfar - znear);
	m.elements[15] = 1.0f;
	return m;
}

/**
 * @function cf_perspective
 * @category math
 * @brief    Builds a right-handed perspective projection matrix.
 * @param    fov_radians  The vertical field of view.
 * @param    aspect       The viewport aspect ratio, width divided by height.
 * @param    znear        Distance to the near clip plane. Must be greater than zero.
 * @param    zfar         Distance to the far clip plane.
 * @return   Returns a matrix mapping the view frustum to clip space.
 * @remarks  Right-handed, with clip-space z in **[0, 1]** -- `znear` maps to 0 and `zfar` to 1. See
 *           `cf_ortho` for why that convention is not optional on SDL_GPU. Pairs with
 *           `CF_COMPARE_FUNCTION_LESS_THAN` and a depth clear of 1.0.
 * @related  CF_M4x4 cf_ortho cf_look_at cf_mul CF_RenderState
 */
CF_INLINE CF_M4x4 cf_perspective(float fov_radians, float aspect, float znear, float zfar)
{
	CF_M4x4 m;
	for (int i = 0; i < 16; ++i) m.elements[i] = 0;
	float t = CF_TANF(fov_radians * 0.5f);
	m.elements[0] = 1.0f / (aspect * t);
	m.elements[5] = 1.0f / t;
	m.elements[10] = zfar / (znear - zfar);
	m.elements[11] = -1.0f;
	m.elements[14] = -(zfar * znear) / (zfar - znear);
	return m;
}

/**
 * @function cf_look_at
 * @category math
 * @brief    Builds a right-handed view matrix looking from `eye` toward `target`.
 * @param    eye     The camera position in world space.
 * @param    target  The point to look at.
 * @param    up      The world-space up direction, typically `cf_v3(0, 1, 0)`.
 * @return   Returns a matrix transforming world space into view space.
 * @remarks  Right-handed: in the resulting view space the camera looks down **-z**, so points in
 *           front of the camera have negative z. This matches `cf_perspective` and `cf_ortho`, which
 *           take `znear`/`zfar` as positive distances along that axis. Mixing this with a
 *           left-handed projection silently renders the scene inside-out.
 * @related  CF_M4x4 cf_perspective cf_ortho cf_mul
 */
CF_INLINE CF_M4x4 cf_look_at(CF_V3 eye, CF_V3 target, CF_V3 up)
{
	CF_V3 f = cf_safe_norm_v3(cf_sub_v3(target, eye));
	CF_V3 s = cf_safe_norm_v3(cf_cross_v3(f, up));
	CF_V3 u = cf_cross_v3(s, f);
	CF_M4x4 m = cf_m4_identity();
	m.elements[0] = s.x; m.elements[4] = s.y; m.elements[8]  = s.z;
	m.elements[1] = u.x; m.elements[5] = u.y; m.elements[9]  = u.z;
	m.elements[2] = -f.x; m.elements[6] = -f.y; m.elements[10] = -f.z;
	m.elements[12] = -cf_dot_v3(s, eye);
	m.elements[13] = -cf_dot_v3(u, eye);
	m.elements[14] =  cf_dot_v3(f, eye);
	return m;
}

//--------------------------------------------------------------------------------------------------
// Flat-expansion macros. MSVC /Od ignores __forceinline, so the functions above become real calls in
// debug builds -- a single mat4 multiply is 64 muls and 48 adds behind four levels of call. These
// write the result into `dst` with no calls at all. All arguments must be side-effect-free.

// M4x4 * M4x4 -> writes CF_M4x4 into dst. `dst` must not alias `a` or `b`.
#define CF_MUL_M4_M4(dst, a, b) do { \
	for (int _cf_c = 0; _cf_c < 4; ++_cf_c) { \
		for (int _cf_r = 0; _cf_r < 4; ++_cf_r) { \
			(dst).elements[_cf_c * 4 + _cf_r] = \
				(a).elements[0 * 4 + _cf_r] * (b).elements[_cf_c * 4 + 0] + \
				(a).elements[1 * 4 + _cf_r] * (b).elements[_cf_c * 4 + 1] + \
				(a).elements[2 * 4 + _cf_r] * (b).elements[_cf_c * 4 + 2] + \
				(a).elements[3 * 4 + _cf_r] * (b).elements[_cf_c * 4 + 3]; \
		} \
	} \
} while (0)

// M4x4 * V4 -> writes CF_V4 into dst.
#define CF_MUL_M4_V4(dst, a, b) do { \
	float _cf_x = (b).x, _cf_y = (b).y, _cf_z = (b).z, _cf_w = (b).w; \
	(dst).x = (a).elements[0] * _cf_x + (a).elements[4] * _cf_y + (a).elements[8]  * _cf_z + (a).elements[12] * _cf_w; \
	(dst).y = (a).elements[1] * _cf_x + (a).elements[5] * _cf_y + (a).elements[9]  * _cf_z + (a).elements[13] * _cf_w; \
	(dst).z = (a).elements[2] * _cf_x + (a).elements[6] * _cf_y + (a).elements[10] * _cf_z + (a).elements[14] * _cf_w; \
	(dst).w = (a).elements[3] * _cf_x + (a).elements[7] * _cf_y + (a).elements[11] * _cf_z + (a).elements[15] * _cf_w; \
} while (0)

#ifdef __cplusplus
} // extern "C"
CF_INLINE CF_M4x4 cf_mul(CF_M4x4 a, CF_M4x4 b) { return cf_mul_m4(a, b); }
CF_INLINE CF_V4 cf_mul(CF_M4x4 a, CF_V4 b) { return cf_mul_m4_v4(a, b); }
CF_INLINE CF_M4x4 cf_mul(CF_M4x4 a, float b) { return cf_mul_m4_f(a, b); }
CF_INLINE CF_Quat cf_mul(CF_Quat a, CF_Quat b) { return cf_mul_q(a, b); }
CF_INLINE CF_V3 cf_mul(CF_Quat a, CF_V3 b) { return cf_mul_q_v3(a, b); }
extern "C" {
#else
#define CF_MUL_CASES_3D(b) \
	CF_V3: _Generic(b, \
		CF_V3:   cf_mul_v3, \
		float:   cf_mul_v3_f, \
		default: cf_mul_v3 \
	), \
	CF_V4: _Generic(b, \
		CF_V4:   cf_mul_v4, \
		float:   cf_mul_v4_f, \
		default: cf_mul_v4 \
	), \
	CF_M4x4: _Generic(b, \
		CF_M4x4: cf_mul_m4, \
		CF_V4:   cf_mul_m4_v4, \
		float:   cf_mul_m4_f, \
		default: cf_mul_m4 \
	), \
	CF_Quat: _Generic(b, \
		CF_Quat: cf_mul_q, \
		CF_V3:   cf_mul_q_v3, \
		default: cf_mul_q \
	)
#undef cf_mul
#define cf_mul(a, b) \
	_Generic((a), \
		CF_MUL_CASES((b)), \
		CF_MUL_CASES_3D((b)), \
	default: cf_mul_v2 \
	)((a), (b))

#define CF_DIV_CASES_3D(b) \
	CF_V3: _Generic(b, \
		CF_V3:   cf_div_v3, \
		float:   cf_div_v3_f, \
		default: cf_div_v3_f \
	), \
	CF_V4: _Generic(b, \
		CF_V4:   cf_div_v4, \
		float:   cf_div_v4_f, \
		default: cf_div_v4_f \
	)
#undef cf_div
#define cf_div(a, b) \
	_Generic((a), \
		CF_DIV_CASES((b)), \
		CF_DIV_CASES_3D((b)), \
		default: cf_div_v2_f \
	)((a), (b))
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using v3 = CF_V3;
using v4 = CF_V4;
using quat = CF_Quat;
using m4 = CF_M4x4;

CF_INLINE v3 min(v3 a, v3 b) { return cf_min_v3(a, b); }
CF_INLINE v3 max(v3 a, v3 b) { return cf_max_v3(a, b); }
CF_INLINE v3 abs(v3 a) { return cf_abs_v3(a); }
CF_INLINE float dot(v3 a, v3 b) { return cf_dot_v3(a, b); }
CF_INLINE float dot(v4 a, v4 b) { return cf_dot_v4(a, b); }
CF_INLINE v3 cross(v3 a, v3 b) { return cf_cross_v3(a, b); }
CF_INLINE float len(v3 a) { return cf_len_v3(a); }
CF_INLINE float len(v4 a) { return cf_len_v4(a); }
CF_INLINE float len_sq(v3 a) { return cf_len_sq_v3(a); }
CF_INLINE float len_sq(v4 a) { return cf_len_sq_v4(a); }
CF_INLINE float distance(v3 a, v3 b) { return cf_distance_v3(a, b); }
CF_INLINE v3 norm(v3 a) { return cf_norm_v3(a); }
CF_INLINE v4 norm(v4 a) { return cf_norm_v4(a); }
CF_INLINE v3 safe_norm(v3 a) { return cf_safe_norm_v3(a); }
CF_INLINE v4 safe_norm(v4 a) { return cf_safe_norm_v4(a); }
CF_INLINE v3 lerp(v3 a, v3 b, float t) { return cf_lerp_v3(a, b, t); }
CF_INLINE v4 lerp(v4 a, v4 b, float t) { return cf_lerp_v4(a, b, t); }
CF_INLINE v4 v4_from_v3(v3 a, float w) { return cf_v4_from_v3(a, w); }
CF_INLINE v3 xyz(v4 a) { return cf_xyz(a); }

CF_INLINE quat quat_identity() { return cf_quat_identity(); }
CF_INLINE quat quat_from_axis_angle(v3 axis, float radians) { return cf_quat_from_axis_angle(axis, radians); }
CF_INLINE quat norm(quat q) { return cf_quat_norm(q); }
CF_INLINE quat conjugate(quat q) { return cf_quat_conjugate(q); }
CF_INLINE quat slerp(quat a, quat b, float t) { return cf_quat_slerp(a, b, t); }
CF_INLINE m4 to_m4(quat q) { return cf_quat_to_m4(q); }

CF_INLINE m4 m4_identity() { return cf_m4_identity(); }
CF_INLINE m4 m4_translate(v3 t) { return cf_m4_translate(t); }
CF_INLINE m4 m4_scale(v3 s) { return cf_m4_scale(s); }
CF_INLINE m4 m4_rotate(v3 axis, float radians) { return cf_m4_rotate(axis, radians); }
CF_INLINE m4 m4_rotate_x(float radians) { return cf_m4_rotate_x(radians); }
CF_INLINE m4 m4_rotate_y(float radians) { return cf_m4_rotate_y(radians); }
CF_INLINE m4 m4_rotate_z(float radians) { return cf_m4_rotate_z(radians); }
CF_INLINE m4 transpose(m4 a) { return cf_m4_transpose(a); }
CF_INLINE m4 invert(m4 a) { return cf_m4_invert(a); }
CF_INLINE m4 normal_matrix(m4 model) { return cf_m4_normal_matrix(model); }
CF_INLINE v3 transform_point(m4 m, v3 p) { return cf_m4_transform_point(m, p); }
CF_INLINE v3 transform_dir(m4 m, v3 d) { return cf_m4_transform_dir(m, d); }

CF_INLINE m4 ortho(float left, float right, float bottom, float top, float znear, float zfar) { return cf_ortho(left, right, bottom, top, znear, zfar); }
CF_INLINE m4 perspective(float fov_radians, float aspect, float znear, float zfar) { return cf_perspective(fov_radians, aspect, znear, zfar); }
CF_INLINE m4 look_at(v3 eye, v3 target, v3 up) { return cf_look_at(eye, target, up); }

}

// Operators live at global scope, outside namespace Cute, matching cute_math.h's v2 operators.
CF_INLINE Cute::v3 operator+(Cute::v3 a, Cute::v3 b) { return V3(a.x + b.x, a.y + b.y, a.z + b.z); }
CF_INLINE Cute::v3 operator-(Cute::v3 a, Cute::v3 b) { return V3(a.x - b.x, a.y - b.y, a.z - b.z); }
CF_INLINE Cute::v3& operator+=(Cute::v3& a, Cute::v3 b) { return a = a + b; }
CF_INLINE Cute::v3& operator-=(Cute::v3& a, Cute::v3 b) { return a = a - b; }
CF_INLINE Cute::v3 operator*(Cute::v3 a, float b) { return V3(a.x * b, a.y * b, a.z * b); }
CF_INLINE Cute::v3 operator*(float a, Cute::v3 b) { return V3(a * b.x, a * b.y, a * b.z); }
CF_INLINE Cute::v3 operator*(Cute::v3 a, Cute::v3 b) { return V3(a.x * b.x, a.y * b.y, a.z * b.z); }
CF_INLINE Cute::v3& operator*=(Cute::v3& a, float b) { return a = a * b; }
CF_INLINE Cute::v3& operator*=(Cute::v3& a, Cute::v3 b) { return a = a * b; }
CF_INLINE Cute::v3 operator/(Cute::v3 a, float b) { return V3(a.x / b, a.y / b, a.z / b); }
CF_INLINE Cute::v3 operator/(Cute::v3 a, Cute::v3 b) { return V3(a.x / b.x, a.y / b.y, a.z / b.z); }
CF_INLINE Cute::v3& operator/=(Cute::v3& a, float b) { return a = a / b; }
CF_INLINE Cute::v3& operator/=(Cute::v3& a, Cute::v3 b) { return a = a / b; }
CF_INLINE Cute::v3 operator-(Cute::v3 a) { return V3(-a.x, -a.y, -a.z); }
CF_INLINE bool operator==(Cute::v3 a, Cute::v3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
CF_INLINE bool operator!=(Cute::v3 a, Cute::v3 b) { return !(a == b); }

CF_INLINE Cute::v4 operator+(Cute::v4 a, Cute::v4 b) { return V4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
CF_INLINE Cute::v4 operator-(Cute::v4 a, Cute::v4 b) { return V4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
CF_INLINE Cute::v4& operator+=(Cute::v4& a, Cute::v4 b) { return a = a + b; }
CF_INLINE Cute::v4& operator-=(Cute::v4& a, Cute::v4 b) { return a = a - b; }
CF_INLINE Cute::v4 operator*(Cute::v4 a, float b) { return V4(a.x * b, a.y * b, a.z * b, a.w * b); }
CF_INLINE Cute::v4 operator*(float a, Cute::v4 b) { return V4(a * b.x, a * b.y, a * b.z, a * b.w); }
CF_INLINE Cute::v4& operator*=(Cute::v4& a, float b) { return a = a * b; }
CF_INLINE Cute::v4 operator-(Cute::v4 a) { return V4(-a.x, -a.y, -a.z, -a.w); }
CF_INLINE bool operator==(Cute::v4 a, Cute::v4 b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }
CF_INLINE bool operator!=(Cute::v4 a, Cute::v4 b) { return !(a == b); }

CF_INLINE Cute::m4 operator*(Cute::m4 a, Cute::m4 b) { return cf_mul_m4(a, b); }
CF_INLINE Cute::v4 operator*(Cute::m4 a, Cute::v4 b) { return cf_mul_m4_v4(a, b); }
CF_INLINE Cute::quat operator*(Cute::quat a, Cute::quat b) { return cf_mul_q(a, b); }
CF_INLINE Cute::v3 operator*(Cute::quat a, Cute::v3 b) { return cf_mul_q_v3(a, b); }

#endif // CF_CPP

#endif // CF_MATH3D_H
