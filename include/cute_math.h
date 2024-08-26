/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_MATH_H
#define CF_MATH_H

#include "cute_defines.h"

#ifndef CF_SQRTF
	#include <math.h>
	#define CF_SQRTF sqrtf
#endif

#ifndef CF_FABSF
	#include <math.h>
	#define CF_FABSF fabsf
#endif

#ifndef CF_SINF
	#include <math.h>
	#define CF_SINF sinf
#endif

#ifndef CF_COSF
	#include <math.h>
	#define CF_COSF cosf
#endif

#ifndef CF_ACOSF
	#include <math.h>
	#define CF_ACOSF acosf
#endif

#ifndef CF_ATAN2F
	#include <math.h>
	#define CF_ATAN2F atan2f
#endif

#ifndef CF_FLOORF
	#include <math.h>
	#define CF_FLOORF floorf
#endif

#ifndef CF_CEILF
	#include <math.h>
	#define CF_CEILF ceilf
#endif

#ifndef CF_ROUNDF
	#include <math.h>
	#define CF_ROUNDF roundf
#endif

#ifndef CF_FMODF
	#include <math.h>
	#define CF_FMODF fmodf
#endif

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_V2
 * @category math
 * @brief    A 2d vector.
 * @remarks  To construct a vector you may use the function `cf_v2`, which is defined like this:
 *           
 *           ```cpp
 *           CF_V2 cf_v2(float x, float y);
 *           ```
 *           
 *           The C++ API uses `V2(x, y)`.
 * @related  CF_V2 cf_add_v2 cf_sub_v2 cf_dot cf_mul_v2_f cf_div_v2_f
 */
typedef struct CF_V2
{
	/* @member The x component. */
	float x;

	/* @member The y component. */
	float y;
} CF_V2;
// @end

CF_INLINE CF_V2 cf_v2(float x, float y)
{
	CF_V2 result;
	result.x = x;
	result.y = y;
	return result;
}

/**
 * @struct   CF_SinCos
 * @category math
 * @brief    Rotation about an axis composed of cos/sin pair.
 * @remarks  You can construct an identity with the `CF_SinCos cf_sin_cos()` function.
 * @related  CF_SinCos cf_sincos cf_sincos_f cf_x_axis cf_y_axis cf_mul_sc_v2 cf_mulT_sc_v2
 */
typedef struct CF_SinCos
{
	/* @member The sin component. */
	float s;

	/* @member The cos component. */
	float c;
} CF_SinCos;
// @end

/**
 * @struct   CF_M2x2
 * @category math
 * @brief    2x2 matrix.
 * @related  CF_M2x2 cf_mul_m2_f cf_mul_m2_v2 cf_mul_m2
 */
typedef struct CF_M2x2
{
	/* @member The x column. */
	CF_V2 x;

	/* @member The y column. */
	CF_V2 y;
} CF_M2x2;
// @end

/**
 * @struct   CF_M3x2
 * @category math
 * @brief    2d transformation.
 * @remarks  Mostly useful for graphics and not physics colliders, since it supports scale.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_rotation cf_make_transform_TSR cf_invert
 */
typedef struct CF_M3x2
{
	/* @member The top-left 2x2 matrix representing scale + rotation. */
	CF_M2x2 m;

	/* @member The position column of the matrix. */
	CF_V2 p;
} CF_M3x2;
// @end

/**
 * @struct   CF_Transform
 * @category math
 * @brief    2d transformation.
 * @remarks  Mostly useful for physics colliders since there's no scale.
 * @related  CF_Transform cf_make_transform cf_make_transform_TR cf_mul_tf_v2 cf_mulT_tf_v2 cf_mul_tf cf_mulT_tf
 */
typedef struct CF_Transform
{
	/* @member The rotation. */
	CF_SinCos r;

	/* @member The position. */
	CF_V2 p;
} CF_Transform;
// @end

/**
 * @struct   CF_Halfspace
 * @category math
 * @brief    2d plane, aka line.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_intersect_halfspace
 */
typedef struct CF_Halfspace
{
	/* @member The plane's normal vector. */
	CF_V2 n;

	/* @member Distance to origin; d = ax + by = dot(n, p). */
	float d;
} CF_Halfspace;
// @end

/**
 * @struct   CF_Ray
 * @category math
 * @brief    A ray.
 * @remarks  A ray is a directional line segment. It starts at an endpoint and extends into another direction for a specified distance (defined by `t`).
 * @related  CF_Ray cf_impact cf_endpoint cf_ray_to_halfspace cf_ray_to_circle cf_ray_to_aabb cf_ray_to_capsule cf_ray_to_poly
 */
typedef struct CF_Ray
{
	/* @member Position. */
	CF_V2 p;

	/* @member Direction (normalized). */
	CF_V2 d;

	/* @member Distance along d from position p to find endpoint of ray. */
	float t;
} CF_Ray;
// @end

/**
 * @struct   CF_Raycast
 * @category math
 * @brief    The results for a raycast query.
 * @related  CF_Raycast cf_ray_to_halfspace cf_ray_to_circle cf_ray_to_aabb cf_ray_to_capsule cf_ray_to_poly
 */
typedef struct CF_Raycast
{
	/* @member True if the ray hit. When this is false then members t and n are zero'd out. */
	bool hit;

	/* @member Time of impact. */
	float t;

	/* @member Normal of surface at impact (unit length). */
	CF_V2 n;
} CF_Raycast;
// @end

/**
 * @struct   CF_Circle
 * @category math
 * @brief    A circle.
 * @related  cf_ray_to_circle cf_circle_to_circle cf_circle_to_aabb cf_circle_to_capsule cf_circle_to_poly cf_circle_to_circle_manifold cf_circle_to_aabb_manifold cf_circle_to_capsule_manifold cf_circle_to_poly_manifold
 */
typedef struct CF_Circle
{
	/* @member Position. */
	CF_V2 p;

	/* @member Radius. */
	float r;
} CF_Circle;
// @end

/**
 * @struct   CF_Aabb
 * @category math
 * @brief    Axis-aligned bounding box. A box that cannot rotate.
 * @remarks  There are many ways to describ an AABB, such as a point + half-extents, or a point and width/height pair. However, of all
 *           these options the min/max choice is the simplest when it comes to the majority of collision detection routine implementations.
 * @related  CF_Aabb cf_make_aabb cf_width cf_height cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right cf_contains_point cf_overlaps cf_make_aabb_verts cf_aabb_verts cf_circle_to_aabb cf_aabb_to_aabb cf_aabb_to_capsule cf_aabb_to_poly cf_ray_to_aabb cf_circle_to_aabb_manifold cf_aabb_to_aabb_manifold cf_aabb_to_capsule_manifold cf_aabb_to_poly_manifold
 */
typedef struct CF_Aabb
{
	/* @member The min corner of the box. */
	CF_V2 min;

	/* @member Top max corner of the box. */
	CF_V2 max;
} CF_Aabb;
// @end

/**
 * @struct   CF_Rect
 * @category math
 * @brief    Box that cannot rotate defined with integers instead of floats.
 * @remarks  Not used for collision detection, but still sometimes useful.
 * @related  CF_Rect cf_render_settings_push_viewport cf_render_settings_push_scissor
 */
typedef struct CF_Rect
{
	/* @member The position + w/h. */
	int x, y, w, h;
} CF_Rect;
// @end

/**
 * @function CF_POLY_MAX_VERTS
 * @category collision
 * @brief    The maximum number of vertices in a `CF_Poly`.
 * @remarks  It's quite common to limit the number of verts on polygons to a low number. Feel free to adjust this number if needed,
 *           but be warned: higher than 8 and shapes generally start to look more like circles/ovals; it becomes pointless beyond a certain point.
 * @related  CF_POLY_MAX_VERTS CF_Poly
 */
#define CF_POLY_MAX_VERTS 8

/**
 * @struct   CF_Poly
 * @category collision
 * @brief    2D polygon.
 * @remarks  Verts are ordered in counter-clockwise order (CCW).
 * @related  CF_POLY_MAX_VERTS CF_Poly cf_circle_to_poly cf_aabb_to_poly cf_capsule_to_poly cf_poly_to_poly cf_ray_to_poly cf_circle_to_poly_manifold cf_aabb_to_poly_manifold cf_capsule_to_poly_manifold cf_poly_to_poly_manifold
 */
typedef struct CF_Poly
{
	/* @member The number of vertices in the polygon, capped at `CF_POLY_MAX_VERTS`. */
	int count;

	/* @member The vertices of the polygon, capped at `CF_POLY_MAX_VERTS`. */
	CF_V2 verts[CF_POLY_MAX_VERTS];

	/* @member The normals of the polygon, capped at `CF_POLY_MAX_VERTS`. Each normal is perpendicular along the poly's surface. */
	CF_V2 norms[CF_POLY_MAX_VERTS];
} CF_Poly;
// @end

/**
 * @struct   CF_Capsule
 * @category collision
 * @brief    A capsule shape.
 * @remarks  It's like a shrink-wrap of 2 circles connected by a rod.
 * @related  CF_Capsule cf_circle_to_capsule cf_aabb_to_capsule cf_capsule_to_capsule cf_capsule_to_poly cf_ray_to_capsule cf_circle_to_capsule_manifold cf_aabb_to_capsule_manifold cf_capsule_to_capsule_manifold cf_capsule_to_poly_manifold
 */
typedef struct CF_Capsule
{
	/* @member The center of one end-cap. */
	CF_V2 a;

	/* @member The center of another end-cap. */
	CF_V2 b;

	/* @member The radius about the rod defined from `a` to `b`. */
	float r;
} CF_Capsule;
// @end

/**
 * @struct   CF_Manifold
 * @category collision
 * @brief    Contains all information necessary to resolve a collision.
 * @remarks  This is the information needed to separate shapes that are colliding.
 * @related  CF_Manifold cf_circle_to_circle_manifold cf_circle_to_aabb_manifold cf_circle_to_capsule_manifold cf_aabb_to_aabb_manifold cf_aabb_to_capsule_manifold cf_capsule_to_capsule_manifold cf_circle_to_poly_manifold cf_aabb_to_poly_manifold cf_capsule_to_poly_manifold cf_poly_to_poly_manifold
 */
typedef struct CF_Manifold
{
	/* @member The number of points in the manifold (0, 1 or 2). */
	int count;

	/* @member The collision depth of each point in the manifold. */
	float depths[2];

	/* @member Up to two points on the contact plane that sufficiently (and minimally) describe how two shapes are touching. */
	CF_V2 contact_points[2];

	/* @member Always points from shape A to shape B. */
	CF_V2 n;
} CF_Manifold;
// @end

/**
 * @function CF_PI
 * @category math
 * @brief    PI the numeric constant.
 */
#define CF_PI 3.14159265f

/**
 * @function CF_TAU
 * @category math
 * @brief    TAU (PI * 2) the numeric constant.
 */
#define CF_TAU (2.0f*3.14159265f)

//--------------------------------------------------------------------------------------------------
// Scalar float ops.

/**
 * @function cf_min
 * @category math
 * @brief    Returns the minimum of two values.
 * @related  cf_min cf_max
 */
#define cf_min(a, b) ((a) < (b) ? (a) : (b))

/**
 * @function cf_max
 * @category math
 * @brief    Returns the maximum of two values.
 * @related  cf_min cf_max
 */
#define cf_max(a, b) ((b) < (a) ? (a) : (b))

/**
 * @function cf_abs
 * @category math
 * @brief    Returns absolute value of a float.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
CF_INLINE float cf_abs(float a) { return CF_FABSF(a); }

/**
 * @function cf_clamp
 * @category math
 * @brief    Returns `a` float clamped between `lo` and `hi`.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
CF_INLINE float cf_clamp(float a, float lo, float hi) { return cf_max(lo, cf_min(a, hi)); }

/**
 * @function cf_clamp01
 * @category math
 * @brief    Returns `a` float clamped between 0.0f and 1.0f.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
CF_INLINE float cf_clamp01(float a) { return cf_max(0.0f, cf_min(a, 1.0f)); }

/**
 * @function cf_sign
 * @category math
 * @brief    Returns the sign (either 1.0f or -1.0f) of `a` float.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
CF_INLINE float cf_sign(float a) { return a < 0 ? -1.0f : 1.0f; }

/**
 * @function cf_intersect
 * @category math
 * @brief    Given the distances of two points `a` and `b` to a plane (`da` and `db` respectively), compute the insterection
 *           value used to lerp from `a` to `b` to find the intersection point.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
CF_INLINE float cf_intersect(float da, float db) { return da / (da - db); }

/**
 * @function cf_safe_invert
 * @category math
 * @brief    Computes `1.0f/a`, but returns 0.0f if `a` is zero.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
CF_INLINE float cf_safe_invert(float a) { return a != 0 ? 1.0f / a : 0; }

/**
 * @function cf_lerp
 * @category math
 * @brief    Returns the linear interpolation from `a` to `b` along `t`, where `t` is _usually_ a value from 0.0f to 1.0f.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
CF_INLINE float cf_lerp(float a, float b, float t) { return a + (b - a) * t; }

/**
 * @function cf_remap01
 * @category math
 * @brief    Returns the value `t` remaped from [0, 1] to [lo, hi].
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
CF_INLINE float cf_remap01(float t, float lo, float hi) { return lo + t * (hi - lo); }

/**
 * @function cf_remap
 * @category math
 * @brief    Returns the value `t` remaped from [old_lo, old_hi] to [lo, hi].
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
CF_INLINE float cf_remap(float t, float old_lo, float old_hi, float lo, float hi) { return lo + ((old_hi - old_lo) != 0 ? (t - old_lo) / (old_hi - old_lo) : 0) * (hi - lo); }

/**
 * @function cf_mod
 * @category math
 * @brief    Returns floating point `x % m`. Can return negative numbers.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
CF_INLINE float cf_mod(float x, float m) { return x - (int)(x / m) * m; }

/**
 * @function cf_fract
 * @category math
 * @brief    Returns the fractional portion of a float.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
CF_INLINE float cf_fract(float x) { return x - CF_FLOORF(x); }

/**
 * @function cf_sign_int
 * @category math
 * @brief    Returns the sign (either 1 or -1) of an int.
 * @related  cf_sign_int cf_abs_int cf_clamp_int cf_clamp01_int cf_is_even cf_is_odd
 */
CF_INLINE int cf_sign_int(int a) { return a < 0 ? -1 : 1; }

/**
 * @function cf_abs_int
 * @category math
 * @brief    Returns the absolute value of an int.
 * @related  cf_sign_int cf_abs_int cf_clamp_int cf_clamp01_int cf_is_even cf_is_odd
 */
CF_INLINE int cf_abs_int(int a) { int mask = a >> ((sizeof(int) * 8) - 1); return (a + mask) ^ mask; }

/**
 * @function cf_clamp_int
 * @category math
 * @brief    Returns an int clamped between `lo` and `hi`.
 * @related  cf_sign_int cf_abs_int cf_clamp_int cf_clamp01_int cf_is_even cf_is_odd
 */
CF_INLINE int cf_clamp_int(int a, int lo, int hi) { return cf_max(lo, cf_min(a, hi)); }

/**
 * @function cf_clamp01_int
 * @category math
 * @brief    Returns an int clamped between 0 and 1.
 * @related  cf_sign_int cf_abs_int cf_clamp_int cf_clamp01_int cf_is_even cf_is_odd
 */
CF_INLINE int cf_clamp01_int(int a) { return cf_max(0, cf_min(a, 1)); }

/**
 * @function cf_is_even
 * @category math
 * @brief    Returns true if an int is even.
 * @related  cf_sign_int cf_abs_int cf_clamp_int cf_clamp01_int cf_is_even cf_is_odd
 */
CF_INLINE bool cf_is_even(int x) { return (x % 2) == 0; }

/**
 * @function cf_is_odd
 * @category math
 * @brief    Returns true if an int is odd.
 * @related  cf_sign_int cf_abs_int cf_clamp_int cf_clamp01_int cf_is_even cf_is_odd
 */
CF_INLINE bool cf_is_odd(int x) { return !cf_is_even(x); }

//--------------------------------------------------------------------------------------------------
// Bit manipulation.

/**
 * @function cf_is_power_of_two
 * @category math
 * @brief    Returns true if an int is a power of two.
 * @related  cf_is_power_of_two cf_is_power_of_two_uint cf_fit_power_of_two
 */
CF_INLINE bool cf_is_power_of_two(int a) { return a != 0 && (a & (a - 1)) == 0; }

/**
 * @function cf_is_power_of_two_uint
 * @category math
 * @brief    Returns true if an unsigned int is a power of two.
 * @related  cf_is_power_of_two cf_is_power_of_two_uint cf_fit_power_of_two
 */
CF_INLINE bool cf_is_power_of_two_uint(uint64_t a) { return a != 0 && (a & (a - 1)) == 0; }

/**
 * @function cf_fit_power_of_two
 * @category math
 * @brief    Returns an integer clamped upwards to the nearest power of two.
 * @related  cf_is_power_of_two cf_is_power_of_two_uint cf_fit_power_of_two
 */
CF_INLINE int cf_fit_power_of_two(int a) { a--; a |= a >> 1; a |= a >> 2; a |= a >> 4; a |= a >> 8; a |= a >> 16; a++; return a; }

//--------------------------------------------------------------------------------------------------
// Easing functions.
// Adapted from Noel Berry: https://github.com/NoelFB/blah/blob/master/include/blah_ease.h

/**
 * @function cf_smoothstep
 * @category math
 * @brief    Returns the smoothstep of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 */
CF_INLINE float cf_smoothstep(float x) { return x * x * (3.0f - 2.0f * x); }

/**
 * @function cf_quad_in
 * @category math
 * @brief    Returns the quadratic-in ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_quad_in cf_quad_out cf_quad_in_out
 */
CF_INLINE float cf_quad_in(float x) { return x * x; }

/**
 * @function cf_quad_out
 * @category math
 * @brief    Returns the quadratic-out ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_quad_in cf_quad_out cf_quad_in_out
 */
CF_INLINE float cf_quad_out(float x) { return -(x * (x - 2.0f)); }

/**
 * @function cf_quad_in_out
 * @category math
 * @brief    Returns the quadratic ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_quad_in cf_quad_out cf_quad_in_out
 */
CF_INLINE float cf_quad_in_out(float x) { if (x < 0.5f) return 2.0f * x * x; else return (-2.0f * x * x) + (4.0f * x) - 1.0f; }

/**
 * @function cf_cube_in
 * @category math
 * @brief    Returns the cubic-in ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_cube_in cf_cube_out cf_cube_in_out
 */
CF_INLINE float cf_cube_in(float x) { return x * x * x; }

/**
 * @function cf_cube_out
 * @category math
 * @brief    Returns the cubic-out ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_cube_in cf_cube_out cf_cube_in_out
 */
CF_INLINE float cf_cube_out(float x) { float f = (x - 1); return f * f * f + 1.0f; }

/**
 * @function cf_cube_in_out
 * @category math
 * @brief    Returns the cubic ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_cube_in cf_cube_out cf_cube_in_out
 */
CF_INLINE float cf_cube_in_out(float x) { if (x < 0.5f) return 4.0f * x * x * x; else { float f = ((2.0f * x) - 2.0f); return 0.5f * x * x * x + 1.0f; } }

/**
 * @function cf_quart_in
 * @category math
 * @brief    Returns the quartic-in ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_quart_in cf_quart_out cf_quart_in_out
 */
CF_INLINE float cf_quart_in(float x) { return x * x * x * x; }

/**
 * @function cf_quart_out
 * @category math
 * @brief    Returns the quartic-out ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_quart_in cf_quart_out cf_quart_in_out
 */
CF_INLINE float cf_quart_out(float x) { float f = (x - 1.0f); return f * f * f * (1.0f - x) + 1.0f; }

/**
 * @function cf_quart_in_out
 * @category math
 * @brief    Returns the quartic ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_quart_in cf_quart_out cf_quart_in_out
 */
CF_INLINE float cf_quart_in_out(float x) { if (x < 0.5f) return 8.0f * x * x * x * x; else { float f = (x - 1); return -8.0f * f * f * f * f + 1.0f; } }

/**
 * @function cf_quint_in
 * @category math
 * @brief    Returns the quintic-in ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_quint_in cf_quint_out cf_quint_in_out
 */
CF_INLINE float cf_quint_in(float x) { return x * x * x * x * x; }

/**
 * @function cf_quint_out
 * @category math
 * @brief    Returns the quintic-out ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_quint_in cf_quint_out cf_quint_in_out
 */
CF_INLINE float cf_quint_out(float x) { float f = (x - 1); return f * f * f * f * f + 1.0f; }

/**
 * @function cf_quint_in_out
 * @category math
 * @brief    Returns the quintic ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_quint_in cf_quint_out cf_quint_in_out
 */
CF_INLINE float cf_quint_in_out(float x) { if (x < 0.5f) return 16.0f * x * x * x * x * x; else { float f = ((2.0f * x) - 2.0f); return  0.5f * f * f * f * f * f + 1.0f; } }

/**
 * @function cf_sin_in
 * @category math
 * @brief    Returns the sin-in ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_sin_in cf_sin_out cf_sin_in_out
 */
CF_INLINE float cf_sin_in(float x) { return CF_SINF((x - 1.0f) * CF_PI * 0.5f) + 1.0f; }

/**
 * @function cf_sin_out
 * @category math
 * @brief    Returns the sin-out ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_sin_in cf_sin_out cf_sin_in_out
 */
CF_INLINE float cf_sin_out(float x) { return CF_SINF(x * (CF_PI * 0.5f)); }

/**
 * @function cf_sin_in_out
 * @category math
 * @brief    Returns the sin ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_sin_in cf_sin_out cf_sin_in_out
 */
CF_INLINE float cf_sin_in_out(float x) { return 0.5f * (1.0f - CF_COSF(x * CF_PI)); }

/**
 * @function cf_circle_in
 * @category math
 * @brief    Returns the circle-in ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_circle_in cf_circle_out cf_circle_in_out
 */
CF_INLINE float cf_circle_in(float x) { return 1.0f - CF_SQRTF(1.0f - (x * x)); }

/**
 * @function cf_circle_out
 * @category math
 * @brief    Returns the circle-out ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_circle_in cf_circle_out cf_circle_in_out
 */
CF_INLINE float cf_circle_out(float x) { return CF_SQRTF((2.0f - x) * x); }

/**
 * @function cf_circle_in_out
 * @category math
 * @brief    Returns the circle ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_circle_in cf_circle_out cf_circle_in_out
 */
CF_INLINE float cf_circle_in_out(float x) { if (x < 0.5f) return 0.5f * (1.0f - CF_SQRTF(1.0f - 4.0f * (x * x))); else return 0.5f * (CF_SQRTF(-((2.0f * x) - 3.0f) * ((2.0f * x) - 1.0f)) + 1.0f); }

/**
 * @function cf_back_in
 * @category math
 * @brief    Returns the back-in ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_back_in cf_back_out cf_back_in_out
 */
CF_INLINE float cf_back_in(float x) { return x * x * x - x * CF_SINF(x * CF_PI); }

/**
 * @function cf_back_out
 * @category math
 * @brief    Returns the back-out ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_back_in cf_back_out cf_back_in_out
 */
CF_INLINE float cf_back_out(float x) { float f = (1.0f - x); return 1.0f - (x * x * x - x * CF_SINF(f * CF_PI)); }

/**
 * @function cf_back_in_out
 * @category math
 * @brief    Returns the back ease of a float from 0.0f to 1.0f.
 * @remarks  Here is a great link to [visualize each easing function](https://easings.net/).
 * @related  cf_back_in cf_back_out cf_back_in_out
 */
CF_INLINE float cf_back_in_out(float x) { if (x < 0.5f) { float f = 2.0f * x; return 0.5f * (f * f * f - f * CF_SINF(f * CF_PI)); } else { float f = (1.0f - (2.0f * x - 1.0f)); return 0.5f * (1.0f - (f * f * f - f * CF_SINF(f * CF_PI))) + 0.5f; } }

//--------------------------------------------------------------------------------------------------
// 2D vector ops.

/**
 * @function cf_add_v2
 * @category math
 * @brief    Returns two vectors added together.
 * @related  CF_V2 cf_add_v2 cf_sub_v2 cf_dot cf_mul_v2_f cf_div_v2_f
 */
CF_INLINE CF_V2 cf_add_v2(CF_V2 a, CF_V2 b) { return cf_v2(a.x + b.x, a.y + b.y); }

/**
 * @function cf_sub_v2
 * @category math
 * @brief    Returns a vector subtracted from another.
 * @related  CF_V2 cf_add_v2 cf_sub_v2 cf_dot cf_mul_v2_f cf_div_v2_f
 */
CF_INLINE CF_V2 cf_sub_v2(CF_V2 a, CF_V2 b) { return cf_v2(a.x - b.x, a.y - b.y); }

/**
 * @function cf_dot
 * @category math
 * @brief    Returns the dot product of two vectors.
 * @related  CF_V2 cf_add_v2 cf_sub_v2 cf_dot cf_mul_v2_f cf_div_v2_f
 */
CF_INLINE float cf_dot(CF_V2 a, CF_V2 b) { return a.x * b.x + a.y * b.y; }

/**
 * @function cf_mul_v2_f
 * @category math
 * @brief    Multiplies a vector with a float.
 * @related  CF_V2 cf_mul_v2_f cf_mul_v2 cf_div_v2_f
 */
CF_INLINE CF_V2 cf_mul_v2_f(CF_V2 a, float b) { return cf_v2(a.x * b, a.y * b); }

/**
 * @function cf_mul_v2
 * @category math
 * @brief    Multiplies two vectors together component-wise.
 * @remarks  The vector returned is `{ a.x * b.x, a.y * b.y }`.
 * @related  CF_V2 cf_mul_v2_f cf_mul_v2 cf_div_v2_f
 */
CF_INLINE CF_V2 cf_mul_v2(CF_V2 a, CF_V2 b) { return cf_v2(a.x * b.x, a.y * b.y); }

/**
 * @function cf_div_v2_f
 * @category math
 * @brief    Divides a vector by a float.
 * @related  CF_V2 cf_mul_v2_f cf_mul_v2 cf_div_v2_f
 */
CF_INLINE CF_V2 cf_div_v2_f(CF_V2 a, float b) { return cf_v2(a.x / b, a.y / b); }

/**
 * @function cf_skew
 * @category math
 * @brief    Returns the skew of a vector. This acts like a 90 degree rotation counter-clockwise.
 * @related  CF_V2 cf_skew cf_cw90 cf_det2 cf_cross cf_perp
 */
CF_INLINE CF_V2 cf_skew(CF_V2 a) { return cf_v2(-a.y, a.x); }

/**
 * @function cf_perp
 * @category math
 * @brief    Returns the skew of a vector. This acts like a 90 degree rotation counter-clockwise. In this case
 *           perp stands for perpendicular.
 * @related  CF_V2 cf_skew cf_cw90 cf_det2 cf_cross cf_perp
 */
CF_INLINE CF_V2 cf_perp(CF_V2 a) { return cf_v2(-a.y, a.x); }

/**
 * @function cf_cw90
 * @category math
 * @brief    Returns the anti-skew of a vector. This acts like a 90 degree rotation clockwise.
 * @related  CF_V2 cf_skew cf_cw90 cf_det2 cf_cross
 */
CF_INLINE CF_V2 cf_cw90(CF_V2 a) { return cf_v2(a.y, -a.x); }

/**
 * @function cf_det2
 * @category math
 * @brief    Returns the 2x2 determinant of a matrix constructed with `a` and `b` as its columns.
 * @remarks  Also known as the 2D cross product.
 * @related  CF_V2 cf_skew cf_cw90 cf_det2 cf_cross cf_perp
 */
CF_INLINE float cf_det2(CF_V2 a, CF_V2 b) { return a.x * b.y - a.y * b.x; }

/**
 * @function cf_cross
 * @category math
 * @brief    Returns the 2D cross product of two vectors.
 * @related  CF_V2 cf_skew cf_cw90 cf_det2 cf_cross cf_perp
 */
CF_INLINE float cf_cross(CF_V2 a, CF_V2 b) { return cf_det2(a, b); }

/**
 * @function cf_cross_v2_f
 * @category math
 * @brief    Returns the 2D cross product of a vector against a scalar.
 * @related  CF_V2 cf_skew cf_cw90 cf_det2 cf_cross cf_cross_v2_f cf_cross_f_v2
 */
CF_INLINE CF_V2 cf_cross_v2_f(CF_V2 a, float b) { return cf_v2(b * a.y, -b * a.x); }

/**
 * @function cf_cross_f_v2
 * @category math
 * @brief    Returns the 2D cross product of a scalar against a vector.
 * @related  CF_V2 cf_skew cf_cw90 cf_det2 cf_cross cf_cross_v2_f cf_cross_f_v2
 */
CF_INLINE CF_V2 cf_cross_f_v2(float a, CF_V2 b) { return cf_v2(-a * b.y, a * b.x); }

/**
 * @function cf_min_v2
 * @category math
 * @brief    Returns the component-wise minimum of two vectors.
 * @remarks  The vector returned has the value `{ cf_min(a.x, b.x), cf_min(a.y, b.y) }`. See `cf_min`.
 * @related  CF_V2 cf_min_v2 cf_max_v2 cf_clamp_v2 cf_clamp01_v2 cf_abs_v2 cf_hmin cf_hmax
 */
CF_INLINE CF_V2 cf_min_v2(CF_V2 a, CF_V2 b) { return cf_v2(cf_min(a.x, b.x), cf_min(a.y, b.y)); }

/**
 * @function cf_max_v2
 * @category math
 * @brief    Returns the component-wise maximum of two vectors.
 * @remarks  The vector returned has the value `{ cf_max(a.x, b.x), cf_max(a.y, b.y) }`. See `cf_max`.
 * @related  CF_V2 cf_min_v2 cf_max_v2 cf_clamp_v2 cf_clamp01_v2 cf_abs_v2 cf_hmin cf_hmax
 */
CF_INLINE CF_V2 cf_max_v2(CF_V2 a, CF_V2 b) { return cf_v2(cf_max(a.x, b.x), cf_max(a.y, b.y)); }

/**
 * @function cf_clamp_v2
 * @category math
 * @brief    Returns the component-wise clamp of two vectors from `lo` to `hi`.
 * @related  CF_V2 cf_min_v2 cf_max_v2 cf_clamp_v2 cf_clamp01_v2 cf_abs_v2 cf_hmin cf_hmax
 */
CF_INLINE CF_V2 cf_clamp_v2(CF_V2 a, CF_V2 lo, CF_V2 hi) { return cf_max_v2(lo, cf_min_v2(a, hi)); }

/**
 * @function cf_clamp01_v2
 * @category math
 * @brief    Returns the component-wise clamp of two vectors from 0.0f to 1.0f.
 * @related  CF_V2 cf_min_v2 cf_max_v2 cf_clamp_v2 cf_clamp01_v2 cf_abs_v2 cf_hmin cf_hmax
 */
CF_INLINE CF_V2 cf_clamp01_v2(CF_V2 a) { return cf_max_v2(cf_v2(0, 0), cf_min_v2(a, cf_v2(1, 1))); }

/**
 * @function cf_abs_v2
 * @category math
 * @brief    Returns the component-wise absolute value of two vectors.
 * @related  CF_V2 cf_min_v2 cf_max_v2 cf_clamp_v2 cf_clamp01_v2 cf_abs_v2 cf_hmin cf_hmax
 */
CF_INLINE CF_V2 cf_abs_v2(CF_V2 a) { return cf_v2(CF_FABSF(a.x), CF_FABSF(a.y)); }

/**
 * @function cf_hmin
 * @category math
 * @brief    Returns minimum of all components of a vector.
 * @related  CF_V2 cf_min_v2 cf_max_v2 cf_clamp_v2 cf_clamp01_v2 cf_abs_v2 cf_hmin cf_hmax
 */
CF_INLINE float cf_hmin(CF_V2 a) { return cf_min(a.x, a.y); }

/**
 * @function cf_hmax
 * @category math
 * @brief    Returns maximum of all components of a vector.
 * @related  CF_V2 cf_min_v2 cf_max_v2 cf_clamp_v2 cf_clamp01_v2 cf_abs_v2 cf_hmin cf_hmax
 */
CF_INLINE float cf_hmax(CF_V2 a) { return cf_max(a.x, a.y); }

/**
 * @function cf_len
 * @category math
 * @brief    Returns length of a vector.
 * @related  CF_V2 cf_len cf_distance cf_norm cf_safe_norm
 */
CF_INLINE float cf_len(CF_V2 a) { return CF_SQRTF(cf_dot(a, a)); }

/**
 * @function cf_len_sq
 * @category math
 * @brief    Returns squared length of a vector.
 * @related  CF_V2 cf_len cf_distance cf_norm cf_safe_norm
 */
CF_INLINE float cf_len_sq(CF_V2 a) { return cf_dot(a, a); }

/**
 * @function cf_distance
 * @category math
 * @brief    Returns distance between two points.
 * @related  CF_V2 cf_len cf_distance cf_norm cf_safe_norm
 */
CF_INLINE float cf_distance(CF_V2 a, CF_V2 b) { CF_V2 d = cf_sub_v2(b, a); return CF_SQRTF(cf_dot(d, d)); }

/**
 * @function cf_norm
 * @category math
 * @brief    Returns a normalized vector.
 * @remarks  Normalized vectors have unit-length without changing the vector's direction. Fails if the vector has a length of zero.
 * @related  CF_V2 cf_len cf_distance cf_norm cf_safe_norm
 */
CF_INLINE CF_V2 cf_norm(CF_V2 a) { return cf_div_v2_f(a, cf_len(a)); }

/**
 * @function cf_safe_norm
 * @category math
 * @brief    Returns a normalized vector.
 * @remarks  Sets the vector to `{ 0, 0 }` if the length of the vector is zero. Unlike `cf_norm`, this function cannot fail for
 *           the case of a zero vector.
 * @related  CF_V2 cf_len cf_distance cf_norm cf_safe_norm
 */
CF_INLINE CF_V2 cf_safe_norm(CF_V2 a) { float sq = cf_dot(a, a); return sq ? cf_div_v2_f(a, CF_SQRTF(sq)) : cf_v2(0, 0); }

/**
 * @function cf_safe_norm_f
 * @category math
 * @brief    Returns the sign of a float, or zero if the float is zero.
 * @related  cf_safe_norm_f cf_safe_norm_int
 */
CF_INLINE float cf_safe_norm_f(float a) { return a == 0 ? 0 : cf_sign(a); }

/**
 * @function cf_safe_norm_int
 * @category math
 * @brief    Returns the sign of an int, or zero if the int is zero.
 * @related  cf_safe_norm_f cf_safe_norm_int
 */
CF_INLINE int cf_safe_norm_int(int a) { return a == 0 ? 0 : cf_sign_int(a); }

/**
 * @function cf_neg_v2
 * @category math
 * @brief    Returns a negated vector.
 * @related  CF_V2 cf_neg_v2 cf_sign_v2
 */
CF_INLINE CF_V2 cf_neg_v2(CF_V2 a) { return cf_v2(-a.x, -a.y); }

/**
 * @function cf_reflect_v2
 * @category math
 * @brief    Returns a vector of equal length to `a` but with its direction reflected
 * @param    a        The vector being reflected
 * @param    n        The normal of the plane that is being reflected off of
 * @related  CF_V2 cf_neg_v2 cf_norm cf_safe_norm
 */
CF_INLINE CF_V2 cf_reflect_v2(CF_V2 a, CF_V2 n) { return cf_sub_v2(a, cf_mul_v2_f(n, (2.f * cf_dot(a, n)))); }

/**
 * @function cf_lerp_v2
 * @category math
 * @brief    Returns a vector linearly interpolated from `a` to `b` along `t`, a value _usually_ from 0.0f to 1.0f.
 * @related  CF_V2 cf_lerp_v2 cf_bezier
 */
CF_INLINE CF_V2 cf_lerp_v2(CF_V2 a, CF_V2 b, float t) { return cf_add_v2(a, cf_mul_v2_f(cf_sub_v2(b, a), t)); }

/**
 * @function cf_bezier
 * @category math
 * @brief    Returns a point along a quadratic bezier curve according to time `t`.
 * @param    a        The start point.
 * @param    c0       A control point.
 * @param    b        The end point.
 * @param    t        A position along the curve.
 * @related  CF_V2 cf_lerp_v2 cf_bezier cf_bezier2
 */
CF_INLINE CF_V2 cf_bezier(CF_V2 a, CF_V2 c0, CF_V2 b, float t) { return cf_lerp_v2(cf_lerp_v2(a, c0, t), cf_lerp_v2(c0, b, t), t); }

/**
 * @function cf_bezier2
 * @category math
 * @brief    Returns a point along a cubic bezier curve according to time `t`.
 * @param    a        The start point.
 * @param    c0       A control point.
 * @param    c1       A control point.
 * @param    b        The end point.
 * @param    t        A position along the curve.
 * @related  CF_V2 cf_lerp_v2 cf_bezier cf_bezier2
 */
CF_INLINE CF_V2 cf_bezier2(CF_V2 a, CF_V2 c0, CF_V2 c1, CF_V2 b, float t) { return cf_bezier(cf_lerp_v2(a, c0, t), cf_lerp_v2(c0, c1, t), cf_lerp_v2(c1, b, t), t); }

/**
 * @function cf_lesser_v2
 * @category math
 * @brief    Returns true if `a.x < b.y` and `a.y < b.y`.
 * @related  CF_V2 cf_round cf_lesser_v2 cf_greater_v2 cf_lesser_equal_v2 cf_greater_equal_v2 cf_parallel
 */
CF_INLINE int cf_lesser_v2(CF_V2 a, CF_V2 b) { return a.x < b.x && a.y < b.y; }

/**
 * @function cf_greater_v2
 * @category math
 * @brief    Returns true if `a.x > b.y` and `a.y > b.y`.
 * @related  CF_V2 cf_round cf_lesser_v2 cf_greater_v2 cf_lesser_equal_v2 cf_greater_equal_v2 cf_parallel
 */
CF_INLINE int cf_greater_v2(CF_V2 a, CF_V2 b) { return a.x > b.x && a.y > b.y; }

/**
 * @function cf_lesser_equal_v2
 * @category math
 * @brief    Returns true if `a.x <= b.y` and `a.y <= b.y`.
 * @related  CF_V2 cf_round cf_lesser_v2 cf_greater_v2 cf_lesser_equal_v2 cf_greater_equal_v2 cf_parallel
 */
CF_INLINE int cf_lesser_equal_v2(CF_V2 a, CF_V2 b) { return a.x <= b.x && a.y <= b.y; }

/**
 * @function cf_greater_equal_v2
 * @category math
 * @brief    Returns true if `a.x >= b.y` and `a.y >= b.y`.
 * @related  CF_V2 cf_roundcf_lesser_v2 cf_greater_v2 cf_lesser_equal_v2 cf_greater_equal_v2 cf_parallel
 */
CF_INLINE int cf_greater_equal_v2(CF_V2 a, CF_V2 b) { return a.x >= b.x && a.y >= b.y; }

/**
 * @function cf_floor
 * @category math
 * @brief    Returns the component-wise floor of a vector.
 * @remarks  Floor means the decimal-point part is zero'd out.
 * @related  CF_V2 cf_round cf_lesser_v2 cf_greater_v2 cf_lesser_equal_v2 cf_greater_equal_v2 cf_parallel cf_floor cf_ceil
 */
CF_INLINE CF_V2 cf_floor(CF_V2 a) { return cf_v2(CF_FLOORF(a.x), CF_FLOORF(a.y)); }

/**
 * @function cf_ceil
 * @category math
 * @brief    Returns the component-wise ceil of a vector.
 * @remarks  Ceil means the number is clamped up to the next whole-number.
 * @related  CF_V2 cf_round cf_lesser_v2 cf_greater_v2 cf_lesser_equal_v2 cf_greater_equal_v2 cf_parallel cf_floor cf_ceil
 */
CF_INLINE CF_V2 cf_ceil(CF_V2 a) { return cf_v2(CF_CEILF(a.x), CF_CEILF(a.y)); }

/**
 * @function cf_round
 * @category math
 * @brief    Returns the component-wise round of a vector.
 * @remarks  Rounding means clamping the float to the nearest whole integer value.
 * @related  CF_V2 cf_lesser_v2 cf_greater_v2 cf_lesser_equal_v2 cf_greater_equal_v2 cf_parallel
 */
CF_INLINE CF_V2 cf_round(CF_V2 a) { return cf_v2(CF_ROUNDF(a.x), CF_ROUNDF(a.y)); }

/**
 * @function cf_safe_invert_v2
 * @category math
 * @brief    Returns the component-wise safe inversion of a vector.
 * @related  CF_V2 cf_safe_invert
 */
CF_INLINE CF_V2 cf_safe_invert_v2(CF_V2 a) { return cf_v2(cf_safe_invert(a.x), cf_safe_invert(a.y)); }

/**
 * @function cf_sign_v2
 * @category math
 * @brief    Returns the component-wise sign of a vector.
 * @related  CF_V2 cf_sign
 */
CF_INLINE CF_V2 cf_sign_v2(CF_V2 a) { return cf_v2(cf_sign(a.x), cf_sign(a.y)); }

//--------------------------------------------------------------------------------------------------
// CF_SinCos rotation ops.

/**
 * @function cf_sincos_f
 * @category math
 * @brief    Returns an initialized `CF_SinCos` from `radians`.
 * @related  CF_SinCos cf_sincos_f cf_x_axis cf_y_axis cf_mul_sc_v2 cf_mulT_sc_v2 cf_mul_sc cf_mulT_sc
 */
CF_INLINE CF_SinCos cf_sincos_f(float radians) { CF_SinCos r; r.s = CF_SINF(radians); r.c = CF_COSF(radians); return r; }
CF_INLINE CF_SinCos cf_sincos() { CF_SinCos r; r.c = 1.0f; r.s = 0; return r; }

/**
 * @function cf_x_axis
 * @category math
 * @brief    Returns the x-axis of the 2x2 rotation matrix represented by `CF_SinCos`.
 * @related  CF_SinCos cf_sincos_f cf_x_axis cf_y_axis cf_mul_sc_v2 cf_mulT_sc_v2 cf_mul_sc cf_mulT_sc
 */
CF_INLINE CF_V2 cf_x_axis(CF_SinCos r) { return cf_v2(r.c, r.s); }

/**
 * @function cf_y_axis
 * @category math
 * @brief    Returns the y-axis of the 2x2 rotation matrix represented by `CF_SinCos`.
 * @related  CF_SinCos cf_sincos_f cf_x_axis cf_y_axis cf_mul_sc_v2 cf_mulT_sc_v2 cf_mul_sc cf_mulT_sc
 */
CF_INLINE CF_V2 cf_y_axis(CF_SinCos r) { return cf_v2(-r.s, r.c); }

/**
 * @function cf_mul_sc_v2
 * @category math
 * @brief    Returns a vector rotated by `a`.
 * @related  CF_SinCos cf_sincos_f cf_x_axis cf_y_axis cf_mul_sc_v2 cf_mulT_sc_v2 cf_mul_sc cf_mulT_sc
 */
CF_INLINE CF_V2 cf_mul_sc_v2(CF_SinCos a, CF_V2 b) { return cf_v2(a.c * b.x - a.s * b.y, a.s * b.x + a.c * b.y); }

/**
 * @function cf_mulT_sc_v2
 * @category math
 * @brief    Returns a vector inverse-rotated by `a`.
 * @related  CF_SinCos cf_sincos_f cf_x_axis cf_y_axis cf_mul_sc_v2 cf_mulT_sc_v2 cf_mul_sc cf_mulT_sc
 */
CF_INLINE CF_V2 cf_mulT_sc_v2(CF_SinCos a, CF_V2 b) { return cf_v2(a.c * b.x + a.s * b.y, -a.s * b.x + a.c * b.y); }

/**
 * @function cf_mul_sc
 * @category math
 * @brief    Returns the composition of `a` multiplied by `b`.
 * @related  CF_SinCos cf_sincos_f cf_x_axis cf_y_axis cf_mul_sc_v2 cf_mulT_sc_v2 cf_mul_sc cf_mulT_sc
 */
CF_INLINE CF_SinCos cf_mul_sc(CF_SinCos a, CF_SinCos b) { CF_SinCos c; c.c = a.c * b.c - a.s * b.s; c.s = a.s * b.c + a.c * b.s; return c; }

/**
 * @function cf_mulT_sc
 * @category math
 * @brief    Returns the composition of `a` multiplied by inverse of `b`.
 * @related  CF_SinCos cf_sincos_f cf_x_axis cf_y_axis cf_mul_sc_v2 cf_mulT_sc_v2 cf_mul_sc cf_mulT_sc
 */
CF_INLINE CF_SinCos cf_mulT_sc(CF_SinCos a, CF_SinCos b) { CF_SinCos c; c.c = a.c * b.c + a.s * b.s; c.s = a.c * b.s - a.s * b.c; return c; }

/**
 * @function cf_atan2_360
 * @category math
 * @brief    Returns a remap'd result from atan2f to the continuous range of 0, 2*PI.
 * @related  cf_atan2_360 cf_atan2_360_sc cf_atan2_360_v2
 */
CF_INLINE float cf_atan2_360(float y, float x) { return CF_ATAN2F(-y, -x) + CF_PI; }

/**
 * @function cf_atan2_360_sc
 * @category math
 * @brief    Returns a remap'd result from atan2f to the continuous range of 0, 2*PI.
 * @related  cf_atan2_360 cf_atan2_360_sc cf_atan2_360_v2
 */
CF_INLINE float cf_atan2_360_sc(CF_SinCos r) { return cf_atan2_360(r.s, r.c); }

/**
 * @function cf_atan2_360_v2
 * @category math
 * @brief    Returns a remap'd result from atan2f to the continuous range of 0, 2*PI.
 * @related  cf_atan2_360 cf_atan2_360_sc cf_atan2_360_v2
 */
CF_INLINE float cf_atan2_360_v2(CF_V2 v) { return CF_ATAN2F(-v.y, -v.x) + CF_PI; }

/**
 * @function cf_shortest_arc
 * @category math
 * @brief    Returns the shortest angle to rotate the vector `a` to the vector `b`.
 * @related  cf_shortest_arc cf_angle_diff cf_from_angle
 */
CF_INLINE float cf_shortest_arc(CF_V2 a, CF_V2 b)
{
	float c = cf_dot(a, b);
	float s = cf_det2(a, b);
	float theta = CF_ACOSF(c);
	if (s > 0) {
		return theta;
	} else {
		return -theta;
	}
}

/**
 * @function cf_angle_diff
 * @category math
 * @brief    Returns the difference of two angles (b - a) in the range of -`CF_PI` to `CF_PI`.
 * @related  cf_shortest_arc cf_angle_diff cf_from_angle
 */
CF_INLINE float cf_angle_diff(float radians_a, float radians_b)
{
	float result = cf_mod(radians_b - radians_a, 2*CF_PI);
	return cf_mod(result + 3*CF_PI, 2*CF_PI) - CF_PI;
}

/**
 * @function cf_from_angle
 * @category math
 * @brief    returns a vector according to the sin/cos of `radians`.
 * @related  cf_shortest_arc cf_angle_diff cf_from_angle
 */
CF_INLINE CF_V2 cf_from_angle(float radians) { return cf_v2(CF_COSF(radians), CF_SINF(radians)); }

//--------------------------------------------------------------------------------------------------
// m2 ops.
// 2D graphics matrix for only scale + rotate.

/**
 * @function cf_mul_m2_f
 * @category math
 * @brief    Multiplies a `CF_M2x2` by a float.
 * @related  CF_M2x2 cf_mul_m2_f cf_mul_m2_v2 cf_mul_m2
 */
CF_INLINE CF_M2x2 cf_mul_m2_f(CF_M2x2 a, float b) { CF_M2x2 c; c.x = cf_mul_v2_f(a.x, b); c.y = cf_mul_v2_f(a.y, b); return c; }

/**
 * @function cf_mul_m2_v2
 * @category math
 * @brief    Multiplies a vector by a `CF_M2x2`.
 * @related  CF_M2x2 CF_V2 cf_mul_m2_f cf_mul_m2_v2 cf_mul_m2
 */
CF_INLINE CF_V2 cf_mul_m2_v2(CF_M2x2 a, CF_V2 b) { CF_V2 c; c.x = a.x.x * b.x + a.y.x * b.y; c.y = a.x.y * b.x + a.y.y * b.y; return c; }

/**
 * @function cf_mul_m2
 * @category math
 * @brief    Returns the composition of `a` times `b`.
 * @related  CF_M2x2 cf_mul_m2_f cf_mul_m2_v2 cf_mul_m2
 */
CF_INLINE CF_M2x2 cf_mul_m2(CF_M2x2 a, CF_M2x2 b) { CF_M2x2 c; c.x = cf_mul_m2_v2(a, b.x); c.y = cf_mul_m2_v2(a, b.y); return c; }

//--------------------------------------------------------------------------------------------------
// m3x2 ops.
// General purpose 2D graphics matrix; scale + rotate + translate.

/**
 * @function cf_mul_m32_v2
 * @category math
 * @brief    Returns a vector multiplied by a `CF_M3x2`.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_V2 cf_mul_m32_v2(CF_M3x2 a, CF_V2 b) { return cf_add_v2(cf_mul_m2_v2(a.m, b), a.p); }

/**
 * @function cf_mul_m32
 * @category math
 * @brief    Returns the composition of two `CF_M3x2`s multiplied together.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_mul_m32(CF_M3x2 a, CF_M3x2 b) { CF_M3x2 c; c.m = cf_mul_m2(a.m, b.m); c.p = cf_add_v2(cf_mul_m2_v2(a.m, b.p), a.p); return c; }

/**
 * @function cf_make_identity
 * @category math
 * @brief    Returns an identity `CF_M3x2`.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_make_identity() { CF_M3x2 m; m.m.x = cf_v2(1, 0); m.m.y = cf_v2(0, 1); m.p = cf_v2(0, 0); return m; }

/**
 * @function cf_make_translation_f
 * @category math
 * @brief    Returns a `CF_M3x2` that represents a translation.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_make_translation_f(float x, float y) { CF_M3x2 m; m.m.x = cf_v2(1, 0); m.m.y = cf_v2(0, 1); m.p = cf_v2(x, y); return m; }

/**
 * @function cf_make_translation
 * @category math
 * @brief    Returns a `CF_M3x2` that represents a translation.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_make_translation(CF_V2 p) { return cf_make_translation_f(p.x, p.y); }

/**
 * @function cf_make_scale
 * @category math
 * @brief    Returns a `CF_M3x2` that represents a scale.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_make_scale(CF_V2 s) { CF_M3x2 m; m.m.x = cf_v2(s.x, 0); m.m.y = cf_v2(0, s.y); m.p = cf_v2(0, 0); return m; }

/**
 * @function cf_make_scale_f
 * @category math
 * @brief    Returns a `CF_M3x2` that represents a scale.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_make_scale_f(float s) { return cf_make_scale(cf_v2(s, s)); }

/**
 * @function cf_make_scale_translation
 * @category math
 * @brief    Returns a `CF_M3x2` that represents a scale + translation.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_make_scale_translation(CF_V2 s, CF_V2 p) { CF_M3x2 m; m.m.x = cf_v2(s.x, 0); m.m.y = cf_v2(0, s.y); m.p = p; return m; }

/**
 * @function cf_make_scale_translation_f
 * @category math
 * @brief    Returns a `CF_M3x2` that represents a scale + translation.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_make_scale_translation_f(float s, CF_V2 p) { return cf_make_scale_translation(cf_v2(s, s), p); }

/**
 * @function cf_make_scale_translation_f_f
 * @category math
 * @brief    Returns a `CF_M3x2` that represents a scale + translation.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_make_scale_translation_f_f(float sx, float sy, CF_V2 p) { return cf_make_scale_translation(cf_v2(sx, sy), p); }

/**
 * @function cf_make_rotation
 * @category math
 * @brief    Returns a `CF_M3x2` that represents a rotation.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_make_rotation(float radians) { CF_SinCos sc = cf_sincos_f(radians); CF_M3x2 m; m.m.x = cf_v2(sc.c, -sc.s); m.m.y = cf_v2(sc.s, sc.c); m.p = cf_v2(0, 0); return m; }

/**
 * @function cf_make_transform_TSR
 * @category math
 * @brief    Returns a `CF_M3x2` that represents a translation + scale + rotation.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_make_transform_TSR(CF_V2 p, CF_V2 s, float radians) { CF_SinCos sc = cf_sincos_f(radians); CF_M3x2 m; m.m.x = cf_mul_v2_f(cf_v2(sc.c, -sc.s), s.x); m.m.y = cf_mul_v2_f(cf_v2(sc.s, sc.c), s.y); m.p = p; return m; }

/**
 * @function cf_invert
 * @category math
 * @brief    Returns a `CF_M3x2` inverted.
 * @remarks  If the determinant is zero, the returned matrix is also zero.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_invert(CF_M3x2 a)
{
	float id = cf_safe_invert(cf_det2(a.m.x, a.m.y));
	CF_M3x2 b;
	b.m.x = cf_v2(a.m.y.y * id, -a.m.x.y * id);
	b.m.y = cf_v2(-a.m.y.x * id, a.m.x.x * id);
	b.p.x = (a.m.y.x * a.p.y - a.p.x * a.m.y.y) * id;
	b.p.y = (a.p.x * a.m.x.y - a.m.x.x * a.p.y) * id;
	return b;
}

/**
 * @function cf_ortho_2d
 * @category math
 * @brief    Constructs an orthographic projection matrix given position and scaling values.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_ortho_2d(float x, float y, float scale_x, float scale_y)
{
	CF_M3x2 matrix;
	matrix.m.x.x = 2.0f / scale_x;
	matrix.m.x.y = 0.0f;
	matrix.m.y.x = 0.0f;
	matrix.m.y.y = 2.0f / scale_y;
	matrix.p.x = -2.0f * x / scale_x;
	matrix.p.y = -2.0f * y / scale_y;
	return matrix;
}

//--------------------------------------------------------------------------------------------------
// Transform ops.
// No scale factor allowed here, good for physics + colliders.

/**
 * @function cf_make_transform
 * @category math
 * @brief    Returns an identity `CF_Transform`.
 * @related  CF_Transform cf_make_transform cf_make_transform_TR cf_mul_tf_v2 cf_mulT_tf_v2 cf_mul_tf cf_mulT_tf
 */
CF_INLINE CF_Transform cf_make_transform() { CF_Transform x; x.p = cf_v2(0, 0); x.r = cf_sincos(); return x; }

/**
 * @function cf_make_transform_TR
 * @category math
 * @brief    Returns a `CF_Transform` that represents a translation + rotation.
 * @related  CF_Transform cf_make_transform cf_make_transform_TR cf_mul_tf_v2 cf_mulT_tf_v2 cf_mul_tf cf_mulT_tf
 */
CF_INLINE CF_Transform cf_make_transform_TR(CF_V2 p, float radians) { CF_Transform x; x.r = cf_sincos_f(radians); x.p = p; return x; }

/**
 * @function cf_mul_tf_v2
 * @category math
 * @brief    Returns a vector multiplied by a `CF_Transform`.
 * @related  CF_Transform cf_make_transform cf_make_transform_TR cf_mul_tf_v2 cf_mulT_tf_v2 cf_mul_tf cf_mulT_tf
 */
CF_INLINE CF_V2 cf_mul_tf_v2(CF_Transform a, CF_V2 b) { return cf_add_v2(cf_mul_sc_v2(a.r, b), a.p); }

/**
 * @function cf_mulT_tf_v2
 * @category math
 * @brief    Returns a vector multiplied by an inverted `CF_Transform`.
 * @related  CF_Transform cf_make_transform cf_make_transform_TR cf_mul_tf_v2 cf_mulT_tf_v2 cf_mul_tf cf_mulT_tf
 */
CF_INLINE CF_V2 cf_mulT_tf_v2(CF_Transform a, CF_V2 b) { return cf_mulT_sc_v2(a.r, cf_sub_v2(b, a.p)); }

/**
 * @function cf_mul_tf
 * @category math
 * @brief    Returns a the composition of multiplying two `CF_Transform`s.
 * @related  CF_Transform cf_make_transform cf_make_transform_TR cf_mul_tf_v2 cf_mulT_tf_v2 cf_mul_tf cf_mulT_tf
 */
CF_INLINE CF_Transform cf_mul_tf(CF_Transform a, CF_Transform b) { CF_Transform c; c.r = cf_mul_sc(a.r, b.r); c.p = cf_add_v2(cf_mul_sc_v2(a.r, b.p), a.p); return c; }


/**
 * @function cf_mulT_tf
 * @category math
 * @brief    Returns `a` multiplied by inverse `b`.
 * @related  CF_Transform cf_make_transform cf_make_transform_TR cf_mul_tf_v2 cf_mulT_tf_v2 cf_mul_tf cf_mulT_tf
 */
CF_INLINE CF_Transform cf_mulT_tf(CF_Transform a, CF_Transform b) { CF_Transform c; c.r = cf_mulT_sc(a.r, b.r); c.p = cf_mulT_sc_v2(a.r, cf_sub_v2(b.p, a.p)); return c; }

//--------------------------------------------------------------------------------------------------
// Halfspace (plane/line) ops.
// Functions for infinite lines.

/**
 * @function cf_plane
 * @category math
 * @brief    Returns an initialized `CF_Halfpsace` (a 2-dimensional plane, aka line).
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mulT_tf_hs cf_intersect_halfspace
 */
CF_INLINE CF_Halfspace cf_plane(CF_V2 n, float d) { CF_Halfspace h; h.n = n; h.d = d; return h; }

/**
 * @function cf_plane2
 * @category math
 * @brief    Returns an initialized `CF_Halfpsace` (a 2-dimensional plane, aka line).
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mulT_tf_hs cf_intersect_halfspace
 */
CF_INLINE CF_Halfspace cf_plane2(CF_V2 n, CF_V2 p) { CF_Halfspace h; h.n = n; h.d = cf_dot(n, p); return h; }

/**
 * @function cf_origin
 * @category math
 * @brief    Returns the origin projected onto the plane.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mulT_tf_hs cf_intersect_halfspace
 */
CF_INLINE CF_V2 cf_origin(CF_Halfspace h) { return cf_mul_v2_f(h.n, h.d); }

/**
 * @function cf_distance_hs
 * @category math
 * @brief    Returns distance of a point to the plane.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mulT_tf_hs cf_intersect_halfspace
 */
CF_INLINE float cf_distance_hs(CF_Halfspace h, CF_V2 p) { return cf_dot(h.n, p) - h.d; }

/**
 * @function cf_project
 * @category math
 * @brief    Projects a point onto the surface of the plane.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mulT_tf_hs cf_intersect_halfspace
 */
CF_INLINE CF_V2 cf_project(CF_Halfspace h, CF_V2 p) { return cf_sub_v2(p, cf_mul_v2_f(h.n, cf_distance_hs(h, p))); }

/**
 * @function cf_mul_tf_hs
 * @category math
 * @brief    Transforms a plane by a `CF_Transform`.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mulT_tf_hs cf_intersect_halfspace
 */
CF_INLINE CF_Halfspace cf_mul_tf_hs(CF_Transform a, CF_Halfspace b) { CF_Halfspace c; c.n = cf_mul_sc_v2(a.r, b.n); c.d = cf_dot(cf_mul_tf_v2(a, cf_origin(b)), c.n); return c; }

/**
 * @function cf_mulT_tf_hs
 * @category math
 * @brief    Transforms a plane by an inverted `CF_Transform`.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mulT_tf_hs cf_intersect_halfspace
 */
CF_INLINE CF_Halfspace cf_mulT_tf_hs(CF_Transform a, CF_Halfspace b) { CF_Halfspace c; c.n = cf_mulT_sc_v2(a.r, b.n); c.d = cf_dot(cf_mulT_tf_v2(a, cf_origin(b)), c.n); return c; }

/**
 * @function cf_intersect_halfspace
 * @category math
 * @brief    Returns the intersection point of two points to a plane.
 * @remarks  The distance to the plane are provided as `da` and `db`. You can compute these with e.g. `cf_distance_hs`, or instead
 *           call the similar function `cf_intersect_halfspace2`.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mulT_tf_hs cf_intersect_halfspace cf_intersect_haflspace2 cf_intersect_haflspace3
 */
CF_INLINE CF_V2 cf_intersect_halfspace(CF_V2 a, CF_V2 b, float da, float db) { return cf_add_v2(a, cf_mul_v2_f(cf_sub_v2(b, a), (da / (da - db)))); }

/**
 * @function cf_intersect_halfspace2
 * @category math
 * @brief    Returns the intersection point of two points to a plane.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mulT_tf_hs cf_intersect_halfspace cf_intersect_haflspace2 cf_intersect_haflspace3
 */
CF_INLINE CF_V2 cf_intersect_halfspace2(CF_Halfspace h, CF_V2 a, CF_V2 b) { return cf_intersect_halfspace(a, b, cf_distance_hs(h, a), cf_distance_hs(h, b)); }

/**
 * @function cf_intersect_halfspace3
 * @category math
 * @brief    Returns the intersection point of two planes.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mulT_tf_hs cf_intersect_halfspace cf_intersect_haflspace2 cf_intersect_haflspace3
 */
CF_INLINE CF_V2 cf_intersect_halfspace3(CF_Halfspace ha, CF_Halfspace hb) { CF_V2 a = {ha.n.x, hb.n.x}, b = {ha.n.y, hb.n.y}, c = {ha.d, hb.d}; float x = cf_det2(c, b) / cf_det2(a, b); float y = cf_det2(a, c) / cf_det2(a, b); return cf_v2(x, y); }

/**
 * @function cf_shift
 * @category math
 * @brief    Returns a plane shifted along it's normal by distance `d`.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mulT_tf_hs cf_intersect_halfspace cf_intersect_haflspace2 cf_intersect_haflspace3
 */
CF_INLINE CF_Halfspace cf_shift(CF_Halfspace h, float d) { h.d += d; return h; }

/**
 * @function cf_parallel
 * @category math
 * @brief    Returns true if two vectors are parallel within a `tol` tolerance value.
 * @remarks  You should experiment to find a good `tol` value, such as commonly used values like 1.0e-3f, 1.0e-6f, or 1.0e-8f.
 *           Different orders of magnitude are suitable for different tasks, so it may take some experience to figure out
 *           what a good tolerance is for your situation.
 * @related  CF_V2 cf_lesser_v2 cf_greater_v2 cf_lesser_equal_v2 cf_greater_equal_v2 cf_parallel cf_parallel2
 */
CF_INLINE bool cf_parallel(CF_V2 a, CF_V2 b, float tol)
{
	float k = cf_len(a) / cf_len(b);
	b = cf_mul_v2_f(b, k);
	if (CF_FABSF(a.x - b.x) < tol && CF_FABSF(a.y - b.y) < tol) {
		return true;
	} else {
		return false;
	}
}

/**
 * @function cf_parallel2
 * @category math
 * @brief    Returns true the planes a-b and b-c are parallel with a distance tolerance.
 * @remarks  You should experiment to find a good `tol` value, such as commonly used values like 1.0e-3f, 1.0e-6f, or 1.0e-8f.
 *           Different orders of magnitude are suitable for different tasks, so it may take some experience to figure out
 *           what a good tolerance is for your situation.
 * @related  CF_V2 cf_lesser_v2 cf_greater_v2 cf_lesser_equal_v2 cf_greater_equal_v2 cf_parallel cf_parallel2
 */
CF_INLINE bool cf_parallel2(CF_V2 a, CF_V2 b, CF_V2 c, float tol)
{
	CF_Halfspace h0 = cf_plane2(cf_skew(cf_norm(cf_sub_v2(b,a))), a);
	CF_Halfspace h1 = cf_plane2(cf_skew(cf_norm(cf_sub_v2(c,b))), b);
	float d0 = cf_abs(cf_distance_hs(h0, c));
	float d1 = cf_abs(cf_distance_hs(h1, a));
	return d0 < tol && d1 < tol;
}

//--------------------------------------------------------------------------------------------------
// Shape helpers.

/**
 * @function cf_make_circle
 * @category math
 * @brief    Returns a circle.
 * @related  CF_Circle
 */
CF_INLINE CF_Circle cf_make_circle(CF_V2 pos, float radius) { CF_Circle c; c.p = pos; c.r = radius; return c; }

/**
 * @function cf_make_capsule
 * @category math
 * @brief    Returns a capsule.
 * @related  CF_Capsule
 */
CF_INLINE CF_Capsule cf_make_capsule(CF_V2 a, CF_V2 b, float radius) { CF_Capsule c; c.a = a; c.b = b; c.r = radius; return c; }

/**
 * @function cf_make_capsule2
 * @category math
 * @brief    Returns a capsule.
 * @param    p          The position the capsule stands upon.
 * @param    height     The total length of the capsule, from end to end.
 * @param    radius     How thick the capsule is. Make sure this is less than or equal to height (or you'll get a sphere).
 * @remarks  The `height` is measured from the end of each cap, covering the full length of the capsule. The capsule is constructed
 *           standing straight up on the y-axis. You should make sure `height` is taller than the `radius`. `a` is the center of the
 *           bottom cap, while `b` is the center of the top cap.
 * @related  CF_Capsule
 */
CF_INLINE CF_Capsule cf_make_capsule2(CF_V2 p, float height, float radius)
{
	CF_Capsule c;
	c.r = radius;

	if (height < radius) {
		c.a = c.b = p;
	} else {
		c.a.x = c.b.x = p.x;
		c.a.y = p.y + radius;
		c.b.y = c.a.y + height - radius * 2;
	}

	return c;
}

/**
 * @function cf_make_aabb
 * @category math
 * @brief    Returns an AABB (axis-aligned bounding box) from min/max points (bottom-left and top-right).
 * @related  CF_Aabb cf_make_aabb cf_make_aabb_pos_w_h cf_make_aabb_center_half_extents cf_make_aabb_from_top_left
 */
CF_INLINE CF_Aabb cf_make_aabb(CF_V2 min, CF_V2 max) { CF_Aabb bb; bb.min = min; bb.max = max; return bb; }

/**
 * @function cf_make_aabb_pos_w_h
 * @category math
 * @brief    Returns an AABB (axis-aligned bounding box).
 * @related  CF_Aabb cf_make_aabb cf_make_aabb_pos_w_h cf_make_aabb_center_half_extents cf_make_aabb_from_top_left
 */
CF_INLINE CF_Aabb cf_make_aabb_pos_w_h(CF_V2 pos, float w, float h) { CF_Aabb bb; CF_V2 he = cf_mul_v2_f(cf_v2(w, h), 0.5f); bb.min = cf_sub_v2(pos, he); bb.max = cf_add_v2(pos, he); return bb; }

/**
 * @function cf_make_aabb_center_half_extents
 * @category math
 * @brief    Returns an AABB (axis-aligned bounding box).
 * @remarks  Half-extents refer to half-width and height-height: `half_extents = { half_width, half_height }`.
 * @related  CF_Aabb cf_make_aabb cf_make_aabb_pos_w_h cf_make_aabb_center_half_extents cf_make_aabb_from_top_left
 */
CF_INLINE CF_Aabb cf_make_aabb_center_half_extents(CF_V2 center, CF_V2 half_extents) { CF_Aabb bb; bb.min = cf_sub_v2(center, half_extents); bb.max = cf_add_v2(center, half_extents); return bb; }

/**
 * @function cf_make_aabb_from_top_left
 * @category math
 * @brief    Returns an AABB (axis-aligned bounding box).
 * @related  CF_Aabb cf_make_aabb cf_make_aabb_pos_w_h cf_make_aabb_center_half_extents cf_make_aabb_from_top_left
 */
CF_INLINE CF_Aabb cf_make_aabb_from_top_left(CF_V2 top_left, float w, float h) { return cf_make_aabb(cf_add_v2(top_left, cf_v2(0, -h)), cf_add_v2(top_left, cf_v2(w, 0))); }

/**
 * @function cf_width
 * @category math
 * @brief    Returns the width of an AABB (axis-aligned bounding box).
 * @related  CF_Aabb cf_make_aabb cf_width cf_height cf_half_width cf_half_height cf_half_extents cf_extents
 */
CF_INLINE float cf_width(CF_Aabb bb) { return bb.max.x - bb.min.x; }

/**
 * @function cf_height
 * @category math
 * @brief    Returns the height of an AABB (axis-aligned bounding box).
 * @related  CF_Aabb cf_make_aabb cf_width cf_height cf_half_width cf_half_height cf_half_extents cf_extents
 */
CF_INLINE float cf_height(CF_Aabb bb) { return bb.max.y - bb.min.y; }

/**
 * @function cf_half_width
 * @category math
 * @brief    Returns the half-width of an AABB (axis-aligned bounding box).
 * @related  CF_Aabb cf_make_aabb cf_width cf_height cf_half_width cf_half_height cf_half_extents cf_extents
 */
CF_INLINE float cf_half_width(CF_Aabb bb) { return cf_width(bb) * 0.5f; }

/**
 * @function cf_half_height
 * @category math
 * @brief    Returns the half-height of an AABB (axis-aligned bounding box).
 * @related  CF_Aabb cf_make_aabb cf_width cf_height cf_half_width cf_half_height cf_half_extents cf_extents
 */
CF_INLINE float cf_half_height(CF_Aabb bb) { return cf_height(bb) * 0.5f; }

/**
 * @function cf_half_extents
 * @category math
 * @brief    Returns the half-extents of an AABB (axis-aligned bounding box).
 * @remarks  Half-extents refer to half-width and height-height: `half_extents = { half_width, half_height }`.
 * @related  CF_Aabb cf_make_aabb cf_width cf_height cf_half_width cf_half_height cf_half_extents cf_extents
 */
CF_INLINE CF_V2 cf_half_extents(CF_Aabb bb) { return (cf_mul_v2_f(cf_sub_v2(bb.max, bb.min), 0.5f)); }

/**
 * @function cf_extents
 * @category math
 * @brief    Returns the extents of an AABB (axis-aligned bounding box).
 * @remarks  Extents refer to width and height: `extents = { width, height }`.
 * @related  CF_Aabb cf_make_aabb cf_width cf_height cf_half_width cf_half_height cf_half_extents cf_extents
 */
CF_INLINE CF_V2 cf_extents(CF_Aabb aabb) { return cf_sub_v2(aabb.max, aabb.min); }

/**
 * @function cf_expand_aabb
 * @category math
 * @brief    Expands an AABB (axis-aligned bounding box).
 * @param    v      A vector of `{ half_width, half_height }` to expand by.
 * @related  CF_Aabb cf_make_aabb cf_expand_aabb cf_expand_aabb_f
 */
CF_INLINE CF_Aabb cf_expand_aabb(CF_Aabb aabb, CF_V2 v) { return cf_make_aabb(cf_sub_v2(aabb.min, v), cf_add_v2(aabb.max, v)); }

/**
 * @function cf_expand_aabb_f
 * @category math
 * @brief    Expands an AABB (axis-aligned bounding box).
 * @remarks  `v` is added to to `max.x` and `max.y` of `aabb`, and subtracted from `min.x` and `min.y` of `aabb`.
 * @related  CF_Aabb cf_make_aabb cf_expand_aabb cf_expand_aabb_f
 */
CF_INLINE CF_Aabb cf_expand_aabb_f(CF_Aabb aabb, float v) { CF_V2 factor = cf_v2(v, v); return cf_make_aabb(cf_sub_v2(aabb.min, factor), cf_add_v2(aabb.max, factor)); }

/**
 * @function cf_min_aabb
 * @category math
 * @brief    Returns `bb.min`.
 * @related  CF_Aabb cf_min_aabb cf_max_aabb cf_midpoint cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right
 */
CF_INLINE CF_V2 cf_min_aabb(CF_Aabb bb) { return bb.min; }

/**
 * @function cf_max_aabb
 * @category math
 * @brief    Returns `bb.max`.
 * @related  CF_Aabb cf_min_aabb cf_max_aabb cf_midpoint cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right
 */
CF_INLINE CF_V2 cf_max_aabb(CF_Aabb bb) { return bb.max; }

/**
 * @function cf_midpoint
 * @category math
 * @brief    Returns the center of `bb`.
 * @related  CF_Aabb cf_min_aabb cf_max_aabb cf_midpoint cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right
 */
CF_INLINE CF_V2 cf_midpoint(CF_Aabb bb) { return cf_mul_v2_f(cf_add_v2(bb.min, bb.max), 0.5f); }

/**
 * @function cf_center
 * @category math
 * @brief    Returns the center of `bb`.
 * @related  CF_Aabb cf_min_aabb cf_max_aabb cf_midpoint cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right
 */
CF_INLINE CF_V2 cf_center(CF_Aabb bb) { return cf_mul_v2_f(cf_add_v2(bb.min, bb.max), 0.5f); }

/**
 * @function cf_top_left
 * @category math
 * @brief    Returns the top-left corner of bb.
 * @related  CF_Aabb cf_min_aabb cf_max_aabb cf_midpoint cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right
 */
CF_INLINE CF_V2 cf_top_left(CF_Aabb bb) { return cf_v2(bb.min.x, bb.max.y); }

/**
 * @function cf_top_right
 * @category math
 * @brief    Returns the top-right corner of bb.
 * @related  CF_Aabb cf_min_aabb cf_max_aabb cf_midpoint cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right
 */
CF_INLINE CF_V2 cf_top_right(CF_Aabb bb) { return cf_v2(bb.max.x, bb.max.y); }

/**
 * @function cf_bottom_left
 * @category math
 * @brief    Returns the bottom-left corner of bb.
 * @related  CF_Aabb cf_min_aabb cf_max_aabb cf_midpoint cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right
 */
CF_INLINE CF_V2 cf_bottom_left(CF_Aabb bb) { return cf_v2(bb.min.x, bb.min.y); }

/**
 * @function cf_bottom_right
 * @category math
 * @brief    Returns the bottom-right corner of bb.
 * @related  CF_Aabb cf_min_aabb cf_max_aabb cf_midpoint cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right
 */
CF_INLINE CF_V2 cf_bottom_right(CF_Aabb bb) { return cf_v2(bb.max.x, bb.min.y); }

/**
 * @function cf_top
 * @category math
 * @brief    Returns the top of the aabb.
 * @related  CF_Aabb cf_min_aabb cf_max_aabb cf_midpoint cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right
 */
CF_INLINE CF_V2 cf_top(CF_Aabb bb) { return cf_v2((bb.min.x + bb.max.x) * 0.5f, bb.max.y); }

/**
 * @function cf_left
 * @category math
 * @brief    Returns the left of the aabb.
 * @related  CF_Aabb cf_min_aabb cf_max_aabb cf_midpoint cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right
 */
CF_INLINE CF_V2 cf_left(CF_Aabb bb) { return cf_v2(bb.min.x, (bb.min.y + bb.max.y) * 0.5f); }

/**
 * @function cf_bottom
 * @category math
 * @brief    Returns the bottom of the aabb.
 * @related  CF_Aabb cf_min_aabb cf_max_aabb cf_midpoint cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right
 */
CF_INLINE CF_V2 cf_bottom(CF_Aabb bb) { return cf_v2((bb.min.x + bb.max.x) * 0.5f, bb.min.y); }

/**
 * @function cf_right
 * @category math
 * @brief    Returns the bottom of the aabb.
 * @related  CF_Aabb cf_min_aabb cf_max_aabb cf_midpoint cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right
 */
CF_INLINE CF_V2 cf_right(CF_Aabb bb) { return cf_v2(bb.max.x, (bb.min.y + bb.max.y) * 0.5f); }

/**
 * @function cf_contains_point
 * @category math
 * @brief    Returns true if `p` is contained within `b`.
 * @related  CF_Aabb cf_contains_point cf_contains_aabb cf_surface_area_aabb cf_area_aabb cf_clamp_aabb_v2 cf_clamp_aabb cf_combine cf_overlaps
 */
CF_INLINE bool cf_contains_point(CF_Aabb bb, CF_V2 p) { return cf_greater_equal_v2(p, bb.min) && cf_lesser_equal_v2(p, bb.max); }

/**
 * @function cf_contains_aabb
 * @category math
 * @brief    Returns true if `a` is _fully_ contained within `b`.
 * @related  CF_Aabb cf_contains_point cf_contains_aabb cf_surface_area_aabb cf_area_aabb cf_clamp_aabb_v2 cf_clamp_aabb cf_combine cf_overlaps
 */
CF_INLINE bool cf_contains_aabb(CF_Aabb a, CF_Aabb b) { return cf_lesser_equal_v2(a.min, b.min) && cf_greater_equal_v2(a.max, b.max); }

/**
 * @function cf_surface_area_aabb
 * @category math
 * @brief    Returns the surface area of a `CF_Aabb`.
 * @related  CF_Aabb cf_contains_point cf_contains_aabb cf_surface_area_aabb cf_area_aabb cf_clamp_aabb_v2 cf_clamp_aabb cf_combine cf_overlaps
 */
CF_INLINE float cf_surface_area_aabb(CF_Aabb bb) { return 2.0f * cf_width(bb) * cf_height(bb); }

/**
 * @function cf_area_aabb
 * @category math
 * @brief    Returns the area of a `CF_Aabb`.
 * @related  CF_Aabb cf_contains_point cf_contains_aabb cf_surface_area_aabb cf_area_aabb cf_clamp_aabb_v2 cf_clamp_aabb cf_combine cf_overlaps
 */
CF_INLINE float cf_area_aabb(CF_Aabb bb) { return cf_width(bb) * cf_height(bb); }

/**
 * @function cf_clamp_aabb_v2
 * @category math
 * @brief    Returns a point clamped within a `CF_Aabb`.
 * @related  CF_Aabb cf_contains_point cf_contains_aabb cf_surface_area_aabb cf_area_aabb cf_clamp_aabb_v2 cf_clamp_aabb cf_combine cf_overlaps
 */
CF_INLINE CF_V2 cf_clamp_aabb_v2(CF_Aabb bb, CF_V2 p) { return cf_clamp_v2(p, bb.min, bb.max); }

/**
 * @function cf_clamp_aabb
 * @category math
 * @brief    Returns `a` clamped within `b`.
 * @related  CF_Aabb cf_contains_point cf_contains_aabb cf_surface_area_aabb cf_area_aabb cf_clamp_aabb_v2 cf_clamp_aabb cf_combine cf_overlaps
 */
CF_INLINE CF_Aabb cf_clamp_aabb(CF_Aabb a, CF_Aabb b) { return cf_make_aabb(cf_clamp_v2(a.min, b.min, b.max), cf_clamp_v2(a.max, b.min, b.max)); }

/**
 * @function cf_combine
 * @category math
 * @brief    Returns a `CF_Aabb` that tightly contains both `a` and `b`.
 * @related  CF_Aabb cf_contains_point cf_contains_aabb cf_surface_area_aabb cf_area_aabb cf_clamp_aabb_v2 cf_clamp_aabb cf_combine cf_overlaps
 */
CF_INLINE CF_Aabb cf_combine(CF_Aabb a, CF_Aabb b) { return cf_make_aabb(cf_min_v2(a.min, b.min), cf_max_v2(a.max, b.max)); }

/**
 * @function cf_overlaps
 * @category math
 * @brief    Returns true if `a` and `b` intersect.
 * @related  CF_Aabb cf_overlaps cf_collide_aabb
 */
CF_INLINE int cf_overlaps(CF_Aabb a, CF_Aabb b)
{
	int d0 = b.max.x < a.min.x;
	int d1 = a.max.x < b.min.x;
	int d2 = b.max.y < a.min.y;
	int d3 = a.max.y < b.min.y;
	return !(d0 | d1 | d2 | d3);
}

/**
 * @function cf_collide_aabb
 * @category math
 * @brief    Returns true if `a` and `b` intersect.
 * @related  CF_Aabb cf_overlaps cf_collide_aabb
 */
CF_INLINE int cf_collide_aabb(CF_Aabb a, CF_Aabb b) { return cf_overlaps(a, b); }

/**
 * @function cf_make_aabb_verts
 * @category math
 * @brief    Returns a `CF_Aabb` that tightly contains all input verts.
 * @related  CF_Aabb cf_make_aabb_verts cf_aabb_verts
 */
CF_INLINE CF_Aabb cf_make_aabb_verts(CF_V2* verts, int count)
{
	CF_V2 vmin = verts[0];
	CF_V2 vmax = vmin;
	for (int i = 0; i < count; ++i) {
		vmin = cf_min_v2(vmin, verts[i]);
		vmax = cf_max_v2(vmax, verts[i]);
	}
	return cf_make_aabb(vmin, vmax);
}

/**
 * @function cf_aabb_verts
 * @category math
 * @brief    Fills in `out` with four vertices, one for each corner of `bb`, in counter-clockwise order.
 * @related  CF_Aabb cf_make_aabb_verts cf_aabb_verts
 */
CF_INLINE void cf_aabb_verts(CF_V2* out, CF_Aabb bb)
{
	out[0] = bb.min;
	out[1] = cf_v2(bb.max.x, bb.min.y);
	out[2] = bb.max;
	out[3] = cf_v2(bb.min.x, bb.max.y);
}

//--------------------------------------------------------------------------------------------------
// Circle helpers.

/**
 * @function cf_area_circle
 * @category math
 * @brief    Returns the area of a `CF_Circle`.
 * @related  CF_Circle cf_area_circle cf_surface_area_circle
 */
CF_INLINE float cf_area_circle(CF_Circle c) { return 3.14159265f * c.r * c.r; }

/**
 * @function cf_surface_area_circle
 * @category math
 * @brief    Returns the surface area of a `CF_Circle`.
 * @related  CF_Circle cf_area_circle cf_surface_area_circle
 */
CF_INLINE float cf_surface_area_circle(CF_Circle c) { return 2.0f * 3.14159265f * c.r; }

/**
 * @function cf_mul_tf_circle
 * @category math
 * @brief    Transforms a `CF_Circle`.
 * @related  CF_Circle cf_mul_tf_circle
 */
CF_INLINE CF_Circle cf_mul_tf_circle(CF_Transform tx, CF_Circle a) { CF_Circle b; b.p = cf_mul_tf_v2(tx, a.p); b.r = a.r; return b; }

//--------------------------------------------------------------------------------------------------
// Ray ops.
// Full raycasting suite is farther down below in this file.

/**
 * @function cf_make_ray
 * @category collision
 * @brief    Returns an initialize `CF_Ray`.
 * @related  CF_Ray
 */
CF_INLINE CF_Ray cf_make_ray(CF_V2 start, CF_V2 direction_normalized, float length) { CF_Ray ray; ray.p = start; ray.d = direction_normalized; ray.t = length; return ray; }

/**
 * @function cf_impact
 * @category collision
 * @brief    Returns the impact point of a ray, given the time of impact `t`.
 * @related  CF_Ray cf_impact cf_endpoint
 */
CF_INLINE CF_V2 cf_impact(CF_Ray r, float t) { return cf_add_v2(r.p, cf_mul_v2_f(r.d, t)); }

/**
 * @function cf_endpoint
 * @category collision
 * @brief    Returns the endpoint of a ray.
 * @remarks  Rays are defined to have an endpoint as an optimization. Usually infinite rays are not needed in games, and cause
 *           unnecessarily large computations when doing raycasts.
 * @related  CF_Ray cf_impact cf_endpoint
 */
CF_INLINE CF_V2 cf_endpoint(CF_Ray r) { return cf_add_v2(r.p, cf_mul_v2_f(r.d, r.t)); }

/**
 * @function cf_ray_to_halfspace
 * @category collision
 * @brief    Returns a raycast to a halfspace (plane)
 * @param    A          The ray.
 * @param    B          The plane.
 * @param    Returns a `CF_Raycast` containing results about the raycast.
 * @related  CF_Ray
 */
CF_INLINE CF_Raycast cf_ray_to_halfspace(CF_Ray A, CF_Halfspace B)
{
	CF_Raycast result = { 0 };
	float da = cf_distance_hs(B, A.p);
	float db = cf_distance_hs(B, cf_impact(A, A.t));
	if (da * db > 0) return result;
	result.n = cf_mul_v2_f(B.n, cf_sign(da));
	result.t = cf_intersect(da, db);
	result.hit = true;
	return result;
}

/**
 * @function cf_distance_sq
 * @category math
 * @brief    Returns the square distance of a point to a line segment.
 * @param    a          The start point of a line segment.
 * @param    b          The end point of a line segment.
 * @param    p          The query point.
 * @remarks  See [this article](https://randygaul.github.io/math/collision-detection/2014/07/01/Distance-Point-to-Line-Segment.html) for implementation details.
 * @related  CF_V2 CF_Ray
 */
CF_INLINE float cf_distance_sq(CF_V2 a, CF_V2 b, CF_V2 p)
{
	CF_V2 n = cf_sub_v2(b, a);
	CF_V2 pa = cf_sub_v2(a, p);
	float c = cf_dot(n, pa);

	// Closest point is a
	if (c > 0.0f) return cf_dot(pa, pa);

	// Closest point is b
	CF_V2 bp = cf_sub_v2(p, b);
	if (cf_dot(n, bp) > 0.0f) return cf_dot(bp, bp);

	// Closest point is between a and b
	CF_V2 e = cf_sub_v2(pa, cf_mul_v2_f(n, (c / cf_dot(n, n))));
	return cf_dot(e, e);
}

//--------------------------------------------------------------------------------------------------
// Polygonal functions.

/**
 * @function cf_center_of_mass
 * @category collision
 * @brief    TODO
 * @related  TODO
 */
CF_API CF_V2 CF_CALL cf_center_of_mass(CF_Poly poly);

/**
 * @function cf_calc_area
 * @category collision
 * @brief    TODO
 * @related  TODO
 */
CF_API float CF_CALL cf_calc_area(CF_Poly poly);

/**
 * @struct   CF_SliceOutput
 * @category collision
 * @brief    TODO
 * @related  TODO
 */
typedef struct CF_SliceOutput
{
	/* @member TODO */
	CF_Poly front;

	/* @member TODO */
	CF_Poly back;
} CF_SliceOutput;
// @end

/**
 * @function cf_slice
 * @category collision
 * @brief    TODO
 * @related  TODO
 */
CF_API CF_SliceOutput CF_CALL cf_slice(CF_Halfspace slice_plane, CF_Poly slice_me, const float k_epsilon);

/**
 * @enum     CF_ShapeType
 * @category collision
 * @brief    Various types of supported collision shapes.
 * @related  CF_ShapeType cf_shape_type_to_string cf_gjk cf_toi cf_inflate cf_collide cf_collided cf_cast_ray
 */
#define CF_SHAPE_TYPE_DEFS \
	/* @entry Unknown shape type. */      \
	CF_ENUM(SHAPE_TYPE_NONE,    0)        \
	/* @entry `CF_Circle` shape type. */  \
	CF_ENUM(SHAPE_TYPE_CIRCLE,  1)        \
	/* @entry `CF_Aabb` shape type. */    \
	CF_ENUM(SHAPE_TYPE_AABB,    2)        \
	/* @entry `CF_Capsule` shape type. */ \
	CF_ENUM(SHAPE_TYPE_CAPSULE, 3)        \
	/* @entry `CF_Poly` shape type. */    \
	CF_ENUM(SHAPE_TYPE_POLY,    4)        \
	/* @end */

typedef enum CF_ShapeType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_SHAPE_TYPE_DEFS
	#undef CF_ENUM
} CF_ShapeType;

/**
 * @function cf_shape_type_to_string
 * @category collision
 * @brief    Converts a `CF_ShapeType` to a C string.
 * @param    type       The string to convert.
 * @related  CF_ShapeType cf_shape_type_to_string
 */
CF_INLINE const char* cf_shape_type_to_string(CF_ShapeType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_SHAPE_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_inflate
 * @category collision
 * @brief    Inflates a shape.
 * @param    shape        The shape.
 * @param    type         The `CF_ShapeType` of `shape`.
 * @param    skin_factor  The amount to inflate the shape by.
 * @remarks  This is useful to numerically grow or shrink a shape. For example, when calling a time of impact function it can be good to use
 *           a slightly smaller shape. Then, once both shapes are moved to the time of impact a collision manifold can be made from the
 *           slightly larger (and now overlapping) shapes.
 *           
 *           IMPORTANT NOTE
 *           
 *           Inflating a shape with sharp corners can cause those corners to move dramatically. Deflating a shape can avoid this problem,
 *           but deflating a very small shape can invert the planes and result in something that is no longer convex. Make sure to pick an
 *           appropriately small skin factor, for example 1.0e-6f.
 * @related  cf_gjk cf_toi CF_ShapeType
 */
CF_API void CF_CALL cf_inflate(void* shape, CF_ShapeType type, float skin_factor);

/**
 * @function cf_hull
 * @category collision
 * @brief    Computes 2D convex hull.
 * @param    verts        The vertices of the shape.
 * @param    count        The number of vertices in `verts`.
 * @param    skin_factor  The amount to inflate the shape by.
 * @return   Returns the number of vertices written to the `verts` array.
 * @remarks  Will not do anything if less than two verts supplied. If more than CF_POLY_MAX_VERTS are supplied extras are ignored.
 * @related  CF_Poly
 */
CF_API int CF_CALL cf_hull(CF_V2* verts, int count);

/**
 * @function cf_norms
 * @category collision
 * @brief    Computes the normals for a polygon.
 * @param    verts        The vertices of the polygon.
 * @param    norms        The normals of the polygon (these are written to as output).
 * @param    count        The number of vertices in `verts`.
 * @return   Writes the calculated normals to `norms`.
 * @related  CF_Poly
 */
CF_API void CF_CALL cf_norms(CF_V2* verts, CF_V2* norms, int count);

/**
 * @function cf_make_poly
 * @category collision
 * @brief    Fills out the polygon with values.
 * @remarks  Runs `cf_hull` and `cf_norms`, assumes p->verts and p->count are both set to valid values.
 * @related  CF_Poly cf_hull cf_norms
 */
CF_API void CF_CALL cf_make_poly(CF_Poly* p);

/**
 * @function cf_centroid
 * @category math
 * @brief    Returns the centroid of a set of vertices.
 */
CF_API CF_V2 CF_CALL cf_centroid(const CF_V2* verts, int count);

//--------------------------------------------------------------------------------------------------
// Collision detection.

// Boolean collision detection functions.
// These versions are slightly faster/simpler than the manifold versions, but only give a YES/NO result.

/**
 * @function cf_circle_to_circle
 * @category collision
 * @brief    Returns true if two circles are intersecting.
 * @remarks  For information about _how_ two shapes are intersecting (and not just boolean result), see `cf_circle_to_circle_manifold`.
 * @related  CF_Circle cf_circle_to_circle cf_circle_to_circle_manifold
 */
CF_API bool CF_CALL cf_circle_to_circle(CF_Circle A, CF_Circle B);

/**
 * @function cf_circle_to_aabb
 * @category collision
 * @brief    Returns true if a circle is intersecting with an Aabb.
 * @remarks  For information about _how_ two shapes are intersecting (and not just boolean result), see `cf_circle_to_aabb_manifold`.
 * @related  CF_Circle CF_Aabb cf_circle_to_aabb cf_circle_to_aabb_manifold
 */
CF_API bool CF_CALL cf_circle_to_aabb(CF_Circle A, CF_Aabb B);

/**
 * @function cf_circle_to_capsule
 * @category collision
 * @brief    Returns true if a circle is intersecting with an capsule.
 * @remarks  For information about _how_ two shapes are intersecting (and not just boolean result), see `cf_circle_to_capsule_manifold`.
 * @related  CF_Circle CF_Capsule cf_circle_to_capsule cf_circle_to_capsule_manifold
 */
CF_API bool CF_CALL cf_circle_to_capsule(CF_Circle A, CF_Capsule B);

/**
 * @function cf_aabb_to_aabb
 * @category collision
 * @brief    Returns true two Aabb's are intersecting.
 * @remarks  For information about _how_ two shapes are intersecting (and not just boolean result), see `cf_aabb_to_aabb_manifold`.
 * @related  CF_Aabb cf_aabb_to_aabb cf_aabb_to_aabb_manifold
 */
CF_API bool CF_CALL cf_aabb_to_aabb(CF_Aabb A, CF_Aabb B);

/**
 * @function cf_aabb_to_capsule
 * @category collision
 * @brief    Returns true if an Aabb is intersecting a capsule.
 * @remarks  For information about _how_ two shapes are intersecting (and not just boolean result), see `cf_aabb_to_capsule_manifold`.
 * @related  CF_Aabb CF_Capsule cf_aabb_to_capsule cf_aabb_to_capsule_manifold
 */
CF_API bool CF_CALL cf_aabb_to_capsule(CF_Aabb A, CF_Capsule B);

/**
 * @function cf_capsule_to_capsule
 * @category collision
 * @brief    Returns true if two capsules are intersecting.
 * @remarks  For information about _how_ two shapes are intersecting (and not just boolean result), see `cf_capsule_to_capsule_manifold`.
 * @related  CF_Capsule cf_capsule_to_capsule cf_capsule_to_capsule_manifold
 */
CF_API bool CF_CALL cf_capsule_to_capsule(CF_Capsule A, CF_Capsule B);

/**
 * @function cf_circle_to_poly
 * @category collision
 * @brief    Returns true if a circle is intersecting a polygon.
 * @remarks  For information about _how_ two shapes are intersecting (and not just boolean result), see `cf_circle_to_poly_manifold`.
 * @related  CF_Circle CF_Poly cf_circle_to_poly cf_circle_to_poly_manifold
 */
CF_API bool CF_CALL cf_circle_to_poly(CF_Circle A, const CF_Poly* B, const CF_Transform* bx);

/**
 * @function cf_aabb_to_poly
 * @category collision
 * @brief    Returns true an Aabb is intersecting a polygon.
 * @remarks  For information about _how_ two shapes are intersecting (and not just boolean result), see `cf_aabb_to_poly_manifold`.
 * @related  CF_Aabb CF_Poly cf_aabb_to_poly cf_aabb_to_poly_manifold
 */
CF_API bool CF_CALL cf_aabb_to_poly(CF_Aabb A, const CF_Poly* B, const CF_Transform* bx);

/**
 * @function cf_capsule_to_poly
 * @category collision
 * @brief    Returns true an capsule is intersecting a polygon.
 * @remarks  For information about _how_ two shapes are intersecting (and not just boolean result), see `cf_capsule_to_poly_manifold`.
 * @related  CF_Capsule CF_Poly cf_capsule_to_poly cf_capsule_to_poly_manifold
 */
CF_API bool CF_CALL cf_capsule_to_poly(CF_Capsule A, const CF_Poly* B, const CF_Transform* bx);

/**
 * @function cf_poly_to_poly
 * @category collision
 * @brief    Returns true if two polygons are intersecting.
 * @remarks  For information about _how_ two shapes are intersecting (and not just boolean result), see `cf_poly_to_poly_manifold`.
 * @related  CF_Poly cf_poly_to_poly cf_poly_to_poly_manifold
 */
CF_API bool CF_CALL cf_poly_to_poly(const CF_Poly* A, const CF_Transform* ax, const CF_Poly* B, const CF_Transform* bx);

/**
 * @function cf_ray_to_circle
 * @category collision
 * @brief    Returns a raycast to a circle.
 * @param    A          The ray.
 * @param    B          The circle.
 * @return   `CF_Raycast` results are placed here. See `CF_RayCast`.
 * @related  CF_Ray CF_Circle CF_Raycast cf_ray_to_circle cf_ray_to_aabb cf_ray_to_capsule cf_ray_to_poly
 */
CF_API CF_Raycast CF_CALL cf_ray_to_circle(CF_Ray A, CF_Circle B);

/**
 * @function cf_ray_to_aabb
 * @category collision
 * @brief    Returns a raycast to an aabb.
 * @param    A          The ray.
 * @param    B          The Aabb.
 * @return   `CF_Raycast` results are placed here. See `CF_RayCast`.
 * @related  CF_Ray CF_Aabb CF_Raycast cf_ray_to_circle cf_ray_to_aabb cf_ray_to_capsule cf_ray_to_poly
 */
CF_API CF_Raycast CF_CALL cf_ray_to_aabb(CF_Ray A, CF_Aabb B);

/**
 * @function cf_ray_to_capsule
 * @category collision
 * @brief    Returns a raycast to a capsule.
 * @param    A          The ray.
 * @param    B          The capsule.
 * @return   `CF_Raycast` results are placed here. See `CF_RayCast`.
 * @related  CF_Ray CF_Capsule CF_Raycast cf_ray_to_circle cf_ray_to_aabb cf_ray_to_capsule cf_ray_to_poly
 */
CF_API CF_Raycast CF_CALL cf_ray_to_capsule(CF_Ray A, CF_Capsule B);

/**
 * @function cf_ray_to_poly
 * @category collision
 * @brief    Returns a raycast to a polygon.
 * @param    A          The ray.
 * @param    B          The polygon.
 * @return   `CF_Raycast` results are placed here. See `CF_RayCast`.
 * @related  CF_Ray CF_Poly CF_Raycast cf_ray_to_circle cf_ray_to_aabb cf_ray_to_capsule cf_ray_to_poly
 */
CF_API CF_Raycast CF_CALL cf_ray_to_poly(CF_Ray A, const CF_Poly* B, const CF_Transform* bx_ptr);

/**
 * @function cf_circle_to_circle_manifold
 * @category collision
 * @brief    Computes information about how two shapes intersect.
 * @param    A          The first shape.
 * @param    B          The second shape.
 * @return   Returns a `CF_Manifold` containing information about the intersection. See `CF_Manifold` for details.
 * @remarks  This function is slightly slower than the boolean version `cf_circle_to_circle`.
 * @related  CF_Manifold CF_Circle cf_circle_to_circle_manifold cf_circle_to_aabb_manifold cf_circle_to_capsule_manifold cf_aabb_to_aabb_manifold cf_aabb_to_capsule_manifold cf_circle_to_poly_manifold cf_aabb_to_poly_manifold cf_capsule_to_poly_manifold cf_poly_to_poly_manifold
 */
CF_API CF_Manifold CF_CALL cf_circle_to_circle_manifold(CF_Circle A, CF_Circle B);

/**
 * @function cf_circle_to_aabb_manifold
 * @category collision
 * @brief    Computes information about how two shapes intersect.
 * @param    A          The first shape.
 * @param    B          The second shape.
 * @return   Returns a `CF_Manifold` containing information about the intersection. See `CF_Manifold` for details.
 * @remarks  This function is slightly slower than the boolean version `cf_circle_to_aabb`.
 * @related  CF_Manifold CF_Circle CF_Aabb cf_circle_to_circle_manifold cf_circle_to_aabb_manifold cf_circle_to_capsule_manifold cf_aabb_to_aabb_manifold cf_aabb_to_capsule_manifold cf_circle_to_poly_manifold cf_aabb_to_poly_manifold cf_capsule_to_poly_manifold cf_poly_to_poly_manifold
 */
CF_API CF_Manifold CF_CALL cf_circle_to_aabb_manifold(CF_Circle A, CF_Aabb B);

/**
 * @function cf_circle_to_capsule_manifold
 * @category collision
 * @brief    Computes information about how two shapes intersect.
 * @param    A          The first shape.
 * @param    B          The second shape.
 * @return   Returns a `CF_Manifold` containing information about the intersection. See `CF_Manifold` for details.
 * @remarks  This function is slightly slower than the boolean version `cf_circle_to_capsule`.
 * @related  CF_Manifold CF_Circle CF_Capsule cf_circle_to_circle_manifold cf_circle_to_aabb_manifold cf_circle_to_capsule_manifold cf_aabb_to_aabb_manifold cf_aabb_to_capsule_manifold cf_circle_to_poly_manifold cf_aabb_to_poly_manifold cf_capsule_to_poly_manifold cf_poly_to_poly_manifold
 */
CF_API CF_Manifold CF_CALL cf_circle_to_capsule_manifold(CF_Circle A, CF_Capsule B);

/**
 * @function cf_aabb_to_aabb_manifold
 * @category collision
 * @brief    Computes information about how two shapes intersect.
 * @param    A          The first shape.
 * @param    B          The second shape.
 * @return   Returns a `CF_Manifold` containing information about the intersection. See `CF_Manifold` for details.
 * @remarks  This function is slightly slower than the boolean version `cf_aabb_to_aabb`.
 * @related  CF_Manifold CF_Aabb cf_circle_to_circle_manifold cf_circle_to_aabb_manifold cf_circle_to_capsule_manifold cf_aabb_to_aabb_manifold cf_aabb_to_capsule_manifold cf_circle_to_poly_manifold cf_aabb_to_poly_manifold cf_capsule_to_poly_manifold cf_poly_to_poly_manifold
 */
CF_API CF_Manifold CF_CALL cf_aabb_to_aabb_manifold(CF_Aabb A, CF_Aabb B);

/**
 * @function cf_aabb_to_capsule_manifold
 * @category collision
 * @brief    Computes information about how two shapes intersect.
 * @param    A          The first shape.
 * @param    B          The second shape.
 * @return   Returns a `CF_Manifold` containing information about the intersection. See `CF_Manifold` for details.
 * @remarks  This function is slightly slower than the boolean version `cf_aabb_to_capsule`.
 * @related  CF_Manifold CF_Aabb CF_Capsule cf_circle_to_circle_manifold cf_circle_to_aabb_manifold cf_circle_to_capsule_manifold cf_aabb_to_aabb_manifold cf_aabb_to_capsule_manifold cf_circle_to_poly_manifold cf_aabb_to_poly_manifold cf_capsule_to_poly_manifold cf_poly_to_poly_manifold
 */
CF_API CF_Manifold CF_CALL cf_aabb_to_capsule_manifold(CF_Aabb A, CF_Capsule B);

/**
 * @function cf_capsule_to_capsule_manifold
 * @category collision
 * @brief    Computes information about how two shapes intersect.
 * @param    A          The first shape.
 * @param    B          The second shape.
 * @return   Returns a `CF_Manifold` containing information about the intersection. See `CF_Manifold` for details.
 * @remarks  This function is slightly slower than the boolean version `cf_capsule_to_capsule`.
 * @related  CF_Manifold CF_Capsule cf_circle_to_circle_manifold cf_circle_to_aabb_manifold cf_circle_to_capsule_manifold cf_aabb_to_aabb_manifold cf_aabb_to_capsule_manifold cf_circle_to_poly_manifold cf_aabb_to_poly_manifold cf_capsule_to_poly_manifold cf_poly_to_poly_manifold
 */
CF_API CF_Manifold CF_CALL cf_capsule_to_capsule_manifold(CF_Capsule A, CF_Capsule B);

/**
 * @function cf_circle_to_poly_manifold
 * @category collision
 * @brief    Computes information about how two shapes intersect.
 * @param    A          The first shape.
 * @param    B          The second shape.
 * @return   Returns a `CF_Manifold` containing information about the intersection. See `CF_Manifold` for details.
 * @remarks  This function is slightly slower than the boolean version `cf_circle_to_poly`.
 * @related  CF_Manifold CF_Circle CF_Poly cf_circle_to_circle_manifold cf_circle_to_aabb_manifold cf_circle_to_capsule_manifold cf_aabb_to_aabb_manifold cf_aabb_to_capsule_manifold cf_circle_to_poly_manifold cf_aabb_to_poly_manifold cf_capsule_to_poly_manifold cf_poly_to_poly_manifold
 */
CF_API CF_Manifold CF_CALL cf_circle_to_poly_manifold(CF_Circle A, const CF_Poly* B, const CF_Transform* bx);

/**
 * @function cf_aabb_to_poly_manifold
 * @category collision
 * @brief    Computes information about how two shapes intersect.
 * @param    A          The first shape.
 * @param    B          The second shape.
 * @return   Returns a `CF_Manifold` containing information about the intersection. See `CF_Manifold` for details.
 * @remarks  This function is slightly slower than the boolean version `cf_aabb_to_poly`.
 * @related  CF_Manifold CF_Aabb CF_Poly cf_circle_to_circle_manifold cf_circle_to_aabb_manifold cf_circle_to_capsule_manifold cf_aabb_to_aabb_manifold cf_aabb_to_capsule_manifold cf_circle_to_poly_manifold cf_aabb_to_poly_manifold cf_capsule_to_poly_manifold cf_poly_to_poly_manifold
 */
CF_API CF_Manifold CF_CALL cf_aabb_to_poly_manifold(CF_Aabb A, const CF_Poly* B, const CF_Transform* bx);

/**
 * @function cf_capsule_to_poly_manifold
 * @category collision
 * @brief    Computes information about how two shapes intersect.
 * @param    A          The first shape.
 * @param    B          The second shape.
 * @return   Returns a `CF_Manifold` containing information about the intersection. See `CF_Manifold` for details.
 * @remarks  This function is slightly slower than the boolean version `cf_capsule_to_poly`.
 * @related  CF_Manifold CF_Capsule CF_Poly cf_circle_to_circle_manifold cf_circle_to_aabb_manifold cf_circle_to_capsule_manifold cf_aabb_to_aabb_manifold cf_aabb_to_capsule_manifold cf_circle_to_poly_manifold cf_aabb_to_poly_manifold cf_capsule_to_poly_manifold cf_poly_to_poly_manifold
 */
CF_API CF_Manifold CF_CALL cf_capsule_to_poly_manifold(CF_Capsule A, const CF_Poly* B, const CF_Transform* bx);

/**
 * @function cf_poly_to_poly_manifold
 * @category collision
 * @brief    Computes information about how two shapes intersect.
 * @param    A          The first shape.
 * @param    B          The second shape.
 * @return   Returns a `CF_Manifold` containing information about the intersection. See `CF_Manifold` for details.
 * @remarks  This function is slightly slower than the boolean version `cf_poly_to_poly`.
 * @related  CF_Manifold CF_Poly cf_circle_to_circle_manifold cf_circle_to_aabb_manifold cf_circle_to_capsule_manifold cf_aabb_to_aabb_manifold cf_aabb_to_capsule_manifold cf_circle_to_poly_manifold cf_aabb_to_poly_manifold cf_capsule_to_poly_manifold cf_poly_to_poly_manifold
 */
CF_API CF_Manifold CF_CALL cf_poly_to_poly_manifold(const CF_Poly* A, const CF_Transform* ax, const CF_Poly* B, const CF_Transform* bx);

/**
 * @struct   CF_GjkCache
 * @category collision
 * @brief    This struct is only for advanced usage of the `cf_gjk` function. See comments inside of the `cf_gjk` function for more details.
 * @remarks  Contains cached geometric information to speed up successive calls to `cf_gjk` where the shapes don't move much relatie to
 *           one other from one call to the next.
 * @related  CF_GjkCache cf_gjk
 */
typedef struct CF_GjkCache
{
	/* A metric used to determine if the cache is good enough to be used again. */
	float metric;

	/* The number of vertices in the simplex cached. */
	int count;

	/* An index into shape A. */
	int iA[3];

	/* An index into shape B. */
	int iB[3];

	/* The divisor (used in baryecentric coordinates internally) for the vertex B - A. */
	float div;
} CF_GjkCache;
// @end

/**
 * @function cf_gjk
 * @category collision
 * @brief    Returns the distance between two shapes, and computes the closest two points of the shapes.
 * @param    A           The first shape.
 * @param    typeA       The `CF_ShapeType` of the first shape `A`.
 * @param    ax_ptr      Can be `NULL` to represent an identity transform. An optional pointer to a `CF_Transform` to transform `A`.
 * @param    B           The second shape.
 * @param    typeA       The `CF_ShapeType` of the second shape `B`.
 * @param    bx_ptr      Can be `NULL` to represent an identity transform. An optional pointer to a `CF_Transform` to transform `B`.
 * @param    outA        The closest point on `A` to `B`. Not well defined if the two shapes are already intersecting.
 * @param    outB        The closest point on `B` to `A`. Not well defined if the two shapes are already intersecting.
 * @param    use_radius  True if you want to use the radius of any `CF_Circle` or `CF_Capsule` inputs, false to treat them as a point/line segment respectively (a radius of zero).
 * @param    iterations  Can be `NULL`. The number of internal GJK iterations that occurred. For debugging.
 * @param    cache       Can be `NULL`. An optional cache to a previous call of this function. See `CF_GjkCache` for details.
 * @return   Returns the distance between the two shapes.
 * @remarks  This is an advanced function, intended to be used by people who know what they're doing.
 *           
 *           The GJK function is sensitive to large shapes, since it internally will compute signed area values. `cf_gjk` is called throughout
 *           this file in many ways, so try to make sure all of your collision shapes are not gigantic. For example, try to keep the volume of
 *           all your shapes less than 100.0f. If you need large shapes, you should use tiny collision geometry for all function here, and simply
 *           render the geometry larger on-screen by scaling it up.
 * @related  cf_gjk CF_GjkCache CF_ShapeType
 */
CF_API float CF_CALL cf_gjk(const void* A, CF_ShapeType typeA, const CF_Transform* ax_ptr, const void* B, CF_ShapeType typeB, const CF_Transform* bx_ptr, CF_V2* outA, CF_V2* outB, bool use_radius, int* iterations, CF_GjkCache* cache);

/**
 * @struct   CF_ToiResult
 * @category collision
 * @brief    Stores results of a time of impact calculation done by `cf_toi`.
 * @remarks  This is an advanced struct, intended to be used by people who know what they're doing.
 * @related  CF_ToiResult cf_toi
 */
typedef struct CF_ToiResult
{
	/* 1 if shapes were touching at the TOI, 0 if they never hit. */
	int hit;

	/* The time of impact between two shapes. */
	float toi;

	/* Surface normal from shape A to B at the time of impact. */
	CF_V2 n;

	/* Point of contact between shapes A and B at time of impact. */
	CF_V2 p;

	/*  Number of iterations the solver underwent. */
	int iterations;
} CF_ToiResult;
// @end

/**
 * @function cf_toi
 * @category collision
 * @brief    Computes the time of impact of two shapes.
 * @param    A           The first shape.
 * @param    typeA       The `CF_ShapeType` of the first shape `A`.
 * @param    ax_ptr      Can be `NULL` to represent an identity transform. An optional pointer to a `CF_Transform` to transform `A`.
 * @param    vA          The velocity of `A`.
 * @param    B           The second shape.
 * @param    typeA       The `CF_ShapeType` of the second shape `B`.
 * @param    bx_ptr      Can be `NULL` to represent an identity transform. An optional pointer to a `CF_Transform` to transform `B`.
 * @param    vB          The velocity of `B`.
 * @param    use_radius  True if you want to use the radius of any `CF_Circle` or `CF_Capsule` inputs, false to treat them as a point/line segment respectively (a radius of zero).
 * @return   Returns a `CF_ToiResult` containing information about the time of impact.
 * @remarks  This is an advanced function, intended to be used by people who know what they're doing.
 *           
 *           Computes the time of impact from shape A and shape B. The velocity of each shape is provided by `vA` and `vB` respectively. The shapes are
 *           _not_ allowed to rotate over time. The velocity is assumed to represent the change in motion from time 0 to time 1, and so the return value
 *           will be a number from 0 to 1. To move each shape to the colliding configuration, multiply `vA` and `vB` each by the return value.
 *           
 *           IMPORTANT NOTE
 *           
 *           The `cf_toi` function can be used to implement a "swept character controller", but it can be difficult to do so. Say we compute a time
 *           of impact with `cf_toi` and move the shapes to the time of impact, and adjust the velocity by zeroing out the velocity along the surface
 *           normal. If we then call `cf_toi` again, it will fail since the shapes will be considered to start in a colliding configuration. There are
 *           many styles of tricks to get around this problem, and all of them involve giving the next call to `cf_toi` some breathing room. It is
 *           recommended to use some variation of the following algorithm:
 *           
 *           1. Call `cf_toi`.
 *           2. Move the shapes to the TOI.
 *           3. Slightly inflate the size of one, or both, of the shapes so they will be intersecting.
 *              The purpose is to make the shapes numerically intersecting, but not visually intersecting.
 *              Another option is to call `cf_toi` with slightly deflated shapes.
 *              See the function `cf_inflate` for some more details.
 *           4. Compute the collision manifold between the inflated shapes (for example, use `cf_poly_to_poly_manifold`).
 *           5. Gently push the shapes apart. This will give the next call to `cf_toi` some breathing room.
 * @related  CF_ToiResult cf_toi CF_ShapeType
 */
CF_API CF_ToiResult CF_CALL cf_toi(const void* A, CF_ShapeType typeA, const CF_Transform* ax_ptr, CF_V2 vA, const void* B, CF_ShapeType typeB, const CF_Transform* bx_ptr, CF_V2 vB, int use_radius);

/**
 * @function cf_collided
 * @category collision
 * @brief    Returns a true if two shapes collided.
 * @param    A           The first shape.
 * @param    typeA       The `CF_ShapeType` of the first shape `A`.
 * @param    ax_ptr      Can be `NULL` to represent an identity transform. An optional pointer to a `CF_Transform` to transform `A`.
 * @param    vA          The velocity of `A`.
 * @param    B           The second shape.
 * @param    typeA       The `CF_ShapeType` of the second shape `B`.
 * @param    bx_ptr      Can be `NULL` to represent an identity transform. An optional pointer to a `CF_Transform` to transform `B`.
 * @related  cf_collided cf_collide CF_Transform CF_ShapeType
 */
CF_API int CF_CALL cf_collided(const void* A, const CF_Transform* ax, CF_ShapeType typeA, const void* B, const CF_Transform* bx, CF_ShapeType typeB);

/**
 * @function cf_collide
 * @category collision
 * @brief    Computes a `CF_Manifold` between two shapes.
 * @param    A           The first shape.
 * @param    ax          Can be `NULL` to represent an identity transform. An optional pointer to a `CF_Transform` to transform `A`.
 * @param    typeA       The `CF_ShapeType` of the first shape `A`.
 * @param    B           The second shape.
 * @param    bx          Can be `NULL` to represent an identity transform. An optional pointer to a `CF_Transform` to transform `B`.
 * @param    typeA       The `CF_ShapeType` of the second shape `B`.
 * @param    m           Contains information about the intersection. `m->count` is set to zero for no-intersection. See `CF_Manifold` for details.
 * @related  cf_collided cf_collide CF_Transform CF_ShapeType CF_Manifold
 */
CF_API void CF_CALL cf_collide(const void* A, const CF_Transform* ax, CF_ShapeType typeA, const void* B, const CF_Transform* bx, CF_ShapeType typeB, CF_Manifold* m);

/**
 * @function cf_cast_ray
 * @category collision
 * @brief    Casts a ray onto a shape.
 * @param    A           The ray.
 * @param    B           The shape.
 * @param    typeB       The `CF_ShapeType` of the shape `B`.
 * @param    bx_ptr      Can be `NULL` to represent an identity transform. An optional pointer to a `CF_Transform` to transform `B`.
 * @param    out         Can be `NULL`. `CF_Raycast` results are placed here (contains normal + time of impact).
 * @return   Returns true if the ray hit the shape.
 * @related  CF_Ray CF_Raycast CF_Transform CF_ShapeType cf_cast_ray
 */
CF_API bool CF_CALL cf_cast_ray(CF_Ray A, const void* B, const CF_Transform* bx, CF_ShapeType typeB, CF_Raycast* out);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using v2 = CF_V2;

using SinCos = CF_SinCos;
using M2x2 = CF_M2x2;
using M3x2 = CF_M3x2;
using Transform = CF_Transform;
using Halfspace = CF_Halfspace;
using Ray = CF_Ray;
using Raycast = CF_Raycast;
using Circle = CF_Circle;
using Aabb = CF_Aabb;
using Rect = CF_Rect;
using Poly = CF_Poly;
using SliceOutput = CF_SliceOutput;
using Capsule = CF_Capsule;
using Manifold = CF_Manifold;
using GjkCache = CF_GjkCache;
using ToiResult = CF_ToiResult;

using ShapeType = CF_ShapeType;
#define CF_ENUM(K, V) CF_INLINE constexpr ShapeType K = CF_##K;
CF_SHAPE_TYPE_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(ShapeType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_SHAPE_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CF_INLINE float min(float a, float b) { return cf_min(a, b); }
CF_INLINE float max(float a, float b) { return cf_max(a, b); }
CF_INLINE float clamp(float a, float lo, float hi) { return cf_clamp(a, lo, hi); }
CF_INLINE float clamp01(float a) { return cf_clamp01(a); }
CF_INLINE float sign(float a) { return cf_sign(a); }
CF_INLINE float intersect(float da, float db) { return cf_intersect(da, db); }
CF_INLINE float safe_invert(float a) { return cf_safe_invert(a); }
CF_INLINE float lerp(float a, float b, float t) { return cf_lerp(a, b, t); }
CF_INLINE float remap(float t, float lo, float hi) { return cf_remap01(t, lo, hi); }
CF_INLINE float remap(float t, float old_lo, float old_hi, float lo, float hi) { return cf_remap(t, old_lo, old_hi, lo, hi); }
CF_INLINE float mod(float x, float m) { return cf_mod(x, m); }
CF_INLINE float fract(float x) { return cf_fract(x); }

CF_INLINE int sign(int a) { return cf_sign_int(a); }
CF_INLINE int min(int a, int b) { return cf_min(a, b); }
CF_INLINE int max(int a, int b) { return cf_max(a, b); }
CF_INLINE uint64_t min(uint64_t a, uint64_t b) { return cf_min(a, b); }
CF_INLINE uint64_t max(uint64_t a, uint64_t b) { return cf_max(a, b); }
CF_INLINE float abs(float a) { return cf_abs(a); }
CF_INLINE int abs(int a) { return cf_abs_int(a); }
CF_INLINE int clamp(int a, int lo, int hi) { return cf_clamp_int(a, lo, hi); }
CF_INLINE int clamp01(int a) { return cf_clamp01_int(a); }
CF_INLINE bool is_even(int x) { return cf_is_even(x); }
CF_INLINE bool is_odd(int x) { return cf_is_odd(x); }

CF_INLINE bool is_power_of_two(int a) { return cf_is_power_of_two(a); }
CF_INLINE bool is_power_of_two(uint64_t a) { return cf_is_power_of_two_uint(a); }
CF_INLINE int fit_power_of_two(int a) { return cf_fit_power_of_two(a); }

CF_INLINE float smoothstep(float x) { return cf_smoothstep(x); }
CF_INLINE float quad_in(float x) { return cf_quad_in(x); }
CF_INLINE float quad_out(float x) { return cf_quad_out(x); }
CF_INLINE float quad_in_out(float x) { return cf_quad_in_out(x); }
CF_INLINE float cube_in(float x) { return cf_cube_in(x); }
CF_INLINE float cube_out(float x) { return cf_cube_out(x); }
CF_INLINE float cube_in_out(float x) { return cf_cube_in_out(x); }
CF_INLINE float quart_in(float x) { return cf_quart_in(x); }
CF_INLINE float quart_out(float x) { return cf_quart_out(x); }
CF_INLINE float quart_in_out(float x) { return cf_quart_in_out(x); }
CF_INLINE float quint_in(float x) { return cf_quint_in(x); }
CF_INLINE float quint_out(float x) { return cf_quint_out(x); }
CF_INLINE float quint_in_out(float x) { return cf_quint_in_out(x); }
CF_INLINE float sin_in(float x) { return cf_sin_in(x); }
CF_INLINE float sin_out(float x) { return cf_sin_out(x); }
CF_INLINE float sin_in_out(float x) { return cf_sin_in_out(x); }
CF_INLINE float circle_in(float x) { return cf_circle_in(x); }
CF_INLINE float circle_out(float x) { return cf_circle_out(x); }
CF_INLINE float circle_in_out(float x) { return cf_circle_in_out(x); }
CF_INLINE float back_in(float x) { return cf_back_in(x); }
CF_INLINE float back_out(float x) { return cf_back_out(x); }
CF_INLINE float back_in_out(float x) { return cf_back_in_out(x); }

CF_INLINE float dot(v2 a, v2 b) { return cf_dot(a, b); }

CF_INLINE v2 skew(v2 a) { return cf_skew(a); }
CF_INLINE v2 perp(v2 a) { return cf_skew(a); }
CF_INLINE v2 cw90(v2 a) { return cf_cw90(a); }
CF_INLINE float det2(v2 a, v2 b) { return cf_det2(a, b); }
CF_INLINE float cross(v2 a, v2 b) { return cf_cross(a, b); }
CF_INLINE v2 cross(v2 a, float b) { return cf_cross_v2_f(a, b); }
CF_INLINE v2 cross(float a, v2 b) { return cf_cross_f_v2(a, b); }
CF_INLINE v2 min(v2 a, v2 b) { return cf_min_v2(a, b); }
CF_INLINE v2 max(v2 a, v2 b) { return cf_max_v2(a, b); }
CF_INLINE v2 clamp(v2 a, v2 lo, v2 hi) { return cf_clamp_v2(a, lo, hi); }
CF_INLINE v2 clamp01(v2 a) { return cf_clamp01_v2(a); }
CF_INLINE v2 abs(v2 a) { return cf_abs_v2(a); }
CF_INLINE float hmin(v2 a) { return cf_hmin(a); }
CF_INLINE float hmax(v2 a) { return cf_hmax(a); }
CF_INLINE float len(v2 a) { return cf_len(a); }
CF_INLINE float len_sq(v2 a) { return cf_len_sq(a); }
CF_INLINE float distance(v2 a, v2 b) { return cf_distance(a, b); }
CF_INLINE v2 norm(v2 a) { return cf_norm(a); }
CF_INLINE v2 safe_norm(v2 a) { return cf_safe_norm(a); }
CF_INLINE float safe_norm(float a) { return cf_safe_norm_f(a); }
CF_INLINE int safe_norm(int a) { return cf_safe_norm_int(a); }
CF_INLINE v2 reflect(v2 a, v2 n) { return cf_reflect_v2(a, n); }

CF_INLINE v2 lerp(v2 a, v2 b, float t) { return cf_lerp_v2(a, b, t); }
CF_INLINE v2 bezier(v2 a, v2 c0, v2 b, float t) { return cf_bezier(a, c0, b, t); }
CF_INLINE v2 bezier(v2 a, v2 c0, v2 c1, v2 b, float t) { return cf_bezier2(a, c0, c1, b, t); }
CF_INLINE v2 floor(v2 a) { return cf_floor(a); }
CF_INLINE v2 round(v2 a) { return cf_round(a); }
CF_INLINE v2 safe_invert(v2 a) { return cf_safe_invert_v2(a); }
CF_INLINE v2 sign(v2 a) { return cf_sign_v2(a); }

CF_INLINE SinCos sincos(float radians) { return cf_sincos_f(radians); }
CF_INLINE SinCos sincos() { return cf_sincos(); }
CF_INLINE v2 x_axis(SinCos r) { return cf_x_axis(r); }
CF_INLINE v2 y_axis(SinCos r) { return cf_y_axis(r); }
CF_INLINE v2 mul(SinCos a, v2 b) { return cf_mul_sc_v2(a, b); }
CF_INLINE v2 mulT(SinCos a, v2 b) { return cf_mulT_sc_v2(a, b); }
CF_INLINE SinCos mul(SinCos a, SinCos b) { return cf_mul_sc(a, b); }
CF_INLINE SinCos mulT(SinCos a, SinCos b) { return cf_mulT_sc(a, b); }

CF_INLINE float atan2_360(float y, float x) { return cf_atan2_360(y, x); }
CF_INLINE float atan2_360(v2 v) { return cf_atan2_360_v2(v); }
CF_INLINE float atan2_360(SinCos r) { return cf_atan2_360_sc(r); }

CF_INLINE float shortest_arc(v2 a, v2 b) { return cf_shortest_arc(a, b); }

CF_INLINE float angle_diff(float radians_a, float radians_b) { return cf_angle_diff(radians_a, radians_b); }
CF_INLINE v2 from_angle(float radians) { return cf_from_angle(radians); }

CF_INLINE v2 mul(M2x2 a, v2 b) { return cf_mul_m2_v2(a, b); }
CF_INLINE M2x2 mul(M2x2 a, M2x2 b) { return cf_mul_m2(a, b); }

CF_INLINE v2 mul(M3x2 a, v2 b) { return cf_mul_m32_v2(a, b); }
CF_INLINE M3x2 mul(M3x2 a, M3x2 b) { return cf_mul_m32(a, b); }
CF_INLINE M3x2 make_identity() { return cf_make_identity(); }
CF_INLINE M3x2 make_translation(float x, float y) { return cf_make_translation_f(x, y); }
CF_INLINE M3x2 make_translation(v2 p) { return cf_make_translation(p); }
CF_INLINE M3x2 make_scale(v2 s) { return cf_make_scale(s); }
CF_INLINE M3x2 make_scale(float s) { return cf_make_scale_f(s); }
CF_INLINE M3x2 make_scale(float sx, float sy) { return cf_make_scale(cf_v2(sx, sy)); }
CF_INLINE M3x2 make_scale(v2 s, v2 p) { return cf_make_scale_translation(s, p); }
CF_INLINE M3x2 make_scale(float s, v2 p) { return cf_make_scale_translation_f(s, p); }
CF_INLINE M3x2 make_scale_translation(float sx, float sy, v2 p) { return cf_make_scale_translation_f_f(sx, sy, p); }
CF_INLINE M3x2 make_rotation(float radians) { return cf_make_rotation(radians); }
CF_INLINE M3x2 make_transform(v2 p, v2 s, float radians) { return cf_make_transform_TSR(p, s, radians); }
CF_INLINE M3x2 invert(M3x2 m) { return cf_invert(m); }
CF_INLINE M3x2 ortho_2d(float x, float y, float scale_x, float scale_y) { return cf_ortho_2d(x, y, scale_x, scale_y); }

CF_INLINE Transform make_transform() { return cf_make_transform(); }
CF_INLINE Transform make_transform(v2 p, float radians) { return cf_make_transform_TR(p, radians); }
CF_INLINE v2 mul(Transform a, v2 b) { return cf_mul_tf_v2(a, b); }
CF_INLINE v2 mulT(Transform a, v2 b) { return cf_mulT_tf_v2(a, b); }
CF_INLINE Transform mul(Transform a, Transform b) { return cf_mul_tf(a, b); }
CF_INLINE Transform mulT(Transform a, Transform b) { return cf_mulT_tf(a, b); }

CF_INLINE Halfspace plane(v2 n, float d) { return cf_plane(n, d); }
CF_INLINE Halfspace plane(v2 n, v2 p) { return cf_plane2(n, p); }
CF_INLINE v2 origin(Halfspace h) { return cf_origin(h); }
CF_INLINE float distance(Halfspace h, v2 p) { return cf_distance_hs(h, p); }
CF_INLINE v2 project(Halfspace h, v2 p) { return cf_project(h, p); }
CF_INLINE Halfspace mul(Transform a, Halfspace b) { return cf_mul_tf_hs(a, b); }
CF_INLINE Halfspace mulT(Transform a, Halfspace b) { return cf_mulT_tf_hs(a, b); }
CF_INLINE v2 intersect(v2 a, v2 b, float da, float db) { return cf_intersect_halfspace(a, b, da, db); }
CF_INLINE v2 intersect(Halfspace h, v2 a, v2 b) { return cf_intersect_halfspace2(h, a, b); }
CF_INLINE v2 intersect(Halfspace a, Halfspace b) { return cf_intersect_halfspace3(a, b); }
CF_INLINE Halfspace shift(Halfspace h, float d) { return cf_shift(h, d); }
CF_INLINE bool parallel(v2 a, v2 b, float tol) { return cf_parallel(a, b, tol); }
CF_INLINE bool parallel(v2 a, v2 b, v2 c, float tol) { return cf_parallel2(a, b, c, tol); }

CF_INLINE Circle make_circle(v2 pos, float radius) { return cf_make_circle(pos, radius); }
CF_INLINE Capsule make_capsule(v2 a, v2 b, float radius) { return cf_make_capsule(a, b, radius); }
CF_INLINE Capsule make_capsule(v2 p, float height, float radius) { return cf_make_capsule2(p, height, radius); }
CF_INLINE Aabb make_aabb(v2 min, v2 max) { return cf_make_aabb(min, max); }
CF_INLINE Aabb make_aabb(v2 pos, float w, float h) { return cf_make_aabb_pos_w_h(pos, w, h); }
CF_INLINE Aabb make_aabb_center_half_extents(v2 center, v2 half_extents) { return cf_make_aabb_center_half_extents(center, half_extents); }
CF_INLINE Aabb make_aabb_from_top_left(v2 top_left, float w, float h) { return cf_make_aabb_from_top_left(top_left, w, h); }
CF_INLINE float width(Aabb bb) { return cf_width(bb); }
CF_INLINE float height(Aabb bb) { return cf_height(bb); }
CF_INLINE float half_width(Aabb bb) { return cf_half_width(bb); }
CF_INLINE float half_height(Aabb bb) { return cf_half_height(bb); }
CF_INLINE v2 half_extents(Aabb bb) { return cf_half_extents(bb); }
CF_INLINE v2 extents(Aabb aabb) { return cf_extents(aabb); }
CF_INLINE Aabb expand(Aabb aabb, v2 v) { return cf_expand_aabb(aabb, v); }
CF_INLINE Aabb expand(Aabb aabb, float v) { return cf_expand_aabb_f(aabb, v); }
CF_INLINE v2 min(Aabb bb) { return cf_min_aabb(bb); }
CF_INLINE v2 max(Aabb bb) { return cf_max_aabb(bb); }
CF_INLINE v2 midpoint(Aabb bb) { return cf_midpoint(bb); }
CF_INLINE v2 center(Aabb bb) { return cf_center(bb); }
CF_INLINE v2 top_left(Aabb bb) { return cf_top_left(bb); }
CF_INLINE v2 top_right(Aabb bb) { return cf_top_right(bb); }
CF_INLINE v2 bottom_left(Aabb bb) { return cf_bottom_left(bb); }
CF_INLINE v2 bottom_right(Aabb bb) { return cf_bottom_right(bb); }
CF_INLINE v2 top(Aabb bb) { return cf_top(bb); }
CF_INLINE v2 left(Aabb bb) { return cf_left(bb); }
CF_INLINE v2 bottom(Aabb bb) { return cf_bottom(bb); }
CF_INLINE v2 right(Aabb bb) { return cf_right(bb); }
CF_INLINE bool contains(Aabb bb, v2 p) { return cf_contains_point(bb, p); }
CF_INLINE bool contains(Aabb a, Aabb b) { return cf_contains_aabb(a, b); }
CF_INLINE float surface_area(Aabb bb) { return cf_surface_area_aabb(bb); }
CF_INLINE float area(Aabb bb) { return cf_area_aabb(bb); }
CF_INLINE v2 clamp(Aabb bb, v2 p) { return cf_clamp_aabb_v2(bb, p); }
CF_INLINE Aabb clamp(Aabb a, Aabb b) { return cf_clamp_aabb(a, b); }
CF_INLINE Aabb combine(Aabb a, Aabb b) { return cf_combine(a, b); }

CF_INLINE int overlaps(Aabb a, Aabb b) { return cf_overlaps(a, b); }
CF_INLINE int collide(Aabb a, Aabb b) { return cf_collide_aabb(a, b); }

CF_INLINE Aabb make_aabb(v2* verts, int count) { return cf_make_aabb_verts((CF_V2*)verts, count); }
CF_INLINE void aabb_verts(v2* out, Aabb bb) { return cf_aabb_verts((CF_V2*)out, bb); }

CF_INLINE float area(Circle c) { return cf_area_circle(c); }
CF_INLINE float surface_area(Circle c) { return cf_surface_area_circle(c); }
CF_INLINE Circle mul(Transform tx, Circle a) { return cf_mul_tf_circle(tx, a); }

CF_INLINE Ray make_ray(v2 start, v2 direction_normalized, float length) { return cf_make_ray(start, direction_normalized, length); }
CF_INLINE v2 impact(Ray r, float t) { return cf_impact(r, t); }
CF_INLINE v2 endpoint(Ray r) { return cf_endpoint(r); }

CF_INLINE Raycast ray_to_halfspace(Ray A, Halfspace B) { return cf_ray_to_halfspace(A, B); }
CF_INLINE float distance_sq(v2 a, v2 b, v2 p) { return cf_distance_sq(a, b, p); }

CF_INLINE v2 center_of_mass(Poly poly) { return cf_center_of_mass(poly); }
CF_INLINE float calc_area(Poly poly) { return cf_calc_area(poly); }
CF_INLINE SliceOutput slice(Halfspace slice_plane, Poly slice_me, const float k_epsilon = 1.e-4f) { return cf_slice(slice_plane, slice_me, k_epsilon); }
CF_INLINE void inflate(void* shape, ShapeType type, float skin_factor) { return cf_inflate(shape, type, skin_factor); }
CF_INLINE int hull(v2* verts, int count) { return cf_hull((CF_V2*)verts, count); }
CF_INLINE void norms(v2* verts, v2* norms, int count) { return cf_norms((CF_V2*)verts, (CF_V2*)norms, count); }
CF_INLINE void make_poly(Poly* p) { return cf_make_poly(p); }
CF_INLINE v2 centroid(const v2* verts, int count) { return cf_centroid((CF_V2*)verts, count); }

CF_INLINE bool circle_to_circle(Circle A, Circle B) { return cf_circle_to_circle(A, B); }
CF_INLINE bool circle_to_aabb(Circle A, Aabb B) { return cf_circle_to_aabb(A, B); }
CF_INLINE bool circle_to_capsule(Circle A, Capsule B) { return cf_circle_to_capsule(A, B); }
CF_INLINE bool aabb_to_aabb(Aabb A, Aabb B) { return cf_aabb_to_aabb(A, B); }
CF_INLINE bool aabb_to_capsule(Aabb A, Capsule B) { return cf_aabb_to_capsule(A, B); }
CF_INLINE bool capsule_to_capsule(Capsule A, Capsule B) { return cf_capsule_to_capsule(A, B); }
CF_INLINE bool circle_to_poly(Circle A, const Poly* B, const Transform* bx) { return cf_circle_to_poly(A, B, bx); }
CF_INLINE bool aabb_to_poly(Aabb A, const Poly* B, const Transform* bx) { return cf_aabb_to_poly(A, B, bx); }
CF_INLINE bool capsule_to_poly(Capsule A, const Poly* B, const Transform* bx) { return cf_capsule_to_poly(A, B, bx); }
CF_INLINE bool poly_to_poly(const Poly* A, const Transform* ax, const Poly* B, const Transform* bx) { return cf_poly_to_poly(A, ax, B, bx); }

CF_INLINE Raycast ray_to_circle(Ray A, Circle B) { return cf_ray_to_circle(A, B); }
CF_INLINE Raycast ray_to_aabb(Ray A, Aabb B) { return cf_ray_to_aabb(A, B); }
CF_INLINE Raycast ray_to_capsule(Ray A, Capsule B) { return cf_ray_to_capsule(A, B); }
CF_INLINE Raycast ray_to_poly(Ray A, const Poly* B, const Transform* bx_ptr = NULL) { return cf_ray_to_poly(A, B, bx_ptr); }

CF_INLINE Manifold circle_to_circle_manifold(Circle A, Circle B) { return cf_circle_to_circle_manifold(A, B); }
CF_INLINE Manifold circle_to_aabb_manifold(Circle A, Aabb B) { return cf_circle_to_aabb_manifold(A, B); }
CF_INLINE Manifold circle_to_capsule_manifold(Circle A, Capsule B) { return cf_circle_to_capsule_manifold(A, B); }
CF_INLINE Manifold aabb_to_aabb_manifold(Aabb A, Aabb B) { return cf_aabb_to_aabb_manifold(A, B); }
CF_INLINE Manifold aabb_to_capsule_manifold(Aabb A, Capsule B) { return cf_aabb_to_capsule_manifold(A, B); }
CF_INLINE Manifold capsule_to_capsule_manifold(Capsule A, Capsule B) { return cf_capsule_to_capsule_manifold(A, B); }
CF_INLINE Manifold circle_to_poly_manifold(Circle A, const Poly* B, const Transform* bx) { return cf_circle_to_poly_manifold(A, B, bx); }
CF_INLINE Manifold aabb_to_poly_manifold(Aabb A, const Poly* B, const Transform* bx) { return cf_aabb_to_poly_manifold(A, B, bx); }
CF_INLINE Manifold capsule_to_poly_manifold(Capsule A, const Poly* B, const Transform* bx) { return cf_capsule_to_poly_manifold(A, B, bx); }
CF_INLINE Manifold poly_to_poly_manifold(const Poly* A, const Transform* ax, const Poly* B, const Transform* bx) { return cf_poly_to_poly_manifold(A, ax, B, bx); }

CF_INLINE float gjk(const void* A, ShapeType typeA, const Transform* ax_ptr, const void* B, ShapeType typeB, const Transform* bx_ptr, v2* outA, v2* outB, int use_radius, int* iterations, GjkCache* cache)
{
	return cf_gjk(A, typeA, ax_ptr, B, typeB, bx_ptr, (CF_V2*)outA, (CF_V2*)outB, use_radius, iterations, cache);
}

CF_INLINE ToiResult toi(const void* A, ShapeType typeA, const Transform* ax_ptr, v2 vA, const void* B, ShapeType typeB, const Transform* bx_ptr, v2 vB, int use_radius, int* iterations)
{
	return cf_toi(A, typeA, ax_ptr, vA, B, typeB, bx_ptr, vB, use_radius);
}

CF_INLINE int collided(const void* A, const Transform* ax, ShapeType typeA, const void* B, const Transform* bx, ShapeType typeB) { return cf_collided(A, ax, typeA, B, bx, typeB); }
CF_INLINE void collide(const void* A, const Transform* ax, ShapeType typeA, const void* B, const Transform* bx, ShapeType typeB, Manifold* m) { return cf_collide(A, ax, typeA, B, bx, typeB, m); }
CF_INLINE bool cast_ray(Ray A, const void* B, const Transform* bx, ShapeType typeB, Raycast* out) { return cf_cast_ray(A, B, bx, typeB, out); }

}

CF_INLINE Cute::v2 V2(float x, float y) { Cute::v2 result; result.x = x; result.y = y; return result; }

CF_INLINE Cute::v2 operator+(Cute::v2 a, Cute::v2 b) { return V2(a.x + b.x, a.y + b.y); }
CF_INLINE Cute::v2 operator-(Cute::v2 a, Cute::v2 b) { return V2(a.x - b.x, a.y - b.y); }
CF_INLINE Cute::v2& operator+=(Cute::v2& a, Cute::v2 b) { return a = a + b; }
CF_INLINE Cute::v2& operator-=(Cute::v2& a, Cute::v2 b) { return a = a - b; }

CF_INLINE Cute::v2 operator*(Cute::v2 a, float b) { return V2(a.x * b, a.y * b); }
CF_INLINE Cute::v2 operator*(Cute::v2 a, Cute::v2 b) { return V2(a.x * b.x, a.y * b.y); }
CF_INLINE Cute::v2& operator*=(Cute::v2& a, float b) { return a = a * b; }
CF_INLINE Cute::v2& operator*=(Cute::v2& a, Cute::v2 b) { return a = a * b; }
CF_INLINE Cute::v2 operator/(Cute::v2 a, float b) { return V2(a.x / b, a.y / b); }
CF_INLINE Cute::v2 operator/(Cute::v2 a, Cute::v2 b) { return V2(a.x / b.x, a.y / b.y); }
CF_INLINE Cute::v2& operator/=(Cute::v2& a, float b) { return a = a / b; }
CF_INLINE Cute::v2& operator/=(Cute::v2& a, Cute::v2 b) { return a = a / b; }

CF_INLINE Cute::v2 operator-(Cute::v2 a) { return V2(-a.x, -a.y); }

CF_INLINE int operator<(Cute::v2 a, Cute::v2 b) { return a.x < b.x&& a.y < b.y; }
CF_INLINE int operator>(Cute::v2 a, Cute::v2 b) { return a.x > b.x && a.y > b.y; }
CF_INLINE int operator<=(Cute::v2 a, Cute::v2 b) { return a.x <= b.x && a.y <= b.y; }
CF_INLINE int operator>=(Cute::v2 a, Cute::v2 b) { return a.x >= b.x && a.y >= b.y; }

#endif // CF_CPP

#endif // CF_MATH_H
