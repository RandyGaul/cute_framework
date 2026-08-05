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

/**
 * @function cf_v3
 * @category math
 * @brief    Constructs a `CF_V3` from `x`, `y` and `z`, or splats a single value to all three components.
 * @related  CF_V3 cf_v4 cf_dot cf_cross
 */
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

/**
 * @function cf_v4
 * @category math
 * @brief    Constructs a `CF_V4` from `x`, `y`, `z` and `w`, or splats a single value to all four components.
 * @related  CF_V4 cf_v3 cf_v4_from_v3 cf_xyz
 */
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

/**
 * @function cf_quat
 * @category math
 * @brief    Constructs a `CF_Quat` component-wise.
 * @param    x  The x component of the vector part.
 * @param    y  The y component of the vector part.
 * @param    z  The z component of the vector part.
 * @param    w  The scalar part.
 * @return   Returns the quaternion `{ x, y, z, w }`.
 * @remarks  The components are not normalized for you. To construct a rotation, prefer
 *           `cf_quat_from_axis_angle`.
 * @related  CF_Quat cf_quat_identity cf_quat_from_axis_angle cf_quat_norm
 */
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

/**
 * @function cf_reflect_v3
 * @category math
 * @brief    Reflects a vector off a plane given the plane's normal.
 * @param    a  The vector to reflect.
 * @param    n  The plane's normal. Should be unit length.
 * @remarks  The 3d analog of `cf_reflect` -- bounces, ricochets, mirrored cameras. The C++ API
 *           overloads `cf_reflect` directly.
 * @related  CF_V3 cf_slide_v3 cf_project_v3 cf_dot
 */
CF_INLINE CF_V3 cf_reflect_v3(CF_V3 a, CF_V3 n) { return cf_sub_v3(a, cf_mul_v3_f(n, 2.0f * cf_dot_v3(a, n))); }

/**
 * @function cf_project_v3
 * @category math
 * @brief    Projects vector `a` onto vector `b`.
 * @param    a  The vector to project.
 * @param    b  The vector to project onto. Need not be unit length.
 * @return   Returns the component of `a` parallel to `b`, or zero when `b` is zero.
 * @remarks  `cf_slide_v3` returns the complementary perpendicular component. For projecting a
 *           point onto a plane see `cf_project_plane3`.
 * @related  CF_V3 cf_slide_v3 cf_reflect_v3 cf_project_plane3 cf_dot
 */
CF_INLINE CF_V3 cf_project_v3(CF_V3 a, CF_V3 b) { float d = cf_dot_v3(b, b); return d != 0 ? cf_mul_v3_f(b, cf_dot_v3(a, b) / d) : cf_v3(0.0f); }

/**
 * @function cf_slide_v3
 * @category math
 * @brief    Removes from `a` its component along a surface normal, leaving motion along the surface.
 * @param    a  The vector to slide, typically a velocity or movement delta.
 * @param    n  The surface normal. Should be unit length.
 * @remarks  The move-and-slide primitive: hit a wall, slide the remaining motion along it.
 *           Equivalent to `a - n * dot(a, n)`.
 * @related  CF_V3 cf_reflect_v3 cf_project_v3 cf_dot
 */
CF_INLINE CF_V3 cf_slide_v3(CF_V3 a, CF_V3 n) { return cf_sub_v3(a, cf_mul_v3_f(n, cf_dot_v3(a, n))); }

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
CF_INLINE CF_V3 cf_reflect(CF_V3 a, CF_V3 n) { return cf_reflect_v3(a, n); }
CF_INLINE CF_V3 cf_project(CF_V3 a, CF_V3 b) { return cf_project_v3(a, b); }
CF_INLINE CF_V3 cf_slide(CF_V3 a, CF_V3 n) { return cf_slide_v3(a, n); }
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

/**
 * @function cf_orthonormal_basis
 * @category math
 * @brief    Builds two unit vectors perpendicular to `n` and to each other.
 * @param    n      The third axis of the basis. Should be unit length.
 * @param    x_out  Written with the first perpendicular axis.
 * @param    y_out  Written with the second perpendicular axis.
 * @remarks  `(x, y, n)` form a right-handed orthonormal basis: `cross(x, y) == n`. Branchless
 *           and stable for every unit `n` (the Duff et al. method). The tangents are arbitrary
 *           but deterministic -- right for sampling disks and cones around a normal, spawning
 *           particles off a surface, or framing a camera ribbon; wrong for mesh tangent spaces,
 *           which must come from UVs.
 * @related  CF_V3 cf_cross cf_quat_from_basis cf_quat_from_to
 */
CF_INLINE void cf_orthonormal_basis(CF_V3 n, CF_V3* x_out, CF_V3* y_out)
{
	float sign = n.z >= 0 ? 1.0f : -1.0f;
	float a = -1.0f / (sign + n.z);
	float b = n.x * n.y * a;
	*x_out = cf_v3(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x);
	*y_out = cf_v3(b, sign + n.y * n.y * a, -n.y);
}

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

/**
 * @function cf_quat_inverse
 * @category math
 * @brief    Returns the inverse rotation.
 * @remarks  Works for non-unit quaternions by dividing the conjugate by the squared length. A
 *           unit quaternion's inverse IS its conjugate -- prefer `cf_quat_conjugate` when you
 *           know the quaternion is normalized.
 * @related  CF_Quat cf_quat_conjugate cf_quat_norm
 */
CF_INLINE CF_Quat cf_quat_inverse(CF_Quat q)
{
	float d = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
	if (d == 0.0f) return cf_quat_identity();
	d = 1.0f / d;
	return cf_quat(-q.x * d, -q.y * d, -q.z * d, q.w * d);
}

/**
 * @function cf_quat_to_axis_angle
 * @category math
 * @brief    Extracts the rotation axis and angle from a unit quaternion.
 * @param    q          The rotation. Should be unit length.
 * @param    axis_out   Written with the unit rotation axis. May be NULL.
 * @param    angle_out  Written with the angle in radians, in [0, 2pi]. May be NULL.
 * @remarks  The inverse of `cf_quat_from_axis_angle`. A rotation at (or vanishingly near) the
 *           identity has no meaningful axis, so `(1, 0, 0)` with angle 0 is returned. Note a
 *           quaternion with negative `w` reports its angle as greater than pi -- the same
 *           rotation, measured the long way around; negate the quaternion first if you want
 *           the short form.
 * @related  CF_Quat cf_quat_from_axis_angle cf_quat_norm
 */
CF_INLINE void cf_quat_to_axis_angle(CF_Quat q, CF_V3* axis_out, float* angle_out)
{
	float s2 = q.x * q.x + q.y * q.y + q.z * q.z;
	if (s2 < 1e-12f) {
		if (axis_out) *axis_out = cf_v3(1.0f, 0, 0);
		if (angle_out) *angle_out = 0;
		return;
	}
	float s = CF_SQRTF(s2);
	if (axis_out) *axis_out = cf_v3(q.x / s, q.y / s, q.z / s);
	if (angle_out) *angle_out = 2.0f * CF_ATAN2F(s, q.w);
}

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

/**
 * @function cf_quat_nlerp
 * @category math
 * @brief    Normalized linear interpolation between two rotations -- slerp's cheap cousin.
 * @param    a  The rotation at `t == 0`.
 * @param    b  The rotation at `t == 1`.
 * @param    t  The interpolant, from 0 to 1.
 * @return   Returns a unit quaternion along the shortest arc from `a` to `b`.
 * @remarks  Same endpoints and same arc as `cf_quat_slerp`, but the angular velocity is not
 *           constant across `t`. For small differences -- animation keyframes, per-frame
 *           smoothing -- the deviation is invisible and this is measurably cheaper.
 * @related  CF_Quat cf_quat_slerp cf_quat_norm cf_lerp
 */
CF_INLINE CF_Quat cf_quat_nlerp(CF_Quat a, CF_Quat b, float t)
{
	float d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	if (d < 0) b = cf_quat(-b.x, -b.y, -b.z, -b.w);
	return cf_quat_norm(cf_quat(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t));
}

/**
 * @function cf_quat_from_to
 * @category math
 * @brief    Returns the shortest rotation taking direction `from` onto direction `to`.
 * @param    from  The starting direction. Need not be unit length.
 * @param    to    The ending direction. Need not be unit length.
 * @remarks  The workhorse of aiming: point a turret at a target, align a decal to a surface
 *           normal, tilt a character to a slope. Exactly-opposed directions have no unique
 *           answer, so a perpendicular axis is chosen arbitrarily.
 * @related  CF_Quat cf_quat_from_axis_angle cf_quat_slerp cf_quat_from_basis
 */
CF_INLINE CF_Quat cf_quat_from_to(CF_V3 from, CF_V3 to)
{
	CF_V3 a = cf_safe_norm_v3(from);
	CF_V3 b = cf_safe_norm_v3(to);
	float d = cf_dot_v3(a, b);
	if (d >= 1.0f - 1e-6f) return cf_quat_identity();
	if (d <= -1.0f + 1e-6f) {
		// Opposed: any perpendicular axis is a valid 180-degree turn. Cross with whichever
		// cardinal axis `a` leans on least, so the cross never degenerates.
		CF_V3 axis = CF_FABSF(a.x) < 0.9f ? cf_v3(1, 0, 0) : cf_v3(0, 1, 0);
		return cf_quat_from_axis_angle(cf_norm_v3(cf_cross_v3(a, axis)), CF_PI);
	}
	CF_V3 c = cf_cross_v3(a, b);
	// The half-angle form: (cross, 1 + dot) normalized is the half-way rotation.
	return cf_quat_norm(cf_quat(c.x, c.y, c.z, 1.0f + d));
}

/**
 * @function cf_quat_from_basis
 * @category math
 * @brief    Builds a rotation from three orthonormal basis vectors (the rotation's columns).
 * @param    x  The direction the +x axis maps to.
 * @param    y  The direction the +y axis maps to.
 * @param    z  The direction the +z axis maps to.
 * @remarks  Assumes a right-handed orthonormal basis; a mirrored (negative determinant) basis
 *           has no quaternion and produces garbage. Use `cf_quat_from_m4` to pull the rotation
 *           out of a full transform instead.
 * @related  CF_Quat cf_quat_from_m4 cf_quat_from_to cf_quat_to_m4
 */
CF_INLINE CF_Quat cf_quat_from_basis(CF_V3 x, CF_V3 y, CF_V3 z)
{
	// Shepperd's method: pick the largest of the four possible divisors so the square root
	// never lands near zero.
	float trace = x.x + y.y + z.z;
	CF_Quat q;
	if (trace > 0) {
		float s = CF_SQRTF(trace + 1.0f) * 2.0f;
		q = cf_quat((y.z - z.y) / s, (z.x - x.z) / s, (x.y - y.x) / s, 0.25f * s);
	} else if (x.x > y.y && x.x > z.z) {
		float s = CF_SQRTF(1.0f + x.x - y.y - z.z) * 2.0f;
		q = cf_quat(0.25f * s, (y.x + x.y) / s, (z.x + x.z) / s, (y.z - z.y) / s);
	} else if (y.y > z.z) {
		float s = CF_SQRTF(1.0f + y.y - x.x - z.z) * 2.0f;
		q = cf_quat((y.x + x.y) / s, 0.25f * s, (z.y + y.z) / s, (z.x - x.z) / s);
	} else {
		float s = CF_SQRTF(1.0f + z.z - x.x - y.y) * 2.0f;
		q = cf_quat((z.x + x.z) / s, (z.y + y.z) / s, 0.25f * s, (x.y - y.x) / s);
	}
	return cf_quat_norm(q);
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
 * @function cf_quat_from_m4
 * @category math
 * @brief    Extracts the rotation from a transform matrix.
 * @param    m  A transform whose upper 3x3 is a rotation, optionally with positive scale.
 * @remarks  Scale is divided out per column, so this reads the rotation off a composed
 *           translate-rotate-scale transform. A mirrored (negative determinant) transform has no
 *           rotation to extract and produces garbage -- see `cf_m4_decompose`, which reports the
 *           flip in its scale.
 * @related  CF_Quat CF_M4x4 cf_quat_to_m4 cf_quat_from_basis cf_m4_decompose
 */
CF_INLINE CF_Quat cf_quat_from_m4(CF_M4x4 m)
{
	CF_V3 x = cf_safe_norm_v3(cf_v3(m.elements[0], m.elements[1], m.elements[2]));
	CF_V3 y = cf_safe_norm_v3(cf_v3(m.elements[4], m.elements[5], m.elements[6]));
	CF_V3 z = cf_safe_norm_v3(cf_v3(m.elements[8], m.elements[9], m.elements[10]));
	return cf_quat_from_basis(x, y, z);
}

/**
 * @function cf_m4_decompose
 * @category math
 * @brief    Splits a transform matrix into translation, rotation, and scale.
 * @param    m            The transform to decompose.
 * @param    translation  Written with the translation. May be NULL.
 * @param    rotation     Written with the rotation. May be NULL.
 * @param    scale        Written with the per-axis scale. May be NULL.
 * @remarks  Assumes `m` is a translate-rotate-scale composition with no shear (skew silently
 *           folds into the rotation). A mirrored transform reports a negative x scale, since a
 *           flip cannot live in the quaternion. The inverse of `cf_m4_from_trs` -- useful for
 *           pulling a bone's pose out of a world matrix, or blending between authored
 *           transforms.
 * @related  CF_M4x4 CF_Quat cf_m4_from_trs cf_quat_from_m4 cf_quat_to_m4 cf_m4_translate cf_m4_scale
 */
CF_INLINE void cf_m4_decompose(CF_M4x4 m, CF_V3* translation, CF_Quat* rotation, CF_V3* scale)
{
	if (translation) *translation = cf_v3(m.elements[12], m.elements[13], m.elements[14]);
	CF_V3 x = cf_v3(m.elements[0], m.elements[1], m.elements[2]);
	CF_V3 y = cf_v3(m.elements[4], m.elements[5], m.elements[6]);
	CF_V3 z = cf_v3(m.elements[8], m.elements[9], m.elements[10]);
	float sx = cf_len_v3(x), sy = cf_len_v3(y), sz = cf_len_v3(z);
	// A negative determinant means one axis is mirrored. Which one is unknowable, so fold
	// the flip into x by convention -- the same choice glTF importers make.
	if (cf_dot_v3(cf_cross_v3(x, y), z) < 0) sx = -sx;
	if (scale) *scale = cf_v3(sx, sy, sz);
	if (rotation) {
		*rotation = cf_quat_from_basis(
			sx != 0 ? cf_div_v3_f(x, sx) : cf_v3(1, 0, 0),
			sy != 0 ? cf_div_v3_f(y, sy) : cf_v3(0, 1, 0),
			sz != 0 ? cf_div_v3_f(z, sz) : cf_v3(0, 0, 1));
	}
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
 * @function cf_m4_from_trs
 * @category math
 * @brief    Composes translation, rotation, and scale into a transform matrix.
 * @param    translation  The translation.
 * @param    rotation     The rotation. Should be unit length.
 * @param    scale        The per-axis scale.
 * @return   Returns the matrix `cf_m4_translate(t) * cf_quat_to_m4(r) * cf_m4_scale(s)`.
 * @remarks  The inverse of `cf_m4_decompose`, computed directly with no matrix multiplies --
 *           the usual way to build an object's or bone's world matrix from its pose.
 * @related  CF_M4x4 CF_Quat cf_m4_decompose cf_quat_to_m4 cf_m4_translate cf_m4_scale
 */
CF_INLINE CF_M4x4 cf_m4_from_trs(CF_V3 translation, CF_Quat rotation, CF_V3 scale)
{
	CF_M4x4 m = cf_quat_to_m4(rotation);
	m.elements[0] *= scale.x; m.elements[1] *= scale.x; m.elements[2] *= scale.x;
	m.elements[4] *= scale.y; m.elements[5] *= scale.y; m.elements[6] *= scale.y;
	m.elements[8] *= scale.z; m.elements[9] *= scale.z; m.elements[10] *= scale.z;
	m.elements[12] = translation.x; m.elements[13] = translation.y; m.elements[14] = translation.z;
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
// Geometry: rays, boxes, spheres, planes, and the frustum, with the queries a 3d game
// actually asks -- "what did the mouse hit" (cf_draw3d_unproject's back half) and "is this
// on screen" (frustum culling). Mirrors cute_math.h's 2d shapes: CF_Ray3/CF_Raycast3 follow
// CF_Ray/CF_Raycast, CF_Aabb3 follows CF_Aabb, CF_Sphere follows CF_Circle.

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Ray3
 * @category math
 * @brief    A 3d ray: a directional line segment from `p` along unit direction `d` for distance `t`.
 * @related  CF_Ray3 CF_Raycast3 cf_make_ray3 cf_ray3_to_aabb3 cf_ray3_to_sphere cf_ray3_to_plane3 cf_draw3d_unproject
 */
typedef struct CF_Ray3
{
	/* @member Position. */
	CF_V3 p;

	/* @member Direction (normalized). */
	CF_V3 d;

	/* @member Distance along d from position p to find endpoint of ray. */
	float t;
} CF_Ray3;
// @end

/**
 * @struct   CF_Raycast3
 * @category math
 * @brief    The results for a 3d raycast query.
 * @related  CF_Ray3 cf_ray3_to_aabb3 cf_ray3_to_sphere cf_ray3_to_plane3
 */
typedef struct CF_Raycast3
{
	/* @member True if the ray hit. When this is false then members t and n are zero'd out. */
	bool hit;

	/* @member Time of impact, as a distance along the ray's direction. */
	float t;

	/* @member Normal of surface at impact (unit length). */
	CF_V3 n;
} CF_Raycast3;
// @end

/**
 * @struct   CF_Aabb3
 * @category math
 * @brief    A 3d axis-aligned bounding box.
 * @related  CF_Aabb3 cf_make_aabb3 cf_make_aabb3_center cf_aabb3_to_aabb3 cf_ray3_to_aabb3 cf_frustum_test_aabb3 cf_transform_aabb3
 */
typedef struct CF_Aabb3
{
	/* @member The minimum corner. */
	CF_V3 min;

	/* @member The maximum corner. */
	CF_V3 max;
} CF_Aabb3;
// @end

/**
 * @struct   CF_Sphere
 * @category math
 * @brief    A sphere.
 * @related  CF_Sphere cf_make_sphere cf_sphere_to_sphere cf_sphere_to_aabb3 cf_ray3_to_sphere cf_frustum_test_sphere
 */
typedef struct CF_Sphere
{
	/* @member Position. */
	CF_V3 p;

	/* @member Radius. */
	float r;
} CF_Sphere;
// @end

/**
 * @struct   CF_Plane3
 * @category math
 * @brief    A 3d plane in normal-distance form: points x on the plane satisfy `dot(n, x) == d`.
 * @remarks  The 3d analog of `CF_Halfspace`. The halfspace `dot(n, x) >= d` is the "inside" for
 *           frustum planes.
 * @related  CF_Plane3 cf_make_plane3 cf_plane3_at cf_distance_plane3 cf_project_plane3 cf_ray3_to_plane3 CF_Frustum
 */
typedef struct CF_Plane3
{
	/* @member The plane's normal vector (unit length). */
	CF_V3 n;

	/* @member Distance to origin along n; d = dot(n, p) for any point p on the plane. */
	float d;
} CF_Plane3;
// @end

/**
 * @struct   CF_Frustum
 * @category math
 * @brief    Six planes bounding a camera's visible volume, normals pointing inward.
 * @remarks  Extract one from a combined view-projection with `cf_frustum_from_m4`, then cull
 *           with `cf_frustum_test_aabb3` / `cf_frustum_test_sphere`.
 * @related  CF_Frustum cf_frustum_from_m4 cf_frustum_test_aabb3 cf_frustum_test_sphere CF_Plane3
 */
typedef struct CF_Frustum
{
	/* @member The bounding planes: left, right, bottom, top, near, far. */
	CF_Plane3 planes[6];
} CF_Frustum;
// @end

/**
 * @function cf_make_ray3
 * @category math
 * @brief    Returns a ray from `p` toward `q`, spanning exactly the distance between them.
 * @remarks  Named like `cf_make_sphere`/`cf_make_aabb3`, the other 3d shape constructors.
 * @related  CF_Ray3 cf_ray3_to_aabb3 cf_ray3_to_sphere cf_ray3_to_plane3
 */
CF_INLINE CF_Ray3 cf_make_ray3(CF_V3 p, CF_V3 q)
{
	CF_Ray3 ray;
	CF_V3 delta = cf_sub_v3(q, p);
	float len = cf_len_v3(delta);
	ray.p = p;
	ray.d = len > 1.0e-8f ? cf_div_v3_f(delta, len) : cf_v3(0, 0, 1.0f);
	ray.t = len;
	return ray;
}

/**
 * @function cf_make_aabb3
 * @category math
 * @brief    Returns an AABB from min/max corners.
 * @related  CF_Aabb3 cf_make_aabb3_center cf_center_aabb3 cf_extents_aabb3
 */
CF_INLINE CF_Aabb3 cf_make_aabb3(CF_V3 min, CF_V3 max) { CF_Aabb3 bb; bb.min = min; bb.max = max; return bb; }

/**
 * @function cf_make_aabb3_center
 * @category math
 * @brief    Returns an AABB from a center and half-extents.
 * @related  CF_Aabb3 cf_make_aabb3 cf_center_aabb3 cf_extents_aabb3
 */
CF_INLINE CF_Aabb3 cf_make_aabb3_center(CF_V3 center, CF_V3 half_extents) { return cf_make_aabb3(cf_sub_v3(center, half_extents), cf_add_v3(center, half_extents)); }

/**
 * @function cf_make_sphere
 * @category math
 * @brief    Returns a sphere at `p` with radius `r`.
 * @related  CF_Sphere cf_sphere_to_sphere cf_ray3_to_sphere
 */
CF_INLINE CF_Sphere cf_make_sphere(CF_V3 p, float r) { CF_Sphere s; s.p = p; s.r = r; return s; }

/**
 * @function cf_make_plane3
 * @category math
 * @brief    Returns a plane with normal `n` (normalized for you) at distance `d` along `n`.
 * @remarks  Named `cf_make_*` like `cf_make_sphere`, the other 3d shape constructors.
 * @related  CF_Plane3 cf_plane3_at cf_distance_plane3
 */
CF_INLINE CF_Plane3 cf_make_plane3(CF_V3 n, float d)
{
	CF_Plane3 plane;
	plane.n = cf_norm_v3(n);
	plane.d = d;
	return plane;
}

/**
 * @function cf_plane3_at
 * @category math
 * @brief    Returns the plane with normal `n` (normalized for you) passing through point `p`.
 * @related  CF_Plane3 cf_make_plane3 cf_distance_plane3
 */
CF_INLINE CF_Plane3 cf_plane3_at(CF_V3 n, CF_V3 p)
{
	CF_Plane3 plane;
	plane.n = cf_norm_v3(n);
	plane.d = cf_dot_v3(plane.n, p);
	return plane;
}

/**
 * @function cf_center_aabb3
 * @category math
 * @brief    Returns the center of an AABB.
 * @related  CF_Aabb3 cf_extents_aabb3 cf_half_extents_aabb3
 */
CF_INLINE CF_V3 cf_center_aabb3(CF_Aabb3 bb) { return cf_mul_v3_f(cf_add_v3(bb.min, bb.max), 0.5f); }

/**
 * @function cf_extents_aabb3
 * @category math
 * @brief    Returns the dimensions (full width/height/depth) of an AABB.
 * @related  CF_Aabb3 cf_center_aabb3 cf_half_extents_aabb3
 */
CF_INLINE CF_V3 cf_extents_aabb3(CF_Aabb3 bb) { return cf_sub_v3(bb.max, bb.min); }

/**
 * @function cf_half_extents_aabb3
 * @category math
 * @brief    Returns the half-dimensions of an AABB.
 * @related  CF_Aabb3 cf_center_aabb3 cf_extents_aabb3
 */
CF_INLINE CF_V3 cf_half_extents_aabb3(CF_Aabb3 bb) { return cf_mul_v3_f(cf_sub_v3(bb.max, bb.min), 0.5f); }

/**
 * @function cf_combine_aabb3
 * @category math
 * @brief    Returns the smallest AABB containing both inputs.
 * @remarks  The workhorse for building chunk/scene bounds out of member bounds.
 * @related  CF_Aabb3 cf_expand_aabb3 cf_contains_point_aabb3
 */
CF_INLINE CF_Aabb3 cf_combine_aabb3(CF_Aabb3 a, CF_Aabb3 b)
{
	return cf_make_aabb3(
		cf_v3(cf_min(a.min.x, b.min.x), cf_min(a.min.y, b.min.y), cf_min(a.min.z, b.min.z)),
		cf_v3(cf_max(a.max.x, b.max.x), cf_max(a.max.y, b.max.y), cf_max(a.max.z, b.max.z)));
}

/**
 * @function cf_expand_aabb3
 * @category math
 * @brief    Returns the AABB grown by `radius` on every side.
 * @related  CF_Aabb3 cf_combine_aabb3
 */
CF_INLINE CF_Aabb3 cf_expand_aabb3(CF_Aabb3 bb, float radius)
{
	CF_V3 r = cf_v3(radius, radius, radius);
	return cf_make_aabb3(cf_sub_v3(bb.min, r), cf_add_v3(bb.max, r));
}

/**
 * @function cf_contains_point_aabb3
 * @category math
 * @brief    Returns true if `p` lies inside the AABB (inclusive).
 * @related  CF_Aabb3 cf_aabb3_to_aabb3 cf_closest_point_on_aabb3
 */
CF_INLINE bool cf_contains_point_aabb3(CF_Aabb3 bb, CF_V3 p)
{
	return p.x >= bb.min.x && p.x <= bb.max.x
	    && p.y >= bb.min.y && p.y <= bb.max.y
	    && p.z >= bb.min.z && p.z <= bb.max.z;
}

/**
 * @function cf_closest_point_on_aabb3
 * @category math
 * @brief    Returns the point on (or in) the AABB closest to `p`.
 * @related  CF_Aabb3 cf_contains_point_aabb3 cf_sphere_to_aabb3
 */
CF_INLINE CF_V3 cf_closest_point_on_aabb3(CF_Aabb3 bb, CF_V3 p)
{
	return cf_v3(
		cf_clamp(p.x, bb.min.x, bb.max.x),
		cf_clamp(p.y, bb.min.y, bb.max.y),
		cf_clamp(p.z, bb.min.z, bb.max.z));
}

/**
 * @function cf_distance_plane3
 * @category math
 * @brief    Returns the signed distance from `p` to the plane; positive on the normal's side.
 * @related  CF_Plane3 cf_project_plane3 cf_ray3_to_plane3
 */
CF_INLINE float cf_distance_plane3(CF_Plane3 plane, CF_V3 p) { return cf_dot_v3(plane.n, p) - plane.d; }

/**
 * @function cf_project_plane3
 * @category math
 * @brief    Projects `p` onto the plane (the closest point on the plane).
 * @related  CF_Plane3 cf_distance_plane3
 */
CF_INLINE CF_V3 cf_project_plane3(CF_Plane3 plane, CF_V3 p) { return cf_sub_v3(p, cf_mul_v3_f(plane.n, cf_distance_plane3(plane, p))); }

/**
 * @function cf_aabb3_to_aabb3
 * @category math
 * @brief    Returns true if two AABBs overlap.
 * @related  CF_Aabb3 cf_sphere_to_aabb3 cf_contains_point_aabb3
 */
CF_INLINE bool cf_aabb3_to_aabb3(CF_Aabb3 a, CF_Aabb3 b)
{
	return a.min.x <= b.max.x && a.max.x >= b.min.x
	    && a.min.y <= b.max.y && a.max.y >= b.min.y
	    && a.min.z <= b.max.z && a.max.z >= b.min.z;
}

/**
 * @function cf_sphere_to_sphere
 * @category math
 * @brief    Returns true if two spheres overlap.
 * @related  CF_Sphere cf_sphere_to_aabb3
 */
CF_INLINE bool cf_sphere_to_sphere(CF_Sphere a, CF_Sphere b)
{
	CF_V3 delta = cf_sub_v3(b.p, a.p);
	float r = a.r + b.r;
	return cf_dot_v3(delta, delta) <= r * r;
}

/**
 * @function cf_sphere_to_aabb3
 * @category math
 * @brief    Returns true if a sphere and an AABB overlap.
 * @related  CF_Sphere CF_Aabb3 cf_closest_point_on_aabb3
 */
CF_INLINE bool cf_sphere_to_aabb3(CF_Sphere s, CF_Aabb3 bb)
{
	CF_V3 closest = cf_closest_point_on_aabb3(bb, s.p);
	CF_V3 delta = cf_sub_v3(closest, s.p);
	return cf_dot_v3(delta, delta) <= s.r * s.r;
}

/**
 * @function cf_ray3_to_plane3
 * @category math
 * @brief    Raycasts against a plane.
 * @remarks  Hits from either side; the reported normal faces back along the ray.
 * @related  CF_Ray3 CF_Raycast3 CF_Plane3 cf_ray3_to_aabb3 cf_ray3_to_sphere
 */
CF_INLINE CF_Raycast3 cf_ray3_to_plane3(CF_Ray3 ray, CF_Plane3 plane)
{
	CF_Raycast3 result = { 0 };
	float denom = cf_dot_v3(plane.n, ray.d);
	if (cf_abs(denom) < 1.0e-8f) return result; // Parallel.
	float t = (plane.d - cf_dot_v3(plane.n, ray.p)) / denom;
	if (t < 0 || t > ray.t) return result;
	result.hit = true;
	result.t = t;
	result.n = denom < 0 ? plane.n : cf_neg_v3(plane.n);
	return result;
}

/**
 * @function cf_ray3_to_sphere
 * @category math
 * @brief    Raycasts against a sphere.
 * @remarks  A ray starting inside the sphere reports the exit point.
 * @related  CF_Ray3 CF_Raycast3 CF_Sphere cf_ray3_to_aabb3 cf_ray3_to_plane3
 */
CF_INLINE CF_Raycast3 cf_ray3_to_sphere(CF_Ray3 ray, CF_Sphere sphere)
{
	CF_Raycast3 result = { 0 };
	CF_V3 m = cf_sub_v3(ray.p, sphere.p);
	float b = cf_dot_v3(m, ray.d);
	float c = cf_dot_v3(m, m) - sphere.r * sphere.r;
	if (c > 0 && b > 0) return result;      // Outside, pointing away.
	float disc = b * b - c;
	if (disc < 0) return result;            // Misses.
	float root = CF_SQRTF(disc);
	float t = -b - root;
	if (t < 0) t = -b + root;               // Started inside: exit point.
	if (t < 0 || t > ray.t) return result;
	result.hit = true;
	result.t = t;
	result.n = cf_norm_v3(cf_sub_v3(cf_add_v3(ray.p, cf_mul_v3_f(ray.d, t)), sphere.p));
	return result;
}

/**
 * @function cf_ray3_to_aabb3
 * @category math
 * @brief    Raycasts against an AABB (the classic slab method).
 * @remarks  A ray starting inside the box hits at t = 0 with a zero normal.
 * @related  CF_Ray3 CF_Raycast3 CF_Aabb3 cf_ray3_to_sphere cf_ray3_to_plane3
 */
CF_INLINE CF_Raycast3 cf_ray3_to_aabb3(CF_Ray3 ray, CF_Aabb3 bb)
{
	CF_Raycast3 result = { 0 };
	float tmin = 0;
	float tmax = ray.t;
	int axis = 0;
	float sign = 0;
	const float* p = &ray.p.x;
	const float* d = &ray.d.x;
	const float* lo = &bb.min.x;
	const float* hi = &bb.max.x;
	for (int i = 0; i < 3; ++i) {
		if (cf_abs(d[i]) < 1.0e-8f) {
			if (p[i] < lo[i] || p[i] > hi[i]) return result; // Parallel and outside the slab.
		} else {
			float inv = 1.0f / d[i];
			float t0 = (lo[i] - p[i]) * inv;
			float t1 = (hi[i] - p[i]) * inv;
			float s = -1.0f;
			if (t0 > t1) { float tmp = t0; t0 = t1; t1 = tmp; s = 1.0f; }
			if (t0 > tmin) { tmin = t0; axis = i; sign = s; }
			if (t1 < tmax) tmax = t1;
			if (tmin > tmax) return result;
		}
	}
	result.hit = true;
	result.t = tmin;
	if (sign != 0) {
		float* n = &result.n.x;
		n[axis] = sign;
	}
	return result;
}

/**
 * @function cf_transform_aabb3
 * @category math
 * @brief    Returns the AABB of a box transformed by an affine matrix (Arvo's method).
 * @remarks  The result bounds the transformed box, so repeated transforms accumulate slop --
 *           transform the original local-space AABB by the full matrix each time instead of
 *           chaining.
 * @related  CF_Aabb3 CF_M4x4 cf_frustum_test_aabb3
 */
CF_INLINE CF_Aabb3 cf_transform_aabb3(CF_M4x4 m, CF_Aabb3 bb)
{
	CF_V3 t = cf_v3(m.elements[12], m.elements[13], m.elements[14]);
	CF_Aabb3 out = cf_make_aabb3(t, t);
	float* omin = &out.min.x;
	float* omax = &out.max.x;
	const float* bmin = &bb.min.x;
	const float* bmax = &bb.max.x;
	for (int r = 0; r < 3; ++r) {
		for (int c = 0; c < 3; ++c) {
			float e = m.elements[c * 4 + r] * bmin[c];
			float f = m.elements[c * 4 + r] * bmax[c];
			omin[r] += cf_min(e, f);
			omax[r] += cf_max(e, f);
		}
	}
	return out;
}

/**
 * @function cf_frustum_from_m4
 * @category math
 * @brief    Extracts the six frustum planes from a combined view-projection matrix.
 * @param    m  Typically `projection * view` from the same matrices handed to the 3d camera
 *              stacks (`cf_perspective`/`cf_ortho` times `cf_look_at`).
 * @remarks  Plane normals point inward and are normalized. Uses CF's clip conventions
 *           (clip-space z in [0, 1], matching SDL_GPU and everything `cf_perspective` emits).
 * @related  CF_Frustum cf_frustum_test_aabb3 cf_frustum_test_sphere
 */
CF_INLINE CF_Frustum cf_frustum_from_m4(CF_M4x4 m)
{
	// Gribb-Hartmann: each plane is a combination of matrix rows, adapted for z in [0, 1].
	// Row i of the column-major matrix is (elements[i], elements[4+i], elements[8+i], elements[12+i]).
	CF_V4 rows[4];
	for (int i = 0; i < 4; ++i) {
		rows[i] = cf_v4(m.elements[i], m.elements[4 + i], m.elements[8 + i], m.elements[12 + i]);
	}
	CF_V4 combos[6];
	combos[0] = cf_add_v4(rows[3], rows[0]); // Left:   w + x >= 0
	combos[1] = cf_sub_v4(rows[3], rows[0]); // Right:  w - x >= 0
	combos[2] = cf_add_v4(rows[3], rows[1]); // Bottom: w + y >= 0
	combos[3] = cf_sub_v4(rows[3], rows[1]); // Top:    w - y >= 0
	combos[4] = rows[2];                     // Near:   z >= 0 (CF clips z to [0, 1]).
	combos[5] = cf_sub_v4(rows[3], rows[2]); // Far:    w - z >= 0
	CF_Frustum f;
	for (int i = 0; i < 6; ++i) {
		CF_V3 n = cf_v3(combos[i].x, combos[i].y, combos[i].z);
		float inv_len = 1.0f / cf_len_v3(n);
		f.planes[i].n = cf_mul_v3_f(n, inv_len);
		f.planes[i].d = -combos[i].w * inv_len; // ax+by+cz+w >= 0 becomes dot(n, x) >= d.
	}
	return f;
}

/**
 * @function cf_frustum_test_aabb3
 * @category math
 * @brief    Returns false only when the AABB is fully outside the frustum -- the culling test.
 * @remarks  Conservative: a box outside the frustum but not fully behind any single plane can
 *           still return true (the standard, cheap p-vertex test). Never culls a visible box.
 * @related  CF_Frustum cf_frustum_from_m4 cf_frustum_test_sphere cf_transform_aabb3
 */
CF_INLINE bool cf_frustum_test_aabb3(const CF_Frustum* f, CF_Aabb3 bb)
{
	for (int i = 0; i < 6; ++i) {
		CF_V3 n = f->planes[i].n;
		// The p-vertex: the box corner farthest along the plane normal.
		CF_V3 p = cf_v3(
			n.x >= 0 ? bb.max.x : bb.min.x,
			n.y >= 0 ? bb.max.y : bb.min.y,
			n.z >= 0 ? bb.max.z : bb.min.z);
		if (cf_dot_v3(n, p) < f->planes[i].d) return false;
	}
	return true;
}

/**
 * @function cf_frustum_test_sphere
 * @category math
 * @brief    Returns false only when the sphere is fully outside the frustum.
 * @related  CF_Frustum cf_frustum_from_m4 cf_frustum_test_aabb3
 */
CF_INLINE bool cf_frustum_test_sphere(const CF_Frustum* f, CF_Sphere s)
{
	for (int i = 0; i < 6; ++i) {
		if (cf_distance_plane3(f->planes[i], s.p) < -s.r) return false;
	}
	return true;
}

//--------------------------------------------------------------------------------------------------
// Stateless collision queries -- the 3d mirror of cute_math.h's 2d kit, powered by Box3D's
// freestanding geometry layer (no physics world involved). Shapes are world-space; manifolds
// carry the depth + normal a character controller resolves with, and the triangle receivers
// are how you collide against your own level geometry one triangle at a time.

/**
 * @struct   CF_Capsule3
 * @category collision
 * @brief    A 3d capsule: two sphere caps connected by a cylinder.
 * @remarks  The classic character-controller shape.
 * @related  CF_Capsule3 cf_make_capsule3 cf_capsule3_to_capsule3_manifold cf_ray3_to_capsule3 cf_sphere_to_capsule3
 */
typedef struct CF_Capsule3
{
	/* @member The center of one cap. */
	CF_V3 a;

	/* @member The center of the other cap. */
	CF_V3 b;

	/* @member The radius about the rod from `a` to `b`. */
	float r;
} CF_Capsule3;
// @end

/**
 * @function cf_make_capsule3
 * @category collision
 * @brief    Returns a `CF_Capsule3`.
 * @related  CF_Capsule3 cf_make_sphere cf_make_aabb3
 */
CF_INLINE CF_Capsule3 cf_make_capsule3(CF_V3 a, CF_V3 b, float r) { CF_Capsule3 c; c.a = a; c.b = b; c.r = r; return c; }

/**
 * @struct   CF_Triangle3
 * @category collision
 * @brief    A single 3d triangle, counter-clockwise winding.
 * @remarks  The triangle queries treat the triangle as one-sided level geometry: the face
 *           normal follows the counter-clockwise winding of `a`, `b`, `c`. Collide gameplay
 *           shapes against your level mesh one (nearby) triangle at a time.
 * @related  CF_Triangle3 cf_make_triangle3 cf_sphere_to_triangle3_manifold cf_capsule3_to_triangle3_manifold cf_ray3_to_triangle3
 */
typedef struct CF_Triangle3
{
	/* @member The first vertex. */
	CF_V3 a;

	/* @member The second vertex. */
	CF_V3 b;

	/* @member The third vertex. */
	CF_V3 c;
} CF_Triangle3;
// @end

/**
 * @function cf_make_triangle3
 * @category collision
 * @brief    Returns a `CF_Triangle3`.
 * @related  CF_Triangle3 cf_make_capsule3
 */
CF_INLINE CF_Triangle3 cf_make_triangle3(CF_V3 a, CF_V3 b, CF_V3 c) { CF_Triangle3 t; t.a = a; t.b = b; t.c = c; return t; }

/**
 * @function CF_MANIFOLD3_MAX_POINTS
 * @category collision
 * @brief    The maximum number of contact points in a `CF_Manifold3`.
 * @related  CF_Manifold3
 */
#define CF_MANIFOLD3_MAX_POINTS 4

/**
 * @struct   CF_Manifold3
 * @category collision
 * @brief    Contains all information necessary to resolve a 3d collision.
 * @remarks  The 3d analog of `CF_Manifold`. A manifold only ever describes shapes that
 *           actually touch: `count` is zero for separated shapes, and every reported point
 *           has a non-negative depth. To separate the shapes, move them apart along `n` by
 *           the largest depth.
 * @related  CF_Manifold3 cf_sphere_to_sphere_manifold cf_aabb3_to_aabb3_manifold cf_capsule3_to_capsule3_manifold cf_collide3
 */
typedef struct CF_Manifold3
{
	/* @member The number of points in the manifold (0 to `CF_MANIFOLD3_MAX_POINTS`). */
	int count;

	/* @member The collision depth of each point in the manifold. */
	float depths[CF_MANIFOLD3_MAX_POINTS];

	/* @member Contact points, on the contact plane between the shapes. */
	CF_V3 contact_points[CF_MANIFOLD3_MAX_POINTS];

	/* @member Always points from shape A to shape B. */
	CF_V3 n;
} CF_Manifold3;
// @end

/**
 * @enum     CF_ShapeType3
 * @category collision
 * @brief    The types of 3d shapes the generic collision functions accept.
 * @related  CF_ShapeType3 cf_collide3 cf_collided3 cf_cast_ray3 cf_gjk3 cf_toi3
 */
#define CF_SHAPE3_TYPE_DEFS \
	/* @entry Unknown shape type. */      \
	CF_ENUM(SHAPE3_TYPE_NONE,     0) \
	/* @entry `CF_Sphere` shape type. */  \
	CF_ENUM(SHAPE3_TYPE_SPHERE,   1) \
	/* @entry `CF_Aabb3` shape type. */   \
	CF_ENUM(SHAPE3_TYPE_AABB,     2) \
	/* @entry `CF_Capsule3` shape type. */\
	CF_ENUM(SHAPE3_TYPE_CAPSULE,  3) \
	/* @entry `CF_Triangle3` shape type. */\
	CF_ENUM(SHAPE3_TYPE_TRIANGLE, 4) \
	/* @end */

typedef enum CF_ShapeType3
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_SHAPE3_TYPE_DEFS
	#undef CF_ENUM
} CF_ShapeType3;

/**
 * @function cf_shape_type3_to_string
 * @category collision
 * @brief    Returns a `CF_ShapeType3` as a string.
 * @related  CF_ShapeType3
 */
CF_INLINE const char* cf_shape_type3_to_string(CF_ShapeType3 type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_SHAPE3_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @struct   CF_GjkCache3
 * @category collision
 * @brief    An opaque warm-start cache for successive `cf_gjk3` calls on the same shape pair.
 * @remarks  Zero-initialize before the first call, then reuse across calls while the shapes
 *           move only a little relative to each other.
 * @related  CF_GjkCache3 cf_gjk3
 */
typedef struct CF_GjkCache3
{
	/* @member A metric used to judge whether the cached simplex is still usable. */
	float metric;

	/* @member The number of cached simplex vertices. Zero-initialize before the first call. */
	uint16_t count;

	/* @member Cached simplex indices on shape A. */
	uint8_t iA[4];

	/* @member Cached simplex indices on shape B. */
	uint8_t iB[4];
} CF_GjkCache3;
// @end

/**
 * @struct   CF_ToiResult3
 * @category collision
 * @brief    Results of a 3d time of impact calculation from `cf_toi3`.
 * @related  CF_ToiResult3 cf_toi3
 */
typedef struct CF_ToiResult3
{
	/* @member 1 if the shapes hit during the sweep, 0 if they never hit. */
	int hit;

	/* @member The time of impact, from 0 to 1. 1 when no hit occurred. */
	float toi;

	/* @member Surface normal from shape A to B at the time of impact. */
	CF_V3 n;

	/* @member Point of contact at the time of impact. */
	CF_V3 p;

	/*  @member Number of iterations the solver underwent. */
	int iterations;
} CF_ToiResult3;
// @end

/**
 * @function cf_sphere_to_capsule3
 * @category collision
 * @brief    Returns true if a sphere and a capsule intersect.
 * @related  CF_Sphere CF_Capsule3 cf_sphere_to_capsule3_manifold cf_sphere_to_sphere cf_sphere_to_aabb3
 */
CF_API bool CF_CALL cf_sphere_to_capsule3(CF_Sphere A, CF_Capsule3 B);

/**
 * @function cf_aabb3_to_capsule3
 * @category collision
 * @brief    Returns true if an AABB and a capsule intersect.
 * @related  CF_Aabb3 CF_Capsule3 cf_aabb3_to_capsule3_manifold cf_aabb3_to_aabb3
 */
CF_API bool CF_CALL cf_aabb3_to_capsule3(CF_Aabb3 A, CF_Capsule3 B);

/**
 * @function cf_capsule3_to_capsule3
 * @category collision
 * @brief    Returns true if two capsules intersect.
 * @related  CF_Capsule3 cf_capsule3_to_capsule3_manifold
 */
CF_API bool CF_CALL cf_capsule3_to_capsule3(CF_Capsule3 A, CF_Capsule3 B);

/**
 * @function cf_sphere_to_triangle3
 * @category collision
 * @brief    Returns true if a sphere touches a triangle.
 * @related  CF_Sphere CF_Triangle3 cf_sphere_to_triangle3_manifold
 */
CF_API bool CF_CALL cf_sphere_to_triangle3(CF_Sphere A, CF_Triangle3 B);

/**
 * @function cf_aabb3_to_triangle3
 * @category collision
 * @brief    Returns true if an AABB touches a triangle.
 * @related  CF_Aabb3 CF_Triangle3 cf_aabb3_to_triangle3_manifold
 */
CF_API bool CF_CALL cf_aabb3_to_triangle3(CF_Aabb3 A, CF_Triangle3 B);

/**
 * @function cf_capsule3_to_triangle3
 * @category collision
 * @brief    Returns true if a capsule touches a triangle.
 * @related  CF_Capsule3 CF_Triangle3 cf_capsule3_to_triangle3_manifold
 */
CF_API bool CF_CALL cf_capsule3_to_triangle3(CF_Capsule3 A, CF_Triangle3 B);

/**
 * @function cf_sphere_to_sphere_manifold
 * @category collision
 * @brief    Computes contact information for two spheres.
 * @related  CF_Manifold3 CF_Sphere cf_sphere_to_sphere
 */
CF_API CF_Manifold3 CF_CALL cf_sphere_to_sphere_manifold(CF_Sphere A, CF_Sphere B);

/**
 * @function cf_sphere_to_aabb3_manifold
 * @category collision
 * @brief    Computes contact information for a sphere and an AABB.
 * @related  CF_Manifold3 CF_Sphere CF_Aabb3 cf_sphere_to_aabb3
 */
CF_API CF_Manifold3 CF_CALL cf_sphere_to_aabb3_manifold(CF_Sphere A, CF_Aabb3 B);

/**
 * @function cf_sphere_to_capsule3_manifold
 * @category collision
 * @brief    Computes contact information for a sphere and a capsule.
 * @related  CF_Manifold3 CF_Sphere CF_Capsule3 cf_sphere_to_capsule3
 */
CF_API CF_Manifold3 CF_CALL cf_sphere_to_capsule3_manifold(CF_Sphere A, CF_Capsule3 B);

/**
 * @function cf_aabb3_to_aabb3_manifold
 * @category collision
 * @brief    Computes contact information for two AABBs.
 * @related  CF_Manifold3 CF_Aabb3 cf_aabb3_to_aabb3
 */
CF_API CF_Manifold3 CF_CALL cf_aabb3_to_aabb3_manifold(CF_Aabb3 A, CF_Aabb3 B);

/**
 * @function cf_aabb3_to_capsule3_manifold
 * @category collision
 * @brief    Computes contact information for an AABB and a capsule.
 * @related  CF_Manifold3 CF_Aabb3 CF_Capsule3 cf_aabb3_to_capsule3
 */
CF_API CF_Manifold3 CF_CALL cf_aabb3_to_capsule3_manifold(CF_Aabb3 A, CF_Capsule3 B);

/**
 * @function cf_capsule3_to_capsule3_manifold
 * @category collision
 * @brief    Computes contact information for two capsules.
 * @related  CF_Manifold3 CF_Capsule3 cf_capsule3_to_capsule3
 */
CF_API CF_Manifold3 CF_CALL cf_capsule3_to_capsule3_manifold(CF_Capsule3 A, CF_Capsule3 B);

/**
 * @function cf_sphere_to_triangle3_manifold
 * @category collision
 * @brief    Computes contact information for a sphere against a one-sided triangle.
 * @remarks  The workhorse of "collide my character against the level mesh": gather nearby
 *           triangles, manifold each, resolve along the deepest normals (or feed the planes
 *           to Box3D's `b3SolvePlanes` mover -- see the Physics topic).
 * @related  CF_Manifold3 CF_Sphere CF_Triangle3 cf_capsule3_to_triangle3_manifold cf_aabb3_to_triangle3_manifold
 */
CF_API CF_Manifold3 CF_CALL cf_sphere_to_triangle3_manifold(CF_Sphere A, CF_Triangle3 B);

/**
 * @function cf_aabb3_to_triangle3_manifold
 * @category collision
 * @brief    Computes contact information for an AABB against a one-sided triangle.
 * @related  CF_Manifold3 CF_Aabb3 CF_Triangle3 cf_sphere_to_triangle3_manifold
 */
CF_API CF_Manifold3 CF_CALL cf_aabb3_to_triangle3_manifold(CF_Aabb3 A, CF_Triangle3 B);

/**
 * @function cf_capsule3_to_triangle3_manifold
 * @category collision
 * @brief    Computes contact information for a capsule against a one-sided triangle.
 * @related  CF_Manifold3 CF_Capsule3 CF_Triangle3 cf_sphere_to_triangle3_manifold
 */
CF_API CF_Manifold3 CF_CALL cf_capsule3_to_triangle3_manifold(CF_Capsule3 A, CF_Triangle3 B);

/**
 * @function cf_ray3_to_capsule3
 * @category collision
 * @brief    Casts a ray against a capsule.
 * @related  CF_Ray3 CF_Capsule3 CF_Raycast3 cf_ray3_to_sphere cf_ray3_to_aabb3 cf_ray3_to_triangle3
 */
CF_API CF_Raycast3 CF_CALL cf_ray3_to_capsule3(CF_Ray3 A, CF_Capsule3 B);

/**
 * @function cf_ray3_to_triangle3
 * @category collision
 * @brief    Casts a ray against a (two-sided) triangle.
 * @remarks  The missing half of mouse picking against meshes: unproject a ray with
 *           `cf_draw3d_unproject`, then cast it against your triangles. The reported normal
 *           faces back along the ray.
 * @related  CF_Ray3 CF_Triangle3 CF_Raycast3 cf_ray3_to_aabb3 cf_draw3d_unproject
 */
CF_API CF_Raycast3 CF_CALL cf_ray3_to_triangle3(CF_Ray3 A, CF_Triangle3 B);

/**
 * @function cf_gjk3
 * @category collision
 * @brief    Returns the distance between two 3d shapes, and their closest points.
 * @param    A           The first shape.
 * @param    typeA       The `CF_ShapeType3` of the first shape `A`.
 * @param    B           The second shape.
 * @param    typeB       The `CF_ShapeType3` of the second shape `B`.
 * @param    outA        The closest point on `A` to `B`. Not well defined when intersecting. May be `NULL`.
 * @param    outB        The closest point on `B` to `A`. Not well defined when intersecting. May be `NULL`.
 * @param    use_radius  True to use the radius of any `CF_Sphere`/`CF_Capsule3` inputs, false to treat them as a point/segment.
 * @param    iterations  Can be `NULL`. The number of internal GJK iterations, for debugging.
 * @param    cache       Can be `NULL`. An optional warm-start cache, see `CF_GjkCache3`.
 * @return   Returns the distance between the two shapes; zero when overlapped.
 * @related  CF_GjkCache3 CF_ShapeType3 cf_toi3 cf_gjk
 */
CF_API float CF_CALL cf_gjk3(const void* A, CF_ShapeType3 typeA, const void* B, CF_ShapeType3 typeB, CF_V3* outA, CF_V3* outB, bool use_radius, int* iterations, CF_GjkCache3* cache);

/**
 * @function cf_toi3
 * @category collision
 * @brief    Computes the time of impact of two linearly moving 3d shapes.
 * @param    A           The first shape.
 * @param    typeA       The `CF_ShapeType3` of the first shape `A`.
 * @param    vA          The velocity of `A` over the sweep.
 * @param    B           The second shape.
 * @param    typeB       The `CF_ShapeType3` of the second shape `B`.
 * @param    vB          The velocity of `B` over the sweep.
 * @param    use_radius  True to use the radius of any `CF_Sphere`/`CF_Capsule3` inputs, false to treat them as a point/segment.
 * @return   Returns a `CF_ToiResult3`; multiply the velocities by `toi` to move to the impact configuration.
 * @remarks  Same contract as the 2d `cf_toi`: no rotation over the sweep, and shapes that
 *           start out already touching report a miss with `toi` 1 -- sweep from a
 *           non-overlapping configuration (inflate radii slightly, resolve, then sweep).
 * @related  CF_ToiResult3 CF_ShapeType3 cf_gjk3 cf_toi
 */
CF_API CF_ToiResult3 CF_CALL cf_toi3(const void* A, CF_ShapeType3 typeA, CF_V3 vA, const void* B, CF_ShapeType3 typeB, CF_V3 vB, bool use_radius);

/**
 * @function cf_collided3
 * @category collision
 * @brief    Returns true if two 3d shapes collided, using `void*` + `CF_ShapeType3` polymorphism.
 * @related  CF_ShapeType3 cf_collide3 cf_collided
 */
CF_API int CF_CALL cf_collided3(const void* A, CF_ShapeType3 typeA, const void* B, CF_ShapeType3 typeB);

/**
 * @function cf_collide3
 * @category collision
 * @brief    Computes a `CF_Manifold3` for two 3d shapes, using `void*` + `CF_ShapeType3` polymorphism.
 * @remarks  Triangle-vs-triangle pairs are not supported and yield an empty manifold.
 * @related  CF_ShapeType3 CF_Manifold3 cf_collided3 cf_collide
 */
CF_API void CF_CALL cf_collide3(const void* A, CF_ShapeType3 typeA, const void* B, CF_ShapeType3 typeB, CF_Manifold3* m);

/**
 * @function cf_cast_ray3
 * @category collision
 * @brief    Casts a ray against a generic 3d shape, using `void*` + `CF_ShapeType3` polymorphism.
 * @related  CF_ShapeType3 CF_Ray3 CF_Raycast3 cf_ray3_to_capsule3 cf_ray3_to_triangle3
 */
CF_API bool CF_CALL cf_cast_ray3(CF_Ray3 A, const void* B, CF_ShapeType3 typeB, CF_Raycast3* out);

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
using Ray3 = CF_Ray3;
using Raycast3 = CF_Raycast3;
using Aabb3 = CF_Aabb3;
using Sphere = CF_Sphere;
using Plane3 = CF_Plane3;
using Frustum = CF_Frustum;
using Capsule3 = CF_Capsule3;
using Triangle3 = CF_Triangle3;
using Manifold3 = CF_Manifold3;
using ToiResult3 = CF_ToiResult3;
using GjkCache3 = CF_GjkCache3;
using ShapeType3 = CF_ShapeType3;

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
CF_INLINE v3 reflect(v3 a, v3 n) { return cf_reflect_v3(a, n); }
CF_INLINE v3 project(v3 a, v3 b) { return cf_project_v3(a, b); }
CF_INLINE v3 slide(v3 a, v3 n) { return cf_slide_v3(a, n); }
CF_INLINE void orthonormal_basis(v3 n, v3* x_out, v3* y_out) { cf_orthonormal_basis(n, x_out, y_out); }
CF_INLINE v4 v4_from_v3(v3 a, float w) { return cf_v4_from_v3(a, w); }
CF_INLINE v3 xyz(v4 a) { return cf_xyz(a); }

CF_INLINE quat quat_identity() { return cf_quat_identity(); }
CF_INLINE quat quat_from_axis_angle(v3 axis, float radians) { return cf_quat_from_axis_angle(axis, radians); }
CF_INLINE void quat_to_axis_angle(quat q, v3* axis_out, float* angle_out) { cf_quat_to_axis_angle(q, axis_out, angle_out); }
CF_INLINE quat norm(quat q) { return cf_quat_norm(q); }
CF_INLINE quat conjugate(quat q) { return cf_quat_conjugate(q); }
CF_INLINE quat inverse(quat q) { return cf_quat_inverse(q); }
CF_INLINE quat slerp(quat a, quat b, float t) { return cf_quat_slerp(a, b, t); }
CF_INLINE quat nlerp(quat a, quat b, float t) { return cf_quat_nlerp(a, b, t); }
CF_INLINE quat quat_from_to(v3 from, v3 to) { return cf_quat_from_to(from, to); }
CF_INLINE quat quat_from_basis(v3 x, v3 y, v3 z) { return cf_quat_from_basis(x, y, z); }
CF_INLINE quat quat_from_m4(m4 m) { return cf_quat_from_m4(m); }
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
CF_INLINE m4 m4_from_trs(v3 translation, quat rotation, v3 scale) { return cf_m4_from_trs(translation, rotation, scale); }
CF_INLINE void m4_decompose(m4 m, v3* translation, quat* rotation, v3* scale) { cf_m4_decompose(m, translation, rotation, scale); }
CF_INLINE v3 transform_point(m4 m, v3 p) { return cf_m4_transform_point(m, p); }
CF_INLINE v3 transform_dir(m4 m, v3 d) { return cf_m4_transform_dir(m, d); }

CF_INLINE m4 ortho(float left, float right, float bottom, float top, float znear, float zfar) { return cf_ortho(left, right, bottom, top, znear, zfar); }
CF_INLINE m4 perspective(float fov_radians, float aspect, float znear, float zfar) { return cf_perspective(fov_radians, aspect, znear, zfar); }
CF_INLINE m4 look_at(v3 eye, v3 target, v3 up) { return cf_look_at(eye, target, up); }

CF_INLINE Capsule3 make_capsule3(v3 a, v3 b, float r) { return cf_make_capsule3(a, b, r); }
CF_INLINE Triangle3 make_triangle3(v3 a, v3 b, v3 c) { return cf_make_triangle3(a, b, c); }
CF_INLINE bool sphere_to_capsule3(Sphere A, Capsule3 B) { return cf_sphere_to_capsule3(A, B); }
CF_INLINE bool aabb3_to_capsule3(Aabb3 A, Capsule3 B) { return cf_aabb3_to_capsule3(A, B); }
CF_INLINE bool capsule3_to_capsule3(Capsule3 A, Capsule3 B) { return cf_capsule3_to_capsule3(A, B); }
CF_INLINE bool sphere_to_triangle3(Sphere A, Triangle3 B) { return cf_sphere_to_triangle3(A, B); }
CF_INLINE bool aabb3_to_triangle3(Aabb3 A, Triangle3 B) { return cf_aabb3_to_triangle3(A, B); }
CF_INLINE bool capsule3_to_triangle3(Capsule3 A, Triangle3 B) { return cf_capsule3_to_triangle3(A, B); }
CF_INLINE Manifold3 sphere_to_sphere_manifold(Sphere A, Sphere B) { return cf_sphere_to_sphere_manifold(A, B); }
CF_INLINE Manifold3 sphere_to_aabb3_manifold(Sphere A, Aabb3 B) { return cf_sphere_to_aabb3_manifold(A, B); }
CF_INLINE Manifold3 sphere_to_capsule3_manifold(Sphere A, Capsule3 B) { return cf_sphere_to_capsule3_manifold(A, B); }
CF_INLINE Manifold3 aabb3_to_aabb3_manifold(Aabb3 A, Aabb3 B) { return cf_aabb3_to_aabb3_manifold(A, B); }
CF_INLINE Manifold3 aabb3_to_capsule3_manifold(Aabb3 A, Capsule3 B) { return cf_aabb3_to_capsule3_manifold(A, B); }
CF_INLINE Manifold3 capsule3_to_capsule3_manifold(Capsule3 A, Capsule3 B) { return cf_capsule3_to_capsule3_manifold(A, B); }
CF_INLINE Manifold3 sphere_to_triangle3_manifold(Sphere A, Triangle3 B) { return cf_sphere_to_triangle3_manifold(A, B); }
CF_INLINE Manifold3 aabb3_to_triangle3_manifold(Aabb3 A, Triangle3 B) { return cf_aabb3_to_triangle3_manifold(A, B); }
CF_INLINE Manifold3 capsule3_to_triangle3_manifold(Capsule3 A, Triangle3 B) { return cf_capsule3_to_triangle3_manifold(A, B); }
CF_INLINE Raycast3 ray3_to_capsule3(Ray3 A, Capsule3 B) { return cf_ray3_to_capsule3(A, B); }
CF_INLINE Raycast3 ray3_to_triangle3(Ray3 A, Triangle3 B) { return cf_ray3_to_triangle3(A, B); }
CF_INLINE float gjk3(const void* A, ShapeType3 typeA, const void* B, ShapeType3 typeB, v3* outA, v3* outB, bool use_radius, int* iterations, GjkCache3* cache) { return cf_gjk3(A, typeA, B, typeB, outA, outB, use_radius, iterations, cache); }
CF_INLINE ToiResult3 toi3(const void* A, ShapeType3 typeA, v3 vA, const void* B, ShapeType3 typeB, v3 vB, bool use_radius) { return cf_toi3(A, typeA, vA, B, typeB, vB, use_radius); }
CF_INLINE int collided3(const void* A, ShapeType3 typeA, const void* B, ShapeType3 typeB) { return cf_collided3(A, typeA, B, typeB); }
CF_INLINE void collide3(const void* A, ShapeType3 typeA, const void* B, ShapeType3 typeB, Manifold3* m) { cf_collide3(A, typeA, B, typeB, m); }
CF_INLINE bool cast_ray3(Ray3 A, const void* B, ShapeType3 typeB, Raycast3* out) { return cf_cast_ray3(A, B, typeB, out); }

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
