/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_MATH_H
#define CF_MATH_H

#include "cute_defines.h"
#include "cute_array.h"

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

#ifndef CF_ASINF
	#include <math.h>
	#define CF_ASINF asinf
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

#ifndef CF_LOG2F
	#include <math.h>
	#define CF_LOG2F log2f
#endif

#ifndef CF_SQRT
	#include <math.h>
	#define CF_SQRT sqrt
#endif

#ifndef CF_FABS
	#include <math.h>
	#define CF_FABS fabs
#endif

#ifndef CF_SIN
	#include <math.h>
	#define CF_SIN sin
#endif

#ifndef CF_COS
	#include <math.h>
	#define CF_COS cos
#endif

#ifndef CF_ACOS
	#include <math.h>
	#define CF_ACOS acos
#endif

#ifndef CF_ATAN2
	#include <math.h>
	#define CF_ATAN2 atan2
#endif

#ifndef CF_FLOOR
	#include <math.h>
	#define CF_FLOOR floor
#endif

#ifndef CF_CEIL
	#include <math.h>
	#define CF_CEIL ceil
#endif

#ifndef CF_ROUND
	#include <math.h>
	#define CF_ROUND round
#endif

#ifndef CF_FMOD
	#include <math.h>
	#define CF_FMOD fmod
#endif

#ifndef CF_LOG2
	#include <math.h>
	#define CF_LOG2 log2
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
 * @related  CF_V2 cf_add cf_sub cf_dot cf_mul_v2_f cf_div_v2_f
 */
typedef struct CF_V2
{
	/* @member The x component. */
	float x;

	/* @member The y component. */
	float y;
} CF_V2;
// @end

#define cf_v2(...)
#undef cf_v2
// We implement cf_v2 in this odd way to 100% for sure force-inline a static initializer. This ensures
// boosted debug performance by inlining the constructor/initializer code no matter what. These expand to
// { x, y } in C++ and (CF_V2){ .x = a, .y = b } in C, and also support "splatting": cf_v2(a) == { a, a }.
#define CF_EXPAND(x) x
#define _CF_V2_SELECT(_1, _2, NAME, ...) NAME
#ifdef __cplusplus
#	define _CF_V2_1(a) CF_V2{ (a), (a) }
#	define _CF_V2_2(a, b) CF_V2{ (a), (b) }
#	define cf_v2(...) CF_EXPAND(_CF_V2_SELECT(__VA_ARGS__, _CF_V2_2, _CF_V2_1)(__VA_ARGS__))
#	define V2 cf_v2
#else
#	define _CF_V2_1(a) (CF_V2){ .x = (a), .y = (a) }
#	define _CF_V2_2(a, b) (CF_V2){ .x = (a), .y = (b) }
#	define cf_v2(...) CF_EXPAND(_CF_V2_SELECT(__VA_ARGS__, _CF_V2_2, _CF_V2_1)(__VA_ARGS__))
#endif

/**
 * @struct   CF_SinCos
 * @category math
 * @brief    Rotation about an axis composed of cos/sin pair.
 * @remarks  You can construct a `CF_SinCos` with the function `cf_sin_cos()/cf_sin_cos(1.0f)` function.
 * @related  CF_SinCos cf_sincos cf_sincos_f cf_x_axis cf_y_axis cf_mul_sc_v2 cf_mul_T_sc_v2
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
 * @related  CF_Transform cf_make_transform cf_make_transform_TR cf_mul_tf_v2 cf_mul_T_tf_v2 cf_mul_tf cf_mul_T_tf
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
 * @brief    2D polygon, used for collision detection functions.
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

/**
 * @function cf_min
 * @category math
 * @brief    Returns the minimum of two values.
 * @related  cf_min cf_max
 */
#define cf_min(a, b) ((a) < (b) ? (a) : (b))
#undef cf_min
#ifdef __cplusplus
} // extern "C"
CF_INLINE float    cf_min(float    a, float    b) { return a < b ? a : b; }
CF_INLINE double   cf_min(double   a, double   b) { return a < b ? a : b; }
CF_INLINE int8_t   cf_min(int8_t   a, int8_t   b) { return a < b ? a : b; }
CF_INLINE uint8_t  cf_min(uint8_t  a, uint8_t  b) { return a < b ? a : b; }
CF_INLINE int16_t  cf_min(int16_t  a, int16_t  b) { return a < b ? a : b; }
CF_INLINE uint16_t cf_min(uint16_t a, uint16_t b) { return a < b ? a : b; }
CF_INLINE int32_t  cf_min(int32_t  a, int32_t  b) { return a < b ? a : b; }
CF_INLINE uint32_t cf_min(uint32_t a, uint32_t b) { return a < b ? a : b; }
CF_INLINE int64_t  cf_min(int64_t  a, int64_t  b) { return a < b ? a : b; }
CF_INLINE uint64_t cf_min(uint64_t a, uint64_t b) { return a < b ? a : b; }
CF_INLINE CF_V2    cf_min(CF_V2    a, CF_V2    b) { return CF_V2 { cf_min(a.x, b.x), cf_min(a.y, b.y) }; }
extern "C" {
#else
CF_INLINE float    cf_min_f  (float    a, float    b) { return a < b ? a : b; }
CF_INLINE double   cf_min_d  (double   a, double   b) { return a < b ? a : b; }
CF_INLINE int8_t   cf_min_i8 (int8_t   a, int8_t   b) { return a < b ? a : b; }
CF_INLINE uint8_t  cf_min_u8 (uint8_t  a, uint8_t  b) { return a < b ? a : b; }
CF_INLINE int16_t  cf_min_i16(int16_t  a, int16_t  b) { return a < b ? a : b; }
CF_INLINE uint16_t cf_min_u16(uint16_t a, uint16_t b) { return a < b ? a : b; }
CF_INLINE int32_t  cf_min_i32(int32_t  a, int32_t  b) { return a < b ? a : b; }
CF_INLINE uint32_t cf_min_u32(uint32_t a, uint32_t b) { return a < b ? a : b; }
CF_INLINE int64_t  cf_min_i64(int64_t  a, int64_t  b) { return a < b ? a : b; }
CF_INLINE uint64_t cf_min_u64(uint64_t a, uint64_t b) { return a < b ? a : b; }
CF_INLINE CF_V2    cf_min_v2 (CF_V2    a, CF_V2    b) { return (CF_V2){ cf_min_f(a.x, b.x), cf_min_f(a.y, b.y) }; }
#define cf_min(a, b)          \
	_Generic((a),             \
		float:    cf_min_f,   \
		double:   cf_min_d,   \
		int8_t:   cf_min_i8,  \
		uint8_t:  cf_min_u8,  \
		int16_t:  cf_min_i16, \
		uint16_t: cf_min_u16, \
		int32_t:  cf_min_i32, \
		uint32_t: cf_min_u32, \
		int64_t:  cf_min_i64, \
		uint64_t: cf_min_u64, \
		CF_V2:    cf_min_v2,  \
		default:  cf_min_i32  \
	)((a), (b))
#endif

/**
 * @function cf_max
 * @category math
 * @brief    Returns the maximum of two values.
 * @related  cf_min cf_max
 */
#define cf_max(a, b) ((a) < (b) ? (a) : (b))
#undef cf_max
#ifdef __cplusplus
} // extern "C"
CF_INLINE float    cf_max(float    a, float    b) { return a > b ? a : b; }
CF_INLINE double   cf_max(double   a, double   b) { return a > b ? a : b; }
CF_INLINE int8_t   cf_max(int8_t   a, int8_t   b) { return a > b ? a : b; }
CF_INLINE uint8_t  cf_max(uint8_t  a, uint8_t  b) { return a > b ? a : b; }
CF_INLINE int16_t  cf_max(int16_t  a, int16_t  b) { return a > b ? a : b; }
CF_INLINE uint16_t cf_max(uint16_t a, uint16_t b) { return a > b ? a : b; }
CF_INLINE int32_t  cf_max(int32_t  a, int32_t  b) { return a > b ? a : b; }
CF_INLINE uint32_t cf_max(uint32_t a, uint32_t b) { return a > b ? a : b; }
CF_INLINE int64_t  cf_max(int64_t  a, int64_t  b) { return a > b ? a : b; }
CF_INLINE uint64_t cf_max(uint64_t a, uint64_t b) { return a > b ? a : b; }
CF_INLINE CF_V2    cf_max(CF_V2    a, CF_V2    b) { return cf_v2(cf_max(a.x, b.x), cf_max(a.y, b.y)); }
extern "C" {
#else
CF_INLINE float    cf_max_f  (float    a, float    b) { return a > b ? a : b; }
CF_INLINE double   cf_max_d  (double   a, double   b) { return a > b ? a : b; }
CF_INLINE int8_t   cf_max_i8 (int8_t   a, int8_t   b) { return a > b ? a : b; }
CF_INLINE uint8_t  cf_max_u8 (uint8_t  a, uint8_t  b) { return a > b ? a : b; }
CF_INLINE int16_t  cf_max_i16(int16_t  a, int16_t  b) { return a > b ? a : b; }
CF_INLINE uint16_t cf_max_u16(uint16_t a, uint16_t b) { return a > b ? a : b; }
CF_INLINE int32_t  cf_max_i32(int32_t  a, int32_t  b) { return a > b ? a : b; }
CF_INLINE uint32_t cf_max_u32(uint32_t a, uint32_t b) { return a > b ? a : b; }
CF_INLINE int64_t  cf_max_i64(int64_t  a, int64_t  b) { return a > b ? a : b; }
CF_INLINE uint64_t cf_max_u64(uint64_t a, uint64_t b) { return a > b ? a : b; }
CF_INLINE CF_V2    cf_max_v2 (CF_V2    a, CF_V2    b) { return cf_v2(cf_max_f(a.x, b.x), cf_max_f(a.y, b.y)); }
#define cf_max(a, b)          \
	_Generic((a),             \
		float:    cf_max_f,   \
		double:   cf_max_d,   \
		int8_t:   cf_max_i8,  \
		uint8_t:  cf_max_u8,  \
		int16_t:  cf_max_i16, \
		uint16_t: cf_max_u16, \
		int32_t:  cf_max_i32, \
		uint32_t: cf_max_u32, \
		int64_t:  cf_max_i64, \
		uint64_t: cf_max_u64, \
		CF_V2:    cf_max_v2,  \
		default:  cf_max_i32  \
	)((a), (b))
#endif

/**
 * @function cf_abs
 * @category math
 * @brief    Returns absolute value of a float.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
#define cf_abs(a)
#undef cf_abs
#ifdef __cplusplus
} // extern "C"
CF_INLINE float    cf_abs(float    x) { return x < 0 ? -x : x; }
CF_INLINE double   cf_abs(double   x) { return x < 0 ? -x : x; }
CF_INLINE int8_t   cf_abs(int8_t   x) { return x < 0 ? (int8_t)-x : x; }
CF_INLINE int16_t  cf_abs(int16_t  x) { return x < 0 ? (int16_t)-x : x; }
CF_INLINE int32_t  cf_abs(int32_t  x) { return x < 0 ? -x : x; }
CF_INLINE int64_t  cf_abs(int64_t  x) { return x < 0 ? -x : x; }
CF_INLINE uint8_t  cf_abs(uint8_t  x) { return x; }
CF_INLINE uint16_t cf_abs(uint16_t x) { return x; }
CF_INLINE uint32_t cf_abs(uint32_t x) { return x; }
CF_INLINE uint64_t cf_abs(uint64_t x) { return x; }
CF_INLINE CF_V2    cf_abs(CF_V2    x) { return cf_v2(cf_abs(x.x), cf_abs(x.y)); }
extern "C" {
#else
CF_INLINE float    cf_abs_f  (float    x) { return x < 0 ? -x : x; }
CF_INLINE double   cf_abs_d  (double   x) { return x < 0 ? -x : x; }
CF_INLINE int8_t   cf_abs_i8 (int8_t   x) { return x < 0 ? (int8_t)-x : x; }
CF_INLINE int16_t  cf_abs_i16(int16_t  x) { return x < 0 ? (int16_t)-x : x; }
CF_INLINE int32_t  cf_abs_i32(int32_t  x) { return x < 0 ? -x : x; }
CF_INLINE int64_t  cf_abs_i64(int64_t  x) { return x < 0 ? -x : x; }
CF_INLINE uint8_t  cf_abs_u8 (uint8_t  x) { return x; }
CF_INLINE uint16_t cf_abs_u16(uint16_t x) { return x; }
CF_INLINE uint32_t cf_abs_u32(uint32_t x) { return x; }
CF_INLINE uint64_t cf_abs_u64(uint64_t x) { return x; }
CF_INLINE CF_V2    cf_abs_v2 (CF_V2    x) { return cf_v2(cf_abs_f(x.x), cf_abs_f(x.y)); }
#define cf_abs(x)             \
	_Generic((x),             \
		float:    cf_abs_f,   \
		double:   cf_abs_d,   \
		int8_t:   cf_abs_i8,  \
		uint8_t:  cf_abs_u8,  \
		int16_t:  cf_abs_i16, \
		uint16_t: cf_abs_u16, \
		int32_t:  cf_abs_i32, \
		uint32_t: cf_abs_u32, \
		int64_t:  cf_abs_i64, \
		uint64_t: cf_abs_u64, \
		CF_V2:    cf_abs_v2,  \
		default:  cf_abs_i32  \
	)(x)
#endif

/**
 * @function cf_clamp
 * @category math
 * @brief    Returns `a` float clamped between `lo` and `hi`.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
#define cf_clamp(x, lo, hi) cf_max(lo, cf_min(x, hi))
#undef cf_clamp
#ifdef __cplusplus
} // extern "C"
CF_INLINE float    cf_clamp(float    x, float    lo, float    hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE double   cf_clamp(double   x, double   lo, double   hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE int8_t   cf_clamp(int8_t   x, int8_t   lo, int8_t   hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE uint8_t  cf_clamp(uint8_t  x, uint8_t  lo, uint8_t  hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE int16_t  cf_clamp(int16_t  x, int16_t  lo, int16_t  hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE uint16_t cf_clamp(uint16_t x, uint16_t lo, uint16_t hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE int32_t  cf_clamp(int32_t  x, int32_t  lo, int32_t  hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE uint32_t cf_clamp(uint32_t x, uint32_t lo, uint32_t hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE int64_t  cf_clamp(int64_t  x, int64_t  lo, int64_t  hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE uint64_t cf_clamp(uint64_t x, uint64_t lo, uint64_t hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE CF_V2    cf_clamp(CF_V2    x, CF_V2    lo, CF_V2    hi) { return cf_max(lo, cf_min(x, hi)); }
extern "C" {
#else
CF_INLINE float    cf_clamp_f  (float    x, float    lo, float    hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE double   cf_clamp_d  (double   x, double   lo, double   hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE int8_t   cf_clamp_i8 (int8_t   x, int8_t   lo, int8_t   hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE uint8_t  cf_clamp_u8 (uint8_t  x, uint8_t  lo, uint8_t  hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE int16_t  cf_clamp_i16(int16_t  x, int16_t  lo, int16_t  hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE uint16_t cf_clamp_u16(uint16_t x, uint16_t lo, uint16_t hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE int32_t  cf_clamp_i32(int32_t  x, int32_t  lo, int32_t  hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE uint32_t cf_clamp_u32(uint32_t x, uint32_t lo, uint32_t hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE int64_t  cf_clamp_i64(int64_t  x, int64_t  lo, int64_t  hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE uint64_t cf_clamp_u64(uint64_t x, uint64_t lo, uint64_t hi) { return cf_max(lo, cf_min(x, hi)); }
CF_INLINE CF_V2    cf_clamp_v2 (CF_V2    x, CF_V2    lo, CF_V2    hi) { return cf_max(lo, cf_min(x, hi)); }
#define cf_clamp(x, lo, hi)     \
	_Generic((x),               \
		float:    cf_clamp_f,   \
		double:   cf_clamp_d,   \
		int8_t:   cf_clamp_i8,  \
		uint8_t:  cf_clamp_u8,  \
		int16_t:  cf_clamp_i16, \
		uint16_t: cf_clamp_u16, \
		int32_t:  cf_clamp_i32, \
		uint32_t: cf_clamp_u32, \
		int64_t:  cf_clamp_i64, \
		uint64_t: cf_clamp_u64, \
		CF_V2:    cf_clamp_v2,  \
		default:  cf_clamp_i32  \
	)((x), (lo), (hi))
#endif

/**
 * @function cf_clamp01
 * @category math
 * @brief    Returns `x` clamped between 0 and 1.
 * @related  cf_min cf_max cf_abs cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
#define cf_clamp01(x) cf_max(0, cf_min(x, 1))
#undef cf_clamp01
#ifdef __cplusplus
} // extern "C"
CF_INLINE float    cf_clamp01(float    x) { return cf_max(0.0f, cf_min(x, 1.0f)); }
CF_INLINE double   cf_clamp01(double   x) { return cf_max(0.0,  cf_min(x, 1.0)); }
CF_INLINE int8_t   cf_clamp01(int8_t   x) { return cf_max((int8_t)0, (int8_t)cf_min(x, (int8_t)1)); }
CF_INLINE uint8_t  cf_clamp01(uint8_t  x) { return cf_max((uint8_t)0, (uint8_t)cf_min(x, (uint8_t)1)); }
CF_INLINE int16_t  cf_clamp01(int16_t  x) { return cf_max((int16_t)0, (int16_t)cf_min(x, (int16_t)1)); }
CF_INLINE uint16_t cf_clamp01(uint16_t x) { return cf_max((uint16_t)0, (uint16_t)cf_min(x, (uint16_t)1)); }
CF_INLINE int32_t  cf_clamp01(int32_t  x) { return cf_max((int32_t)0, (int32_t)cf_min(x, (int32_t)1)); }
CF_INLINE uint32_t cf_clamp01(uint32_t x) { return cf_max((uint32_t)0, (uint32_t)cf_min(x, (uint32_t)1)); }
CF_INLINE int64_t  cf_clamp01(int64_t  x) { return cf_max((int64_t)0, (int64_t)cf_min(x, (int64_t)1)); }
CF_INLINE uint64_t cf_clamp01(uint64_t x) { return cf_max((uint64_t)0, (uint64_t)cf_min(x, (uint64_t)1)); }
CF_INLINE CF_V2    cf_clamp01(CF_V2    x) { return cf_max(cf_v2(0), cf_min(x, cf_v2(1))); }
extern "C" {
#else
CF_INLINE float    cf_clamp01_f  (float    x) { return cf_max(0.0f, cf_min(x, 1.0f)); }
CF_INLINE double   cf_clamp01_d  (double   x) { return cf_max(0.0,  cf_min(x, 1.0)); }
CF_INLINE int8_t   cf_clamp01_i8 (int8_t   x) { return cf_max((int8_t)0, (int8_t)cf_min(x, (int8_t)1)); }
CF_INLINE uint8_t  cf_clamp01_u8 (uint8_t  x) { return cf_max((uint8_t)0, (uint8_t)cf_min(x, (uint8_t)1)); }
CF_INLINE int16_t  cf_clamp01_i16(int16_t  x) { return cf_max((int16_t)0, (int16_t)cf_min(x, (int16_t)1)); }
CF_INLINE uint16_t cf_clamp01_u16(uint16_t x) { return cf_max((uint16_t)0, (uint16_t)cf_min(x, (uint16_t)1)); }
CF_INLINE int32_t  cf_clamp01_i32(int32_t  x) { return cf_max((int32_t)0, (int32_t)cf_min(x, (int32_t)1)); }
CF_INLINE uint32_t cf_clamp01_u32(uint32_t x) { return cf_max((uint32_t)0, (uint32_t)cf_min(x, (uint32_t)1)); }
CF_INLINE int64_t  cf_clamp01_i64(int64_t  x) { return cf_max((int64_t)0, (int64_t)cf_min(x, (int64_t)1)); }
CF_INLINE uint64_t cf_clamp01_u64(uint64_t x) { return cf_max((uint64_t)0, (uint64_t)cf_min(x, (uint64_t)1)); }
CF_INLINE CF_V2    cf_clamp01_v2 (CF_V2    x) { return cf_max_v2(cf_v2(0), cf_min_v2(x, cf_v2(1))); }
#define cf_clamp01(x)             \
	_Generic((x),                 \
		float:    cf_clamp01_f,   \
		double:   cf_clamp01_d,   \
		int8_t:   cf_clamp01_i8,  \
		uint8_t:  cf_clamp01_u8,  \
		int16_t:  cf_clamp01_i16, \
		uint16_t: cf_clamp01_u16, \
		int32_t:  cf_clamp01_i32, \
		uint32_t: cf_clamp01_u32, \
		int64_t:  cf_clamp01_i64, \
		uint64_t: cf_clamp01_u64, \
		CF_V2:    cf_clamp01_v2,  \
		default:  cf_clamp01_i32  \
	)(x)
#endif

/**
 * @function cf_sign
 * @category math
 * @brief    Returns the sign (either 1 or -1) of `x`.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
#define cf_sign(a)
#undef cf_sign
#ifdef __cplusplus
} // extern "C"
CF_INLINE float    cf_sign(float    x) { return x < 0.0f ? -1.0f : 1.0f; }
CF_INLINE double   cf_sign(double   x) { return x < 0.0  ? -1.0  : 1.0; }
CF_INLINE int8_t   cf_sign(int8_t   x) { return x < 0 ? (int8_t)-1 : (int8_t)1; }
CF_INLINE int16_t  cf_sign(int16_t  x) { return x < 0 ? (int16_t)-1 : (int16_t)1; }
CF_INLINE int32_t  cf_sign(int32_t  x) { return x < 0 ? -1 : 1; }
CF_INLINE int64_t  cf_sign(int64_t  x) { return x < 0 ? -1 : 1; }
CF_INLINE uint8_t  cf_sign(uint8_t  x) { return (uint8_t)1; }
CF_INLINE uint16_t cf_sign(uint16_t x) { return (uint16_t)1; }
CF_INLINE uint32_t cf_sign(uint32_t x) { return 1u; }
CF_INLINE uint64_t cf_sign(uint64_t x) { return 1ull; }
CF_INLINE CF_V2    cf_sign(CF_V2    x) { return { cf_sign(x.x), cf_sign(x.y) }; }
extern "C" {
#else
CF_INLINE float    cf_sign_f  (float    x) { return x < 0.0f ? -1.0f : 1.0f; }
CF_INLINE double   cf_sign_d  (double   x) { return x < 0.0  ? -1.0  : 1.0; }
CF_INLINE int8_t   cf_sign_i8 (int8_t   x) { return x < 0 ? (int8_t)-1 : (int8_t)1; }
CF_INLINE int16_t  cf_sign_i16(int16_t  x) { return x < 0 ? (int16_t)-1 : (int16_t)1; }
CF_INLINE int32_t  cf_sign_i32(int32_t  x) { return x < 0 ? -1 : 1; }
CF_INLINE int64_t  cf_sign_i64(int64_t  x) { return x < 0 ? -1 : 1; }
CF_INLINE uint8_t  cf_sign_u8 (uint8_t  x) { CF_UNUSED(x); return (uint8_t)1; }
CF_INLINE uint16_t cf_sign_u16(uint16_t x) { CF_UNUSED(x); return (uint16_t)1; }
CF_INLINE uint32_t cf_sign_u32(uint32_t x) { CF_UNUSED(x); return 1u; }
CF_INLINE uint64_t cf_sign_u64(uint64_t x) { CF_UNUSED(x); return 1ull; }
CF_INLINE CF_V2    cf_sign_v2 (CF_V2    x) { return (CF_V2){ cf_sign_f(x.x), cf_sign_f(x.y) }; }
#define cf_sign(x) \
	_Generic((x), \
		float:    cf_sign_f,   \
		double:   cf_sign_d,   \
		int8_t:   cf_sign_i8,  \
		uint8_t:  cf_sign_u8,  \
		int16_t:  cf_sign_i16, \
		uint16_t: cf_sign_u16, \
		int32_t:  cf_sign_i32, \
		uint32_t: cf_sign_u32, \
		int64_t:  cf_sign_i64, \
		uint64_t: cf_sign_u64, \
		CF_V2:    cf_sign_v2,  \
		default:  cf_sign_i32  \
	)(x)
#endif

/**
 * @function cf_intersect
 * @category math
 * @brief    Given the distances of two points `a` and `b` to a plane (`da` and `db` respectively), compute the insterection
 *           value used to lerp from `a` to `b` to find the intersection point.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_intersect cf_safe_invert cf_lerp cf_remap cf_mod cf_fract
 */
CF_INLINE float cf_intersect(float da, float db) { return da / (da - db); }

/**
 * @function cf_add
 * @category math
 * @brief    Adds two 2D vectors component-wise.
 * @related  cf_sub cf_mul cf_div cf_dot cf_length
 */
CF_INLINE CF_V2 cf_add(CF_V2 a, CF_V2 b) { return cf_v2(a.x + b.x, a.y + b.y); }

/**
 * @function cf_sub
 * @category math
 * @brief    Subtracts two 2D vectors component-wise.
 * @related  cf_add cf_mul cf_div cf_dot cf_length
 */
CF_INLINE CF_V2 cf_sub(CF_V2 a, CF_V2 b) { return cf_v2(a.x - b.x, a.y - b.y); }

/**
 * @function cf_div
 * @category math
 * @brief    Divides vector `a` by `b`. Supports scalar and component-wise vector division.
 * @related  cf_mul cf_safe_invert cf_mul_T cf_sqrt
 */
#define cf_div(a, b)
#undef cf_div
#ifdef __cplusplus
} // extern "C"
CF_INLINE CF_V2 cf_div(CF_V2 a, CF_V2 b) { return cf_v2(a.x / b.x, a.y / b.y); }
CF_INLINE CF_V2 cf_div(CF_V2 a, float b) { return cf_v2(a.x / b, a.y / b); }
extern "C" {
#else
CF_INLINE CF_V2 cf_div_v2  (CF_V2 a, CF_V2 b) { return cf_v2(a.x / b.x, a.y / b.y); }
CF_INLINE CF_V2 cf_div_v2_f(CF_V2 a, float b) { return cf_v2(a.x / b, a.y / b); }
#define cf_div(a, b)          \
	_Generic((b),             \
		CF_V2:   cf_div_v2,   \
		float:   cf_div_v2_f, \
		default: cf_div_v2_f  \
	)((a), (b))
#endif

/**
 * @function cf_lesser
 * @category math
 * @brief    Returns true if `a.x < b.y` and `a.y < b.y`.
 * @related  CF_V2 cf_round cf_lesser cf_greater cf_lesser_equal cf_greater_equal cf_parallel
 */
CF_INLINE int cf_lesser(CF_V2 a, CF_V2 b) { return a.x < b.x && a.y < b.y; }

/**
 * @function cf_greater
 * @category math
 * @brief    Returns true if `a.x > b.y` and `a.y > b.y`.
 * @related  CF_V2 cf_round cf_lesser cf_greater cf_lesser_equal cf_greater_equal cf_parallel
 */
CF_INLINE int cf_greater(CF_V2 a, CF_V2 b) { return a.x > b.x && a.y > b.y; }

/**
 * @function cf_lesser_equal
 * @category math
 * @brief    Returns true if `a.x <= b.y` and `a.y <= b.y`.
 * @related  CF_V2 cf_round cf_lesser cf_greater cf_lesser_equal cf_greater_equal cf_parallel
 */
CF_INLINE int cf_lesser_equal(CF_V2 a, CF_V2 b) { return a.x <= b.x && a.y <= b.y; }

/**
 * @function cf_greater_equal
 * @category math
 * @brief    Returns true if `a.x >= b.y` and `a.y >= b.y`.
 * @related  CF_V2 cf_round cf_lesser cf_greater cf_lesser_equal cf_greater_equal cf_parallel
 */
CF_INLINE int cf_greater_equal(CF_V2 a, CF_V2 b) { return a.x >= b.x && a.y >= b.y; }

/**
 * @function cf_equal
 * @category math
 * @brief    TODO
 * @related  TODO
 */
CF_INLINE int cf_equal(CF_V2 a, CF_V2 b) { return a.x == b.x && a.y == b.y; }

/**
 * @function cf_equals
 * @category math
 * @brief    TODO
 * @related  TODO
 */
CF_INLINE int cf_equals(CF_V2 a, CF_V2 b) { return a.x == b.x && a.y == b.y; }

/**
 * @function cf_round
 * @category math
 * @brief    Rounds each component of `x` to the nearest integer.
 * @related  cf_floor cf_ceil cf_trunc cf_abs
 */
#define cf_round(x)
#undef cf_round
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_round(float  x) { return CF_ROUNDF(x); }
CF_INLINE double cf_round(double x) { return CF_ROUND(x); }
CF_INLINE CF_V2  cf_round(CF_V2  v) { return cf_v2(CF_ROUNDF(v.x), CF_ROUNDF(v.y)); }
extern "C" {
#else
CF_INLINE float  cf_round_f(float  x)  { return CF_ROUNDF(x); }
CF_INLINE double cf_round_d(double x)  { return CF_ROUND(x); }
CF_INLINE CF_V2  cf_round_v2(CF_V2  v) { return cf_v2(CF_ROUNDF(v.x), CF_ROUNDF(v.y)); }

#define cf_round(x)           \
	_Generic((x),             \
		float:   cf_round_f,  \
		double:  cf_round_d,  \
		CF_V2:   cf_round_v2, \
		default: cf_round_f   \
	)(x)
#endif

/**
 * @function cf_remap
 * @category math
 * @brief    Remaps `t` from [old_lo, old_hi] to [lo, hi].
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_lerp cf_remap cf_remap01 cf_mod cf_fract
 */
#define cf_remap(t, old_lo, old_hi, lo, hi)
#undef cf_remap
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_remap(float  t, float  old_lo, float  old_hi, float  lo, float  hi) { return lo + ((old_hi - old_lo) != 0 ? (t - old_lo) / (old_hi - old_lo) : 0.0f) * (hi - lo); }
CF_INLINE double cf_remap(double t, double old_lo, double old_hi, double lo, double hi) { return lo + ((old_hi - old_lo) != 0 ? (t - old_lo) / (old_hi - old_lo) : 0.0)  * (hi - lo); }
extern "C" {
#else
CF_INLINE float  cf_remap_f(float  t, float  old_lo, float  old_hi, float  lo, float  hi) { return lo + ((old_hi - old_lo) != 0 ? (t - old_lo) / (old_hi - old_lo) : 0.0f) * (hi - lo); }
CF_INLINE double cf_remap_d(double t, double old_lo, double old_hi, double lo, double hi) { return lo + ((old_hi - old_lo) != 0 ? (t - old_lo) / (old_hi - old_lo) : 0.0)  * (hi - lo); }
#define cf_remap(t, old_lo, old_hi, lo, hi) \
	_Generic((t),            \
		float:   cf_remap_f, \
		double:  cf_remap_d, \
		default: cf_remap_f  \
	)((t), (old_lo), (old_hi), (lo), (hi))
#endif

/**
 * @function cf_floor
 * @category math
 * @brief    Returns the largest integer less than or equal to `x`.
 * @related  cf_ceil cf_round cf_fract cf_mod
 */
#define cf_floor(x)
#undef cf_floor
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_floor(float  x) { return CF_FLOORF(x); }
CF_INLINE double cf_floor(double x) { return CF_FLOOR(x); }
CF_INLINE CF_V2  cf_floor(CF_V2  x) { return cf_v2(CF_FLOORF(x.x), CF_FLOORF(x.y)); }
extern "C" {
#else
CF_INLINE float  cf_floor_f(float  x) { return CF_FLOORF(x); }
CF_INLINE double cf_floor_d(double x) { return CF_FLOOR(x); }
CF_INLINE CF_V2  cf_floor_v2(CF_V2 x) { return cf_v2(CF_FLOORF(x.x), CF_FLOORF(x.y)); }
#define cf_floor(x)            \
	_Generic((x),              \
		float:    cf_floor_f,  \
		double:   cf_floor_d,  \
		CF_V2:    cf_floor_v2, \
		default:  cf_floor_f   \
	)(x)
#endif

/**
 * @function cf_fract
 * @category math
 * @brief    Returns the fractional part of `x`.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_lerp cf_remap cf_mod cf_fract
 */
#define cf_fract(x)
#undef cf_fract
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_fract(float  x) { return x - CF_FLOORF(x); }
CF_INLINE double cf_fract(double x) { return x - CF_FLOOR(x); }
CF_INLINE CF_V2  cf_fract(CF_V2  x) { return cf_sub(x, cf_floor(x)); }
extern "C" {
#else
CF_INLINE float  cf_fract_f(float  x) { return x - CF_FLOORF(x); }
CF_INLINE double cf_fract_d(double x) { return x - CF_FLOOR(x); }
CF_INLINE CF_V2  cf_fract_v2(CF_V2 x) { return cf_sub(x, cf_floor(x)); }
#define cf_fract(x) \
	_Generic((x), \
		float:    cf_fract_f,  \
		double:   cf_fract_d,  \
		CF_V2:    cf_fract_v2, \
		default:  cf_fract_f   \
	)(x)
#endif

/**
 * @function cf_ceil
 * @category math
 * @brief    Returns the smallest integer greater than or equal to `x`.
 * @related  cf_floor cf_round cf_fract cf_mod
 */
#define cf_ceil(x)
#undef cf_ceil
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_ceil(float  x) { return CF_CEILF(x); }
CF_INLINE double cf_ceil(double x) { return CF_CEIL(x); }
CF_INLINE CF_V2  cf_ceil(CF_V2  x) { return cf_v2(CF_CEILF(x.x), CF_CEILF(x.y)); }
extern "C" {
#else
CF_INLINE float  cf_ceil_f(float  x) { return CF_CEILF(x); }
CF_INLINE double cf_ceil_d(double x) { return CF_CEIL(x); }
CF_INLINE CF_V2  cf_ceil_v2(CF_V2 x) { return cf_v2(CF_CEILF(x.x), CF_CEILF(x.y)); }
#define cf_ceil(x)           \
	_Generic((x),            \
		float:   cf_ceil_f,  \
		double:  cf_ceil_d,  \
		CF_V2:   cf_ceil_v2, \
		default: cf_ceil_f   \
	)(x)
#endif

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
// Trig functions.

/**
 * @function cf_sin
 * @category math
 * @brief    Returns the sine of `x` (angle in radians).
 * @related  cf_cos cf_tan cf_asin cf_acos cf_atan2
 */
#define cf_sin(x)
#undef cf_sin
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_sin(float  x) { return CF_SINF(x); }
CF_INLINE double cf_sin(double x) { return CF_SIN(x); }
CF_INLINE CF_V2  cf_sin(CF_V2  x) { return { cf_sin(x.x), cf_sin(x.y) }; }
extern "C" {
#else
CF_INLINE float  cf_sin_f(float  x) { return CF_SINF(x); }
CF_INLINE double cf_sin_d(double x) { return CF_SIN(x); }
CF_INLINE CF_V2  cf_sin_v2(CF_V2 x) { return cf_v2(CF_SINF(x.x), CF_SINF(x.y)); }
#define cf_sin(x)           \
	_Generic((x),           \
		float:   cf_sin_f,  \
		double:  cf_sin_d,  \
		CF_V2:   cf_sin_v2, \
		default: cf_sin_f   \
	)(x)
#endif

/**
 * @function cf_cos
 * @category math
 * @brief    Returns the cosine of `x` (angle in radians).
 * @related  cf_sin cf_tan cf_asin cf_acos cf_atan2
 */
#define cf_cos(x)
#undef cf_cos
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_cos(float  x) { return CF_COSF(x); }
CF_INLINE double cf_cos(double x) { return CF_COS(x); }
CF_INLINE CF_V2  cf_cos(CF_V2  x) { return cf_v2(CF_COSF(x.x), CF_COSF(x.y)); }
extern "C" {
#else
CF_INLINE float  cf_cos_f(float  x) { return CF_COSF(x); }
CF_INLINE double cf_cos_d(double x) { return CF_COS(x); }
CF_INLINE CF_V2  cf_cos_v2(CF_V2 x) { return cf_v2(CF_COSF(x.x), CF_COSF(x.y)); }
#define cf_cos(x)           \
	_Generic((x),           \
		float:   cf_cos_f,  \
		double:  cf_cos_d,  \
		CF_V2:   cf_cos_v2, \
		default: cf_cos_f   \
	)(x)
#endif

/**
 * @function cf_asin
 * @category math
 * @brief    Returns the arc-sine of `x` in radians.
 * @related  cf_acos cf_atan2 cf_sin cf_cos
 */
#define cf_asin(x)
#undef cf_asin
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_asin(float  x) { return asinf(x); }
CF_INLINE double cf_asin(double x) { return asin(x); }
CF_INLINE CF_V2  cf_asin(CF_V2  x) { return cf_v2(CF_ASINF(x.x), CF_ASINF(x.y)); }
extern "C" {
#else
CF_INLINE float  cf_asin_f(float  x) { return asinf(x); }
CF_INLINE double cf_asin_d(double x) { return asin(x); }
CF_INLINE CF_V2  cf_asin_v2(CF_V2 x) { return cf_v2(CF_ASINF(x.x), CF_ASINF(x.y)); }
#define cf_asin(x)           \
	_Generic((x),            \
		float:   cf_asin_f,  \
		double:  cf_asin_d,  \
		CF_V2:   cf_asin_v2, \
		default: cf_asin_f   \
	)(x)
#endif

/**
 * @function cf_acos
 * @category math
 * @brief    Returns the arc-cosine of `x` in radians.
 * @related  cf_asin cf_atan2 cf_sin cf_cos
 */
#define cf_acos(x)
#undef cf_acos
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_acos(float  x) { return CF_ACOSF(x); }
CF_INLINE double cf_acos(double x) { return CF_ACOS(x); }
CF_INLINE CF_V2  cf_acos(CF_V2  x) { return cf_v2(CF_ACOSF(x.x), CF_ACOSF(x.y)); }
extern "C" {
#else
CF_INLINE float  cf_acos_f(float  x) { return CF_ACOSF(x); }
CF_INLINE double cf_acos_d(double x) { return CF_ACOS(x); }
CF_INLINE CF_V2  cf_acos_v2(CF_V2 x) { return cf_v2(CF_ACOSF(x.x), CF_ACOSF(x.y)); }
#define cf_acos(x)           \
	_Generic((x),            \
		float:   cf_acos_f,  \
		double:  cf_acos_d,  \
		CF_V2:   cf_acos_v2, \
		default: cf_acos_f   \
	)(x)
#endif

/**
 * @function cf_atan2
 * @category math
 * @brief    Returns the arc-tangent of `y / x` in radians.
 * @related  cf_asin cf_acos cf_sin cf_cos
 */
#define cf_atan2(y, x)
#undef cf_atan2
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_atan2(float  y, float  x) { return CF_ATAN2F(y, x); }
CF_INLINE double cf_atan2(double y, double x) { return CF_ATAN2(y, x); }
CF_INLINE float  cf_atan2(CF_V2  v)           { return CF_ATAN2F(v.y, v.x); }
extern "C" {
#else
CF_INLINE float  cf_atan2_f (float  y, float  x) { return CF_ATAN2F(y, x); }
CF_INLINE double cf_atan2_d (double y, double x) { return CF_ATAN2(y, x); }
CF_INLINE float  cf_atan2_v2(CF_V2  v)           { return CF_ATAN2F(v.y, v.x); }
#define cf_atan2(y, ...)      \
	_Generic((y),             \
		float:   cf_atan2_f,  \
		double:  cf_atan2_d,  \
		CF_V2:   cf_atan2_v2, \
		default: cf_atan2_f   \
	)((y), (__VA_ARGS__))
#endif

/**
 * @function cf_sqrt
 * @category math
 * @brief    Returns the square root of `x`.
 * @related  cf_abs cf_pow cf_square cf_length cf_normalize
 */
#define cf_sqrt(x)
#undef cf_sqrt
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_sqrt(float  x) { return CF_SQRTF(x); }
CF_INLINE double cf_sqrt(double x) { return CF_SQRT(x); }
extern "C" {
#else
CF_INLINE float  cf_sqrt_f(float  x) { return CF_SQRTF(x); }
CF_INLINE double cf_sqrt_d(double x) { return CF_SQRT(x); }
#define cf_sqrt(x)           \
	_Generic((x),            \
		float:   cf_sqrt_f,  \
		double:  cf_sqrt_d,  \
		default: cf_sqrt_f   \
	)(x)
#endif

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
CF_INLINE float cf_cube_in_out(float x) { if (x < 0.5f) return 4.0f * x * x * x; else { return 4.0f * x * x * x - 3.0f * x * x + 1.0f; } }

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

/**
 * @function cf_dot
 * @category math
 * @brief    Returns the dot product of two vectors.
 * @related  CF_V2 cf_add cf_sub cf_dot cf_mul_v2_f cf_div_v2_f
 */
CF_INLINE float cf_dot(CF_V2 a, CF_V2 b) { return a.x * b.x + a.y * b.y; }

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

#define cf_sincos(a)
#undef cf_sincos
#ifdef __cplusplus
} // extern "C"
CF_INLINE CF_SinCos cf_sincos(float radians) { CF_SinCos r; r.s = CF_SINF(radians); r.c = CF_COSF(radians); return r; }
CF_INLINE CF_SinCos cf_sincos() { CF_SinCos r; r.c = 1.0f; r.s = 0.0f; return r; }
CF_INLINE CF_SinCos cf_sincos_identity() { CF_SinCos r; r.c = 1.0f; r.s = 0.0f; return r; }
extern "C" {
#else
CF_INLINE CF_SinCos cf_sincos(float radians) { CF_SinCos r; r.s = CF_SINF(radians); r.c = CF_COSF(radians); return r; }
CF_INLINE CF_SinCos cf_sincos_identity(void) { CF_SinCos r; r.c = 1.0f; r.s = 0.0f; return r; }
#endif

/**
 * @function cf_cross
 * @category math
 * @brief    Computes the 2D cross product between `a` and `b`.
 * @remarks  Returns a scalar when both arguments are vectors, or a perpendicular vector when one argument is a scalar.
 * @related  cf_dot cf_det2 cf_mul cf_div
 */
#define cf_cross(a, b)
#undef cf_cross
#ifdef __cplusplus
} // extern "C"
CF_INLINE float cf_cross(CF_V2 a, CF_V2 b) { return cf_det2(a, b); }
CF_INLINE CF_V2 cf_cross(CF_V2 a, float b) { return cf_v2(b * a.y, -b * a.x); }
CF_INLINE CF_V2 cf_cross(float a, CF_V2 b) { return cf_v2(-a * b.y, a * b.x); }
extern "C" {
#else
CF_INLINE float cf_cross_v2(CF_V2 a, CF_V2 b) { return cf_det2(a, b); }
CF_INLINE CF_V2 cf_cross_v2_f(CF_V2 a, float b) { return cf_v2(b * a.y, -b * a.x); }
CF_INLINE CF_V2 cf_cross_f_v2(float a, CF_V2 b) { return cf_v2(-a * b.y, a * b.x); }
#define cf_cross(a, b)              \
	_Generic((a),                   \
		CF_V2: _Generic((b),        \
			CF_V2:   cf_cross_v2,   \
			float:   cf_cross_v2_f, \
			default: cf_cross_v2_f  \
		),                          \
		float: _Generic((b),        \
			CF_V2:   cf_cross_f_v2, \
			default: cf_cross_f_v2  \
		),                          \
	default: cf_cross_v2            \
	)((a), (b))
#endif


CF_INLINE CF_V2 cf_neg(CF_V2 a) { return cf_v2(-a.x, -a.y); }

/**
 * @function cf_safe_invert
 * @category math
 * @brief    Safely inverts `a`, returning `0` when components are zero to avoid division-by-zero.
 * @related  cf_div cf_mul cf_safe_invert cf_normalize
 */
#define cf_safe_invert(a)
#undef cf_safe_invert
#ifdef __cplusplus
} // extern "C"
CF_INLINE float cf_safe_invert(float a) { return a != 0.0f ? 1.0f / a : 0.0f; }
CF_INLINE CF_V2 cf_safe_invert(CF_V2 a) { return cf_v2(cf_safe_invert(a.x), cf_safe_invert(a.y)); }
extern "C" {
#else
CF_INLINE float cf_safe_invert_f (float a) { return a != 0.0f ? 1.0f / a : 0.0f; }
CF_INLINE CF_V2 cf_safe_invert_v2(CF_V2 a) { return cf_v2(cf_safe_invert_f(a.x), cf_safe_invert_f(a.y)); }
#define cf_safe_invert(a)          \
	_Generic((a),                  \
		float:  cf_safe_invert_f,  \
		CF_V2:  cf_safe_invert_v2, \
		default: cf_safe_invert_f  \
	)(a)
#endif

/**
 * @function cf_atan2_360
 * @category math
 * @brief    Returns the full-range (0-2pi) angle of the vector or rotation input.
 * @remarks  Works with raw (y, x), `CF_V2`, or `CF_SinCos` values.
 * @related  cf_sin cf_cos cf_sincos cf_mul cf_mul_T
 */
#define cf_atan2_360(a, b)
#undef cf_atan2_360
#ifdef __cplusplus
} // extern "C"
CF_INLINE float cf_atan2_360(float y, float x) { return CF_ATAN2F(-y, -x) + CF_PI; }
CF_INLINE float cf_atan2_360(CF_SinCos r) { return cf_atan2_360(r.s, r.c); }
CF_INLINE float cf_atan2_360(CF_V2 v) { return CF_ATAN2F(-v.y, -v.x) + CF_PI; }
extern "C" {
#else
CF_INLINE float cf_atan2_360_f_f(float y, float x) { return CF_ATAN2F(-y, -x) + CF_PI; }
CF_INLINE float cf_atan2_360_sc(CF_SinCos r)       { return cf_atan2_360_f_f(r.s, r.c); }
CF_INLINE float cf_atan2_360_v2(CF_V2 v)           { return CF_ATAN2F(-v.y, -v.x) + CF_PI; }

#define _CF_ATAN2_360_1ARG(a) _Generic((a), CF_V2: cf_atan2_360_v2, CF_SinCos: cf_atan2_360_sc)(a)
#define _CF_ATAN2_360_SELECT(_1, _2, NAME, ...) NAME
#define cf_atan2_360(...) \
	CF_EXPAND(_CF_ATAN2_360_SELECT(__VA_ARGS__, cf_atan2_360_f_f, _CF_ATAN2_360_1ARG)(__VA_ARGS__))
#endif

/**
 * @function cf_make_translation
 * @category math
 * @brief    Constructs a 2D translation matrix from position `p` or scalar components `x, y`.
 * @related  cf_make_scale cf_make_rotation cf_mul cf_mul_T
 */
#define cf_make_translation(a, b)
#undef cf_make_translation
#ifdef __cplusplus
} // extern "C"
CF_INLINE CF_M3x2 cf_make_translation(float x, float y) { CF_M3x2 m; m.m.x = cf_v2(1,0); m.m.y = cf_v2(0,1); m.p = cf_v2(x,y); return m; }
CF_INLINE CF_M3x2 cf_make_translation(CF_V2 p) { return cf_make_translation(p.x,p.y); }
extern "C" {
#else
CF_INLINE CF_M3x2 cf_make_translation_f_f(float x, float y) { CF_M3x2 m; m.m.x = cf_v2(1,0); m.m.y = cf_v2(0,1); m.p = cf_v2(x,y); return m; }
CF_INLINE CF_M3x2 cf_make_translation_v2(CF_V2 p) { return cf_make_translation_f_f(p.x,p.y); }

#define _CF_MAKE_TRANSLATION_SELECT(_1, _2, NAME, ...) NAME
#define cf_make_translation(...)         \
	CF_EXPAND(_CF_MAKE_TRANSLATION_SELECT(__VA_ARGS__, cf_make_translation_f_f, cf_make_translation_v2)(__VA_ARGS__))
#endif

/**
 * @function cf_make_scale
 * @category math
 * @brief    Constructs a 2D scale matrix from a vector or uniform scalar.
 * @related  cf_make_translation cf_make_rotation cf_mul cf_mul_T
 */
#define cf_make_scale(a)
#undef cf_make_scale
#ifdef __cplusplus
} // extern "C"
CF_INLINE CF_M3x2 cf_make_scale(CF_V2 s) { CF_M3x2 m; m.m.x = cf_v2(s.x,0); m.m.y = cf_v2(0,s.y); m.p = cf_v2(0,0); return m; }
CF_INLINE CF_M3x2 cf_make_scale(float s) { return cf_make_scale(cf_v2(s,s)); }
extern "C" {
#else
CF_INLINE CF_M3x2 cf_make_scale_v2(CF_V2 s) { CF_M3x2 m; m.m.x = cf_v2(s.x,0); m.m.y = cf_v2(0,s.y); m.p = cf_v2(0,0); return m; }
CF_INLINE CF_M3x2 cf_make_scale_f(float s) { return cf_make_scale_v2(cf_v2(s,s)); }

#define cf_make_scale(a)           \
	_Generic((a),                  \
		CF_V2:   cf_make_scale_v2, \
		float:   cf_make_scale_f,  \
		default: cf_make_scale_f   \
	)(a)
#endif

/**
 * @function cf_make_scale_translation
 * @category math
 * @brief    Constructs a 2D scale + translation matrix from scale `s` and position `p`.
 * @related  cf_make_scale cf_make_translation cf_make_rotation cf_mul cf_mul_T
 */
#define cf_make_scale_translation(a, b)
#undef cf_make_scale_translation
#ifdef __cplusplus
} // extern "C"
CF_INLINE CF_M3x2 cf_make_scale_translation(CF_V2 s, CF_V2 p) { CF_M3x2 m; m.m.x = cf_v2(s.x,0); m.m.y = cf_v2(0,s.y); m.p = p; return m; }
CF_INLINE CF_M3x2 cf_make_scale_translation(float s, CF_V2 p) { return cf_make_scale_translation(cf_v2(s,s), p); }
CF_INLINE CF_M3x2 cf_make_scale_translation(float sx, float sy, CF_V2 p) { return cf_make_scale_translation(cf_v2(sx,sy), p); }
extern "C" {
#else
CF_INLINE CF_M3x2 cf_make_scale_translation_v2_v2(CF_V2 s, CF_V2 p) { CF_M3x2 m; m.m.x = cf_v2(s.x,0); m.m.y = cf_v2(0,s.y); m.p = p; return m; }
CF_INLINE CF_M3x2 cf_make_scale_translation_f_v2(float s, CF_V2 p) { return cf_make_scale_translation_v2_v2(cf_v2(s,s), p); }
CF_INLINE CF_M3x2 cf_make_scale_translation_f_f_v2(float sx, float sy, CF_V2 p) { return cf_make_scale_translation_v2_v2(cf_v2(sx,sy), p); }

#define cf_make_scale_translation(a, b)           \
	_Generic((a),                                 \
		CF_V2:   cf_make_scale_translation_v2_v2, \
		float:   cf_make_scale_translation_f_v2,  \
		default: cf_make_scale_translation_f_v2   \
	)((a), (b))
#endif

/**
 * @function cf_origin
 * @category math
 * @brief    Returns the origin projected onto the plane.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mul_T_tf_hs cf_intersect_halfspace
 */
CF_INLINE CF_V2 cf_origin(CF_Halfspace h) { return cf_v2(h.n.x * h.d, h.n.y * h.d); }

/**
 * @function cf_distance_hs
 * @category math
 * @brief    Returns distance of a point to the plane.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mul_T_tf_hs cf_intersect_halfspace
 */
CF_INLINE float cf_distance_hs(CF_Halfspace h, CF_V2 p) { return cf_dot(h.n, p) - h.d; }

/**
 * @function cf_project
 * @category math
 * @brief    Projects a point onto the surface of the plane.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mul_T_tf_hs cf_intersect_halfspace
 */
CF_INLINE CF_V2 cf_project(CF_Halfspace h, CF_V2 p) { float d = cf_distance_hs(h, p); return cf_sub(p, cf_v2(h.n.x * d, h.n.y * d)); }

/**
 * @function cf_intersect_halfspace
 * @category math
 * @brief    Returns the intersection point of two points to a plane.
 * @remarks  The distance to the plane are provided as `da` and `db`. You can compute these with e.g. `cf_distance_hs`, or instead
 *           call the similar function `cf_intersect_halfspace2`.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mul_T_tf_hs cf_intersect_halfspace cf_intersect_haflspace2 cf_intersect_haflspace3
 */
CF_INLINE CF_V2 cf_intersect_halfspace(CF_V2 a, CF_V2 b, float da, float db) { float d = (da / (da - db)); CF_V2 ab = cf_sub(b, a); return cf_add(a, cf_v2(ab.x * d, ab.y * d)); }

/**
 * @function cf_intersect_halfspace2
 * @category math
 * @brief    Returns the intersection point of two points to a plane.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mul_T_tf_hs cf_intersect_halfspace cf_intersect_haflspace2 cf_intersect_haflspace3
 */
CF_INLINE CF_V2 cf_intersect_halfspace2(CF_Halfspace h, CF_V2 a, CF_V2 b) { return cf_intersect_halfspace(a, b, cf_distance_hs(h, a), cf_distance_hs(h, b)); }

/**
 * @function cf_intersect_halfspace3
 * @category math
 * @brief    Returns the intersection point of two planes.
 * @related  CF_Halfspace cf_plane cf_origin cf_distance_hs cf_project cf_mul_tf_hs cf_mul_T_tf_hs cf_intersect_halfspace cf_intersect_haflspace2 cf_intersect_haflspace3
 */
CF_INLINE CF_V2 cf_intersect_halfspace3(CF_Halfspace ha, CF_Halfspace hb) { CF_V2 a = {ha.n.x, hb.n.x}, b = {ha.n.y, hb.n.y}, c = {ha.d, hb.d}; float x = cf_det2(c, b) / cf_det2(a, b); float y = cf_det2(a, c) / cf_det2(a, b); return cf_v2(x, y); }


CF_INLINE CF_V2 cf_mul_sc_v2(CF_SinCos a, CF_V2 b) { return cf_v2(a.c * b.x - a.s * b.y, a.s * b.x + a.c * b.y); }
CF_INLINE CF_V2 cf_mul_T_sc_v2(CF_SinCos a, CF_V2 b) { return cf_v2(a.c * b.x + a.s * b.y, -a.s * b.x + a.c * b.y); }
CF_INLINE CF_SinCos cf_mul_sc(CF_SinCos a, CF_SinCos b) { CF_SinCos c; c.c = a.c * b.c - a.s * b.s; c.s = a.s * b.c + a.c * b.s; return c; }
CF_INLINE CF_SinCos cf_mul_T_sc(CF_SinCos a, CF_SinCos b) { CF_SinCos c; c.c = a.c * b.c + a.s * b.s; c.s = a.c * b.s - a.s * b.c; return c; }

CF_INLINE CF_V2 cf_mul_v2_f(CF_V2 a, float b) { return cf_v2(a.x * b, a.y * b); }
CF_INLINE CF_V2 cf_mul_v2(CF_V2 a, CF_V2 b) { return cf_v2(a.x * b.x, a.y * b.y); }

CF_INLINE CF_M2x2 cf_mul_m2_f(CF_M2x2 a, float b) { CF_M2x2 c; c.x = cf_mul_v2_f(a.x, b); c.y = cf_mul_v2_f(a.y, b); return c; }
CF_INLINE CF_V2 cf_mul_m2_v2(CF_M2x2 a, CF_V2 b)   { CF_V2 c; c.x = a.x.x * b.x + a.y.x * b.y; c.y = a.x.y * b.x + a.y.y * b.y; return c; }
CF_INLINE CF_V2 cf_mul_T_m2_v2(CF_M2x2 a, CF_V2 b) { CF_V2 c; c.x = a.x.x * b.x + a.x.y * b.y; c.y = a.y.x * b.x + a.y.y * b.y; return c; }
CF_INLINE CF_M2x2 cf_mul_m2(CF_M2x2 a, CF_M2x2 b) { CF_M2x2 c; c.x = cf_mul_m2_v2(a, b.x);  c.y = cf_mul_m2_v2(a, b.y); return c; }
CF_INLINE CF_M2x2 cf_mul_T_m2(CF_M2x2 a, CF_M2x2 b) { CF_M2x2 c; c.x = cf_mul_T_m2_v2(a, b.x); c.y = cf_mul_T_m2_v2(a, b.y); return c; }

CF_INLINE CF_V2 cf_mul_m32_v2(CF_M3x2 a, CF_V2 b) { return cf_add(cf_mul_m2_v2(a.m, b), a.p); }
CF_INLINE CF_M3x2 cf_mul_m32(CF_M3x2 a, CF_M3x2 b) { CF_M3x2 c; c.m = cf_mul_m2(a.m, b.m); c.p = cf_add(cf_mul_m2_v2(a.m, b.p), a.p); return c; }

CF_INLINE CF_V2 cf_mul_tf_v2(CF_Transform a, CF_V2 b) { return cf_add(cf_mul_sc_v2(a.r, b), a.p); }
CF_INLINE CF_V2 cf_mul_T_tf_v2(CF_Transform a, CF_V2 b) { return cf_mul_T_sc_v2(a.r, cf_sub(b, a.p)); }
CF_INLINE CF_Transform cf_mul_tf(CF_Transform a, CF_Transform b) { CF_Transform c; c.r = cf_mul_sc(a.r, b.r); c.p = cf_add(cf_mul_sc_v2(a.r, b.p), a.p); return c; }
CF_INLINE CF_Transform cf_mul_T_tf(CF_Transform a, CF_Transform b) { CF_Transform c; c.r = cf_mul_T_sc(a.r, b.r); c.p = cf_mul_T_sc_v2(a.r, cf_sub(b.p, a.p)); return c; }
CF_INLINE CF_Halfspace cf_mul_tf_hs(CF_Transform a, CF_Halfspace b) { CF_Halfspace c; c.n = cf_mul_sc_v2(a.r, b.n); c.d = cf_dot(cf_mul_tf_v2(a, cf_origin(b)), c.n); return c; }
CF_INLINE CF_Halfspace cf_mul_T_tf_hs(CF_Transform a, CF_Halfspace b) { CF_Halfspace c; c.n = cf_mul_T_sc_v2(a.r, b.n); c.d = cf_dot(cf_mul_T_tf_v2(a, cf_origin(b)), c.n); return c; }

/**
 * @function cf_mul
 * @category math
 * @brief    Multiplies `a` by `b`. Supports scalars, vectors, matrices, transforms, and rotations.
 * @remarks  Performs standard composition (not transposed/inverted). Use cf_mul_T for the transposed/inverted variant.
 * @related  cf_mul_T cf_div cf_add cf_sub cf_safe_invert
 */
#define cf_mul(a, b)
#undef cf_mul
#ifdef __cplusplus
} // extern "C"
CF_INLINE CF_SinCos    cf_mul(CF_SinCos a, CF_SinCos b) { CF_SinCos c; c.c = a.c*b.c - a.s*b.s; c.s = a.s*b.c + a.c*b.s; return c; }
CF_INLINE CF_V2        cf_mul(CF_SinCos a, CF_V2 b) { return cf_v2(a.c*b.x - a.s*b.y, a.s*b.x + a.c*b.y); }
CF_INLINE CF_V2        cf_mul(CF_V2 a, CF_V2 b) { return cf_v2(a.x*b.x, a.y*b.y); }
CF_INLINE CF_V2        cf_mul(CF_V2 a, float b) { return cf_v2(a.x*b, a.y*b); }
CF_INLINE CF_M2x2      cf_mul(CF_M2x2 a, CF_M2x2 b) { CF_M2x2 c; c.x = cf_mul_m2_v2(a,b.x); c.y = cf_mul_m2_v2(a,b.y); return c; }
CF_INLINE CF_M2x2      cf_mul(CF_M2x2 a, float b) { CF_M2x2 c; c.x = cf_mul_v2_f(a.x,b); c.y = cf_mul_v2_f(a.y,b); return c; }
CF_INLINE CF_V2        cf_mul(CF_M2x2 a, CF_V2 b) { CF_V2 c; c.x = a.x.x*b.x + a.y.x*b.y; c.y = a.x.y*b.x + a.y.y*b.y; return c; }
CF_INLINE CF_M3x2      cf_mul(CF_M3x2 a, CF_M3x2 b) { CF_M3x2 c; c.m = cf_mul_m2(a.m,b.m); c.p = cf_add(cf_mul_m2_v2(a.m,b.p),a.p); return c; }
CF_INLINE CF_V2        cf_mul(CF_M3x2 a, CF_V2 b) { return cf_add(cf_mul_m2_v2(a.m,b),a.p); }
CF_INLINE CF_V2        cf_mul(CF_Transform a, CF_V2 b) { return cf_add(cf_mul_sc_v2(a.r,b),a.p); }
CF_INLINE CF_Transform cf_mul(CF_Transform a, CF_Transform b) { CF_Transform c; c.r = cf_mul_sc(a.r,b.r); c.p = cf_add(cf_mul_sc_v2(a.r,b.p),a.p); return c; }
CF_INLINE CF_Halfspace cf_mul(CF_Transform a, CF_Halfspace b) { CF_Halfspace c; c.n = cf_mul_sc_v2(a.r,b.n); c.d = cf_dot(cf_mul_tf_v2(a,cf_origin(b)),c.n); return c; }
extern "C" {
#else
#define cf_mul(a, b) \
	_Generic((a), \
		CF_SinCos: _Generic((b), \
			CF_SinCos: cf_mul_sc, \
			CF_V2:     cf_mul_sc_v2, \
			default:   cf_mul_sc \
		), \
		CF_V2: _Generic((b), \
			CF_V2:   cf_mul_v2, \
			float:   cf_mul_v2_f, \
			default: cf_mul_v2 \
		), \
		CF_M2x2: _Generic((b), \
			CF_M2x2: cf_mul_m2, \
			CF_V2:   cf_mul_m2_v2, \
			float:   cf_mul_m2_f, \
			default: cf_mul_m2 \
		), \
		CF_M3x2: _Generic((b), \
			CF_M3x2: cf_mul_m32, \
			CF_V2:   cf_mul_m32_v2, \
			default: cf_mul_m32_v2 \
		), \
		CF_Transform: _Generic((b), \
			CF_Transform: cf_mul_tf, \
			CF_V2:        cf_mul_tf_v2, \
			CF_Halfspace: cf_mul_tf_hs, \
			default:      cf_mul_tf_v2 \
		), \
	default: cf_mul_v2 \
	)((a), (b))
#endif

/**
 * @function cf_mul_T
 * @category math
 * @brief    Multiplies `a` by `b` using the transposed or inverted form of `a`.
 * @remarks  These functions are _slightly_ faster than inverting/transposing `a` explicitly, by avoiding making temporary copies.
 *           However, it's mostly just here for convenience.
 * @related  cf_mul cf_div cf_safe_invert cf_add cf_sub
 */
#define cf_mul_T(a, b)
#undef cf_mul_T
#ifdef __cplusplus
} // extern "C"
CF_INLINE CF_SinCos    cf_mul_T(CF_SinCos a, CF_SinCos b) { CF_SinCos c; c.c = a.c*b.c + a.s*b.s; c.s = a.c*b.s - a.s*b.c; return c; }
CF_INLINE CF_V2        cf_mul_T(CF_SinCos a, CF_V2 b) { return cf_v2(a.c*b.x + a.s*b.y, -a.s*b.x + a.c*b.y); }
CF_INLINE CF_V2        cf_mul_T(CF_M2x2 a, CF_V2 b) { CF_V2 c; c.x = a.x.x*b.x + a.x.y*b.y; c.y = a.y.x*b.x + a.y.y*b.y; return c; }
CF_INLINE CF_M2x2      cf_mul_T(CF_M2x2 a, CF_M2x2 b) { CF_M2x2 c; c.x = cf_mul_T_m2_v2(a,b.x); c.y = cf_mul_T_m2_v2(a,b.y); return c; }
CF_INLINE CF_V2        cf_mul_T(CF_M3x2 a, CF_V2 b) { return cf_mul_T_m2_v2(a.m, cf_sub(b,a.p)); }
CF_INLINE CF_M3x2      cf_mul_T(CF_M3x2 a, CF_M3x2 b) { CF_M3x2 c; c.m = cf_mul_T_m2(a.m,b.m); c.p = cf_mul_T_m2_v2(a.m, cf_sub(b.p,a.p)); return c; }
CF_INLINE CF_V2        cf_mul_T(CF_Transform a, CF_V2 b) { return cf_mul_T_sc_v2(a.r, cf_sub(b,a.p)); }
CF_INLINE CF_Transform cf_mul_T(CF_Transform a, CF_Transform b) { CF_Transform c; c.r = cf_mul_T_sc(a.r,b.r); c.p = cf_mul_T_sc_v2(a.r, cf_sub(b.p,a.p)); return c; }
CF_INLINE CF_Halfspace cf_mul_T(CF_Transform a, CF_Halfspace b) { CF_Halfspace c; c.n = cf_mul_T_sc_v2(a.r,b.n); c.d = cf_dot(cf_mul_T_tf_v2(a,cf_origin(b)),c.n); return c; }
extern "C" {
#else
#define cf_mul_T(a, b) \
	_Generic((a), \
		CF_SinCos: _Generic((b), \
			CF_SinCos: cf_mul_T_sc, \
			CF_V2:     cf_mul_T_sc_v2, \
			default:   cf_mul_T_sc \
		), \
		CF_M2x2: _Generic((b), \
			CF_M2x2: cf_mul_T_m2, \
			CF_V2:   cf_mul_T_m2_v2, \
			default: cf_mul_T_m2_v2 \
		), \
		CF_M3x2: _Generic((b), \
			CF_M3x2: cf_mul_T_m32, \
			CF_V2:   cf_mul_T_m32_v2, \
			default: cf_mul_T_m32_v2 \
		), \
		CF_Transform: _Generic((b), \
			CF_Transform: cf_mul_T_tf, \
			CF_V2:        cf_mul_T_tf_v2, \
			CF_Halfspace: cf_mul_T_tf_hs, \
			default:      cf_mul_T_tf_v2 \
		), \
	default: cf_mul_T_tf_v2 \
	)((a), (b))
#endif

/**
 * @function cf_remap01
 * @category math
 * @brief    Remaps `t` from [0, 1] to [lo, hi].
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_lerp cf_remap cf_remap01 cf_mod cf_fract
 */
#define cf_remap01(t, lo, hi)
#undef cf_remap01
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_remap01(float  t, float  lo, float  hi) { return lo + t * (hi - lo); }
CF_INLINE double cf_remap01(double t, double lo, double hi) { return lo + t * (hi - lo); }
CF_INLINE CF_V2  cf_remap01(CF_V2  t, CF_V2  lo, CF_V2  hi) { return cf_add(lo, cf_mul(cf_sub(hi, lo), t)); }
extern "C" {
#else
CF_INLINE float  cf_remap01_f(float  t, float  lo, float  hi) { return lo + t * (hi - lo); }
CF_INLINE double cf_remap01_d(double t, double lo, double hi) { return lo + t * (hi - lo); }
CF_INLINE CF_V2  cf_remap01_v2(CF_V2 t, CF_V2  lo, CF_V2  hi) { return cf_add(lo, cf_mul_v2(cf_sub(hi, lo), t)); }
#define cf_remap01(t, lo, hi)   \
	_Generic((t),               \
		float:   cf_remap01_f,  \
		double:  cf_remap01_d,  \
		CF_V2:   cf_remap01_v2, \
		default: cf_remap01_f   \
	)((t), (lo), (hi))
#endif


/**
 * @function cf_mod
 * @category math
 * @brief    Returns `x` modulo `m`.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_lerp cf_remap cf_mod cf_fract
 */
#define cf_mod(x, m)
#undef cf_mod
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_mod(float  x, float  m) { return x - (int)(x / m) * m; }
CF_INLINE double cf_mod(double x, double m) { return x - (int64_t)(x / m) * m; }
CF_INLINE CF_V2  cf_mod(CF_V2  x, CF_V2  m) { return cf_sub(x, cf_mul(cf_floor(cf_div(x, m)), m)); }
extern "C" {
#else
CF_INLINE float  cf_mod_f(float  x, float  m) { return x - (int)(x / m) * m; }
CF_INLINE double cf_mod_d(double x, double m) { return x - (int64_t)(x / m) * m; }
CF_INLINE CF_V2  cf_mod_v2(CF_V2 x, CF_V2  m) { return cf_sub(x, cf_mul(cf_floor(cf_div(x, m)), m)); }
#define cf_mod(x, m)        \
	_Generic((x),           \
		float:   cf_mod_f,  \
		double:  cf_mod_d,  \
		CF_V2:   cf_mod_v2, \
		default: cf_mod_f   \
	)((x), (m))
#endif

/**
 * @function cf_lerp
 * @category math
 * @brief    Returns the linear interpolation from `a` to `b` along `t`, where `t` is _usually_ between 0.0 and 1.0.
 * @related  cf_min cf_max cf_clamp cf_clamp01 cf_sign cf_lerp cf_remap cf_mod cf_fract
 */
#define cf_lerp(a, b, t)
#undef cf_lerp
#ifdef __cplusplus
} // extern "C"
CF_INLINE float  cf_lerp(float  a, float  b, float  t) { return a + (b - a) * t; }
CF_INLINE double cf_lerp(double a, double b, double t) { return a + (b - a) * t; }
CF_INLINE CF_V2  cf_lerp(CF_V2  a, CF_V2  b, float  t) { return cf_add(a, cf_mul(cf_sub(b, a), t)); }
extern "C" {
#else
CF_INLINE float  cf_lerp_f (float  a, float  b, float  t) { return a + (b - a) * t; }
CF_INLINE double cf_lerp_d (double a, double b, double t) { return a + (b - a) * t; }
CF_INLINE CF_V2  cf_lerp(CF_V2  a, CF_V2  b, float  t) { return cf_add(a, cf_mul_v2_f(cf_sub(b, a), t)); }
#define cf_lerp(a, b, t)     \
	_Generic((a),            \
		float:   cf_lerp_f,  \
		double:  cf_lerp_d,  \
		CF_V2:   cf_lerp, \
		default: cf_lerp_f   \
	)((a), (b), (t))
#endif

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
CF_INLINE float cf_distance(CF_V2 a, CF_V2 b) { CF_V2 d = cf_sub(b, a); return CF_SQRTF(cf_dot(d, d)); }

/**
 * @function cf_norm
 * @category math
 * @brief    Returns a normalized vector.
 * @remarks  Normalized vectors have unit-length without changing the vector's direction. Fails if the vector has a length of zero.
 * @related  CF_V2 cf_len cf_distance cf_norm cf_safe_norm
 */
CF_INLINE CF_V2 cf_norm(CF_V2 a) { return cf_div(a, cf_len(a)); }

/**
 * @function cf_safe_norm
 * @category math
 * @brief    Returns a normalized vector.
 * @remarks  Sets the vector to `{ 0, 0 }` if the length of the vector is zero. Unlike `cf_norm`, this function cannot fail for
 *           the case of a zero vector.
 * @related  CF_V2 cf_len cf_distance cf_norm cf_safe_norm
 */
CF_INLINE CF_V2 cf_safe_norm(CF_V2 a) { float sq = cf_dot(a, a); return sq ? cf_div(a, CF_SQRTF(sq)) : cf_v2(0, 0); }

/**
 * @function cf_reflect
 * @category math
 * @brief    Returns a vector of equal length to `a` but with its direction reflected
 * @param    a        The vector being reflected
 * @param    n        The normal of the plane that is being reflected off of
 * @related  CF_V2 cf_neg_v2 cf_norm cf_safe_norm
 */
CF_INLINE CF_V2 cf_reflect(CF_V2 a, CF_V2 n) { return cf_sub(a, cf_mul_v2_f(n, (2.f * cf_dot(a, n)))); }

/**
 * @function cf_bezier
 * @category math
 * @brief    Returns a point along a quadratic bezier curve according to time `t`.
 * @param    a        The start point.
 * @param    c0       A control point.
 * @param    b        The end point.
 * @param    t        A position along the curve.
 * @related  CF_V2 cf_lerp cf_bezier cf_bezier2
 */
CF_INLINE CF_V2 cf_bezier(CF_V2 a, CF_V2 c0, CF_V2 b, float t) { return cf_lerp(cf_lerp(a, c0, t), cf_lerp(c0, b, t), t); }

/**
 * @function cf_bezier2
 * @category math
 * @brief    Returns a point along a cubic bezier curve according to time `t`.
 * @param    a        The start point.
 * @param    c0       A control point.
 * @param    c1       A control point.
 * @param    b        The end point.
 * @param    t        A position along the curve.
 * @related  CF_V2 cf_lerp cf_bezier cf_bezier2
 */
CF_INLINE CF_V2 cf_bezier2(CF_V2 a, CF_V2 c0, CF_V2 c1, CF_V2 b, float t) { return cf_bezier(cf_lerp(a, c0, t), cf_lerp(c0, c1, t), cf_lerp(c1, b, t), t); }

//--------------------------------------------------------------------------------------------------
// CF_SinCos rotation ops.

/**
 * @function cf_x_axis
 * @category math
 * @brief    Returns the x-axis of the 2x2 rotation matrix represented by `CF_SinCos`.
 * @related  CF_SinCos cf_sincos_f cf_x_axis cf_y_axis cf_mul_sc_v2 cf_mul_T_sc_v2 cf_mul_sc cf_mul_T_sc
 */
CF_INLINE CF_V2 cf_x_axis(CF_SinCos r) { return cf_v2(r.c, r.s); }

/**
 * @function cf_y_axis
 * @category math
 * @brief    Returns the y-axis of the 2x2 rotation matrix represented by `CF_SinCos`.
 * @related  CF_SinCos cf_sincos_f cf_x_axis cf_y_axis cf_mul_sc_v2 cf_mul_T_sc_v2 cf_mul_sc cf_mul_T_sc
 */
CF_INLINE CF_V2 cf_y_axis(CF_SinCos r) { return cf_v2(-r.s, r.c); }

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
// m3x2 ops.
// General purpose 2D graphics matrix; scale + rotate + translate.

/**
 * @function cf_make_identity
 * @category math
 * @brief    Returns an identity `CF_M3x2`.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_make_identity(void) { CF_M3x2 m; m.m.x = cf_v2(1, 0); m.m.y = cf_v2(0, 1); m.p = cf_v2(0, 0); return m; }

/**
 * @function cf_make_rotation
 * @category math
 * @brief    Returns a `CF_M3x2` that represents a rotation.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_make_rotation(float radians) { CF_SinCos sc = cf_sincos(radians); CF_M3x2 m; m.m.x = cf_v2(sc.c, -sc.s); m.m.y = cf_v2(sc.s, sc.c); m.p = cf_v2(0, 0); return m; }

/**
 * @function cf_make_transform_TSR
 * @category math
 * @brief    Returns a `CF_M3x2` that represents a translation + scale + rotation.
 * @related  CF_M3x2 cf_mul_m32_v2 cf_mul_m32 cf_make_identity cf_make_translation cf_make_scale cf_make_scale_translation cf_make_rotation cf_make_transform_TSR cf_invert
 */
CF_INLINE CF_M3x2 cf_make_transform_TSR(CF_V2 p, CF_V2 s, float radians) { CF_SinCos sc = cf_sincos(radians); CF_M3x2 m; m.m.x = cf_mul(cf_v2(sc.c, -sc.s), s.x); m.m.y = cf_mul(cf_v2(sc.s, sc.c), s.y); m.p = p; return m; }

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
 * @related  CF_Transform cf_make_transform cf_make_transform_TR cf_mul_tf_v2 cf_mul_T_tf_v2 cf_mul_tf cf_mul_T_tf
 */
CF_INLINE CF_Transform cf_make_transform(void) { CF_Transform x; x.p = cf_v2(0, 0); x.r = cf_sincos_identity(); return x; }

/**
 * @function cf_make_transform_TR
 * @category math
 * @brief    Returns a `CF_Transform` that represents a translation + rotation.
 * @related  CF_Transform cf_make_transform cf_make_transform_TR cf_mul_tf_v2 cf_mul_T_tf_v2 cf_mul_tf cf_mul_T_tf
 */
CF_INLINE CF_Transform cf_make_transform_TR(CF_V2 p, float radians) { CF_Transform x; x.r = cf_sincos(radians); x.p = p; return x; }

//--------------------------------------------------------------------------------------------------
// Halfspace (plane/line) ops.
// Functions for infinite lines.

/**
 * @function cf_plane
 * @category math
 * @brief    Constructs a halfspace (2D plane) from a normal and distance or from a normal and point.
 * @remarks  `cf_plane(n, d)` creates a plane with signed distance `d` from the origin.
 *           `cf_plane(n, p)` creates a plane passing through point `p` with normal `n`.
 * @related  cf_halfspace cf_mul cf_mul_T cf_dot
 */
#define cf_plane(a, b)
#undef cf_plane
#ifdef __cplusplus
} // extern "C"
CF_INLINE CF_Halfspace cf_plane(CF_V2 n, float d) { CF_Halfspace h; h.n = n; h.d = d; return h; }
CF_INLINE CF_Halfspace cf_plane(CF_V2 n, CF_V2 p) { CF_Halfspace h; h.n = n; h.d = cf_dot(n,p); return h; }
extern "C" {
#else
CF_INLINE CF_Halfspace cf_plane_v2_f(CF_V2 n, float d) { CF_Halfspace h; h.n = n; h.d = d; return h; }
CF_INLINE CF_Halfspace cf_plane_v2_v2(CF_V2 n, CF_V2 p) { CF_Halfspace h; h.n = n; h.d = cf_dot(n,p); return h; }
#define cf_plane(a, b)          \
	_Generic((b),               \
		float:  cf_plane_v2_f,  \
		CF_V2:  cf_plane_v2_v2, \
		default: cf_plane_v2_f  \
	)((a), (b))
#endif
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
CF_INLINE CF_Aabb cf_make_aabb_pos_w_h(CF_V2 pos, float w, float h) { CF_Aabb bb; CF_V2 he = cf_mul_v2_f(cf_v2(w, h), 0.5f); bb.min = cf_sub(pos, he); bb.max = cf_add(pos, he); return bb; }

/**
 * @function cf_make_aabb_center_half_extents
 * @category math
 * @brief    Returns an AABB (axis-aligned bounding box).
 * @remarks  Half-extents refer to half-width and height-height: `half_extents = { half_width, half_height }`.
 * @related  CF_Aabb cf_make_aabb cf_make_aabb_pos_w_h cf_make_aabb_center_half_extents cf_make_aabb_from_top_left
 */
CF_INLINE CF_Aabb cf_make_aabb_center_half_extents(CF_V2 center, CF_V2 half_extents) { CF_Aabb bb; bb.min = cf_sub(center, half_extents); bb.max = cf_add(center, half_extents); return bb; }

/**
 * @function cf_make_aabb_from_top_left
 * @category math
 * @brief    Returns an AABB (axis-aligned bounding box).
 * @related  CF_Aabb cf_make_aabb cf_make_aabb_pos_w_h cf_make_aabb_center_half_extents cf_make_aabb_from_top_left
 */
CF_INLINE CF_Aabb cf_make_aabb_from_top_left(CF_V2 top_left, float w, float h) { return cf_make_aabb(cf_add(top_left, cf_v2(0, -h)), cf_add(top_left, cf_v2(w, 0))); }

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
CF_INLINE CF_V2 cf_half_extents(CF_Aabb bb) { return (cf_mul_v2_f(cf_sub(bb.max, bb.min), 0.5f)); }

/**
 * @function cf_extents
 * @category math
 * @brief    Returns the extents of an AABB (axis-aligned bounding box).
 * @remarks  Extents refer to width and height: `extents = { width, height }`.
 * @related  CF_Aabb cf_make_aabb cf_width cf_height cf_half_width cf_half_height cf_half_extents cf_extents
 */
CF_INLINE CF_V2 cf_extents(CF_Aabb aabb) { return cf_sub(aabb.max, aabb.min); }

/**
 * @function cf_expand_aabb
 * @category math
 * @brief    Expands an AABB (axis-aligned bounding box).
 * @param    v      A vector of `{ half_width, half_height }` to expand by.
 * @related  CF_Aabb cf_make_aabb cf_expand_aabb cf_expand_aabb_f
 */
CF_INLINE CF_Aabb cf_expand_aabb(CF_Aabb aabb, CF_V2 v) { return cf_make_aabb(cf_sub(aabb.min, v), cf_add(aabb.max, v)); }

/**
 * @function cf_expand_aabb_f
 * @category math
 * @brief    Expands an AABB (axis-aligned bounding box).
 * @remarks  `v` is added to to `max.x` and `max.y` of `aabb`, and subtracted from `min.x` and `min.y` of `aabb`.
 * @related  CF_Aabb cf_make_aabb cf_expand_aabb cf_expand_aabb_f
 */
CF_INLINE CF_Aabb cf_expand_aabb_f(CF_Aabb aabb, float v) { CF_V2 factor = cf_v2(v, v); return cf_make_aabb(cf_sub(aabb.min, factor), cf_add(aabb.max, factor)); }

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
CF_INLINE CF_V2 cf_midpoint(CF_Aabb bb) { return cf_mul_v2_f(cf_add(bb.min, bb.max), 0.5f); }

/**
 * @function cf_center
 * @category math
 * @brief    Returns the center of `bb`.
 * @related  CF_Aabb cf_min_aabb cf_max_aabb cf_midpoint cf_center cf_top_left cf_top_right cf_bottom_left cf_bottom_right
 */
CF_INLINE CF_V2 cf_center(CF_Aabb bb) { return cf_mul_v2_f(cf_add(bb.min, bb.max), 0.5f); }

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
CF_INLINE bool cf_contains_point(CF_Aabb bb, CF_V2 p) { return cf_greater_equal(p, bb.min) && cf_lesser_equal(p, bb.max); }

/**
 * @function cf_contains_aabb
 * @category math
 * @brief    Returns true if `a` is _fully_ contained within `b`.
 * @related  CF_Aabb cf_contains_point cf_contains_aabb cf_surface_area_aabb cf_area_aabb cf_clamp_aabb_v2 cf_clamp_aabb cf_combine cf_overlaps
 */
CF_INLINE bool cf_contains_aabb(CF_Aabb a, CF_Aabb b) { return cf_lesser_equal(a.min, b.min) && cf_greater_equal(a.max, b.max); }

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
CF_INLINE CF_V2 cf_clamp_aabb_v2(CF_Aabb bb, CF_V2 p) { return cf_clamp(p, bb.min, bb.max); }

/**
 * @function cf_clamp_aabb
 * @category math
 * @brief    Returns `a` clamped within `b`.
 * @related  CF_Aabb cf_contains_point cf_contains_aabb cf_surface_area_aabb cf_area_aabb cf_clamp_aabb_v2 cf_clamp_aabb cf_combine cf_overlaps
 */
CF_INLINE CF_Aabb cf_clamp_aabb(CF_Aabb a, CF_Aabb b) { return cf_make_aabb(cf_clamp(a.min, b.min, b.max), cf_clamp(a.max, b.min, b.max)); }

/**
 * @function cf_combine
 * @category math
 * @brief    Returns a `CF_Aabb` that tightly contains both `a` and `b`.
 * @related  CF_Aabb cf_contains_point cf_contains_aabb cf_surface_area_aabb cf_area_aabb cf_clamp_aabb_v2 cf_clamp_aabb cf_combine cf_overlaps
 */
CF_INLINE CF_Aabb cf_combine(CF_Aabb a, CF_Aabb b) { return cf_make_aabb(cf_min(a.min, b.min), cf_max(a.max, b.max)); }

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
CF_INLINE CF_Aabb cf_make_aabb_verts(const CF_V2* verts, int count)
{
	CF_V2 vmin = verts[0];
	CF_V2 vmax = vmin;
	for (int i = 0; i < count; ++i) {
		vmin = cf_min(vmin, verts[i]);
		vmax = cf_max(vmax, verts[i]);
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
CF_INLINE CF_V2 cf_impact(CF_Ray r, float t) { return cf_add(r.p, cf_mul_v2_f(r.d, t)); }

/**
 * @function cf_endpoint
 * @category collision
 * @brief    Returns the endpoint of a ray.
 * @remarks  Rays are defined to have an endpoint as an optimization. Usually infinite rays are not needed in games, and cause
 *           unnecessarily large computations when doing raycasts.
 * @related  CF_Ray cf_impact cf_endpoint
 */
CF_INLINE CF_V2 cf_endpoint(CF_Ray r) { return cf_add(r.p, cf_mul_v2_f(r.d, r.t)); }

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
	CF_V2 n = cf_sub(b, a);
	CF_V2 pa = cf_sub(a, p);
	float c = cf_dot(n, pa);

	// Closest point is a
	if (c > 0.0f) return cf_dot(pa, pa);

	// Closest point is b
	CF_V2 bp = cf_sub(p, b);
	if (cf_dot(n, bp) > 0.0f) return cf_dot(bp, bp);

	// Closest point is between a and b
	CF_V2 e = cf_sub(pa, cf_mul_v2_f(n, (c / cf_dot(n, n))));
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

CF_INLINE int sign(int a) { return cf_sign(a); }
CF_INLINE int min(int a, int b) { return cf_min(a, b); }
CF_INLINE int max(int a, int b) { return cf_max(a, b); }
CF_INLINE uint64_t min(uint64_t a, uint64_t b) { return cf_min(a, b); }
CF_INLINE uint64_t max(uint64_t a, uint64_t b) { return cf_max(a, b); }
CF_INLINE float abs(float a) { return cf_abs(a); }
CF_INLINE int abs(int a) { return cf_abs(a); }
CF_INLINE int clamp(int a, int lo, int hi) { return cf_clamp(a, lo, hi); }
CF_INLINE int clamp01(int a) { return cf_clamp01(a); }
CF_INLINE bool is_even(int x) { return cf_is_even(x); }
CF_INLINE bool is_odd(int x) { return cf_is_odd(x); }

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
CF_INLINE v2 cross(v2 a, float b) { return cf_cross(a, b); }
CF_INLINE v2 cross(float a, v2 b) { return cf_cross(a, b); }
CF_INLINE v2 min(v2 a, v2 b) { return cf_min(a, b); }
CF_INLINE v2 max(v2 a, v2 b) { return cf_max(a, b); }
CF_INLINE v2 clamp(v2 a, v2 lo, v2 hi) { return cf_clamp(a, lo, hi); }
CF_INLINE v2 clamp01(v2 a) { return cf_clamp01(a); }
CF_INLINE v2 abs(v2 a) { return cf_abs(a); }
CF_INLINE float hmin(v2 a) { return cf_hmin(a); }
CF_INLINE float hmax(v2 a) { return cf_hmax(a); }
CF_INLINE float len(v2 a) { return cf_len(a); }
CF_INLINE float len_sq(v2 a) { return cf_len_sq(a); }
CF_INLINE float distance(v2 a, v2 b) { return cf_distance(a, b); }
CF_INLINE v2 norm(v2 a) { return cf_norm(a); }
CF_INLINE v2 safe_norm(v2 a) { return cf_safe_norm(a); }
CF_INLINE v2 reflect(v2 a, v2 n) { return cf_reflect(a, n); }

CF_INLINE v2 lerp(v2 a, v2 b, float t) { return cf_lerp(a, b, t); }
CF_INLINE v2 bezier(v2 a, v2 c0, v2 b, float t) { return cf_bezier(a, c0, b, t); }
CF_INLINE v2 bezier(v2 a, v2 c0, v2 c1, v2 b, float t) { return cf_bezier2(a, c0, c1, b, t); }
CF_INLINE v2 floor(v2 a) { return cf_floor(a); }
CF_INLINE v2 round(v2 a) { return cf_round(a); }
CF_INLINE v2 safe_invert(v2 a) { return cf_safe_invert(a); }
CF_INLINE v2 sign(v2 a) { return cf_sign(a); }

CF_INLINE CF_SinCos sincos(float radians) { return cf_sincos(radians); }
CF_INLINE CF_SinCos sincos() { return cf_sincos(); }
CF_INLINE v2 x_axis(CF_SinCos r) { return cf_x_axis(r); }
CF_INLINE v2 y_axis(CF_SinCos r) { return cf_y_axis(r); }
CF_INLINE v2 mul(CF_SinCos a, v2 b) { return cf_mul_sc_v2(a, b); }
CF_INLINE v2 mul_T(CF_SinCos a, v2 b) { return cf_mul_T_sc_v2(a, b); }
CF_INLINE CF_SinCos mul(CF_SinCos a, CF_SinCos b) { return cf_mul_sc(a, b); }
CF_INLINE CF_SinCos mul_T(CF_SinCos a, CF_SinCos b) { return cf_mul_T_sc(a, b); }

CF_INLINE float atan2_360(float y, float x) { return cf_atan2_360(y, x); }
CF_INLINE float atan2_360(v2 v) { return cf_atan2_360(v); }
CF_INLINE float atan2_360(CF_SinCos r) { return cf_atan2_360(r); }

CF_INLINE float shortest_arc(v2 a, v2 b) { return cf_shortest_arc(a, b); }

CF_INLINE float angle_diff(float radians_a, float radians_b) { return cf_angle_diff(radians_a, radians_b); }
CF_INLINE v2 from_angle(float radians) { return cf_from_angle(radians); }

CF_INLINE v2 mul(CF_M2x2 a, v2 b) { return cf_mul_m2_v2(a, b); }
CF_INLINE CF_M2x2 mul(CF_M2x2 a, CF_M2x2 b) { return cf_mul_m2(a, b); }

CF_INLINE v2 mul(CF_M3x2 a, v2 b) { return cf_mul_m32_v2(a, b); }
CF_INLINE CF_M3x2 mul(CF_M3x2 a, CF_M3x2 b) { return cf_mul_m32(a, b); }
CF_INLINE CF_M3x2 make_identity() { return cf_make_identity(); }
CF_INLINE CF_M3x2 make_translation(float x, float y) { return cf_make_translation(x, y); }
CF_INLINE CF_M3x2 make_translation(v2 p) { return cf_make_translation(p); }
CF_INLINE CF_M3x2 make_scale(v2 s) { return cf_make_scale(s); }
CF_INLINE CF_M3x2 make_scale(float s) { return cf_make_scale(s); }
CF_INLINE CF_M3x2 make_scale(float sx, float sy) { return cf_make_scale(cf_v2(sx, sy)); }
CF_INLINE CF_M3x2 make_scale(v2 s, v2 p) { return cf_make_scale_translation(s, p); }
CF_INLINE CF_M3x2 make_scale(float s, v2 p) { return cf_make_scale_translation(s, p); }
CF_INLINE CF_M3x2 make_scale_translation(float sx, float sy, v2 p) { return cf_make_scale_translation(sx, sy, p); }
CF_INLINE CF_M3x2 make_rotation(float radians) { return cf_make_rotation(radians); }
CF_INLINE CF_M3x2 make_transform(v2 p, v2 s, float radians) { return cf_make_transform_TSR(p, s, radians); }
CF_INLINE CF_M3x2 invert(CF_M3x2 m) { return cf_invert(m); }
CF_INLINE CF_M3x2 ortho_2d(float x, float y, float scale_x, float scale_y) { return cf_ortho_2d(x, y, scale_x, scale_y); }

CF_INLINE CF_Transform make_transform() { return cf_make_transform(); }
CF_INLINE CF_Transform make_transform(v2 p, float radians) { return cf_make_transform_TR(p, radians); }
CF_INLINE v2 mul(CF_Transform a, v2 b) { return cf_mul_tf_v2(a, b); }
CF_INLINE v2 mul_T(CF_Transform a, v2 b) { return cf_mul_T_tf_v2(a, b); }
CF_INLINE CF_Transform mul(CF_Transform a, CF_Transform b) { return cf_mul_tf(a, b); }
CF_INLINE CF_Transform mul_T(CF_Transform a, CF_Transform b) { return cf_mul_T_tf(a, b); }

CF_INLINE CF_Halfspace plane(v2 n, float d) { return cf_plane(n, d); }
CF_INLINE CF_Halfspace plane(v2 n, v2 p) { return cf_plane(n, p); }
CF_INLINE v2 origin(CF_Halfspace h) { return cf_origin(h); }
CF_INLINE float distance(CF_Halfspace h, v2 p) { return cf_distance_hs(h, p); }
CF_INLINE v2 project(CF_Halfspace h, v2 p) { return cf_project(h, p); }
CF_INLINE CF_Halfspace mul(CF_Transform a, CF_Halfspace b) { return cf_mul_tf_hs(a, b); }
CF_INLINE CF_Halfspace mul_T(CF_Transform a, CF_Halfspace b) { return cf_mul_T_tf_hs(a, b); }
CF_INLINE v2 intersect(v2 a, v2 b, float da, float db) { return cf_intersect_halfspace(a, b, da, db); }
CF_INLINE v2 intersect(CF_Halfspace h, v2 a, v2 b) { return cf_intersect_halfspace2(h, a, b); }
CF_INLINE v2 intersect(CF_Halfspace a, CF_Halfspace b) { return cf_intersect_halfspace3(a, b); }

CF_INLINE CF_Circle make_circle(v2 pos, float radius) { return cf_make_circle(pos, radius); }
CF_INLINE CF_Capsule make_capsule(v2 a, v2 b, float radius) { return cf_make_capsule(a, b, radius); }
CF_INLINE CF_Capsule make_capsule(v2 p, float height, float radius) { return cf_make_capsule2(p, height, radius); }
CF_INLINE CF_Aabb make_aabb(v2 min, v2 max) { return cf_make_aabb(min, max); }
CF_INLINE CF_Aabb make_aabb(v2 pos, float w, float h) { return cf_make_aabb_pos_w_h(pos, w, h); }
CF_INLINE CF_Aabb make_aabb_center_half_extents(v2 center, v2 half_extents) { return cf_make_aabb_center_half_extents(center, half_extents); }
CF_INLINE CF_Aabb make_aabb_from_top_left(v2 top_left, float w, float h) { return cf_make_aabb_from_top_left(top_left, w, h); }
CF_INLINE float width(CF_Aabb bb) { return cf_width(bb); }
CF_INLINE float height(CF_Aabb bb) { return cf_height(bb); }
CF_INLINE float half_width(CF_Aabb bb) { return cf_half_width(bb); }
CF_INLINE float half_height(CF_Aabb bb) { return cf_half_height(bb); }
CF_INLINE v2 half_extents(CF_Aabb bb) { return cf_half_extents(bb); }
CF_INLINE v2 extents(CF_Aabb aabb) { return cf_extents(aabb); }
CF_INLINE CF_Aabb expand(CF_Aabb aabb, v2 v) { return cf_expand_aabb(aabb, v); }
CF_INLINE CF_Aabb expand(CF_Aabb aabb, float v) { return cf_expand_aabb_f(aabb, v); }
CF_INLINE v2 min(CF_Aabb bb) { return cf_min_aabb(bb); }
CF_INLINE v2 max(CF_Aabb bb) { return cf_max_aabb(bb); }
CF_INLINE v2 midpoint(CF_Aabb bb) { return cf_midpoint(bb); }
CF_INLINE v2 center(CF_Aabb bb) { return cf_center(bb); }
CF_INLINE v2 top_left(CF_Aabb bb) { return cf_top_left(bb); }
CF_INLINE v2 top_right(CF_Aabb bb) { return cf_top_right(bb); }
CF_INLINE v2 bottom_left(CF_Aabb bb) { return cf_bottom_left(bb); }
CF_INLINE v2 bottom_right(CF_Aabb bb) { return cf_bottom_right(bb); }
CF_INLINE v2 top(CF_Aabb bb) { return cf_top(bb); }
CF_INLINE v2 left(CF_Aabb bb) { return cf_left(bb); }
CF_INLINE v2 bottom(CF_Aabb bb) { return cf_bottom(bb); }
CF_INLINE v2 right(CF_Aabb bb) { return cf_right(bb); }
CF_INLINE bool contains(CF_Aabb bb, v2 p) { return cf_contains_point(bb, p); }
CF_INLINE bool contains(CF_Aabb a, CF_Aabb b) { return cf_contains_aabb(a, b); }
CF_INLINE float surface_area(CF_Aabb bb) { return cf_surface_area_aabb(bb); }
CF_INLINE float area(CF_Aabb bb) { return cf_area_aabb(bb); }
CF_INLINE v2 clamp(CF_Aabb bb, v2 p) { return cf_clamp_aabb_v2(bb, p); }
CF_INLINE CF_Aabb clamp(CF_Aabb a, CF_Aabb b) { return cf_clamp_aabb(a, b); }
CF_INLINE CF_Aabb combine(CF_Aabb a, CF_Aabb b) { return cf_combine(a, b); }

CF_INLINE int overlaps(CF_Aabb a, CF_Aabb b) { return cf_overlaps(a, b); }
CF_INLINE int collide(CF_Aabb a, CF_Aabb b) { return cf_collide_aabb(a, b); }

CF_INLINE CF_Aabb make_aabb(const v2* verts, int count) { return cf_make_aabb_verts((const v2*)verts, count); }
CF_INLINE void aabb_verts(v2* out, CF_Aabb bb) { return cf_aabb_verts((v2*)out, bb); }

CF_INLINE float area(CF_Circle c) { return cf_area_circle(c); }
CF_INLINE float surface_area(CF_Circle c) { return cf_surface_area_circle(c); }
CF_INLINE CF_Circle mul(CF_Transform tx, CF_Circle a) { return cf_mul_tf_circle(tx, a); }

CF_INLINE CF_Ray make_ray(v2 start, v2 direction_normalized, float length) { return cf_make_ray(start, direction_normalized, length); }
CF_INLINE v2 impact(CF_Ray r, float t) { return cf_impact(r, t); }
CF_INLINE v2 endpoint(CF_Ray r) { return cf_endpoint(r); }

CF_INLINE CF_Raycast ray_to_halfspace(CF_Ray A, CF_Halfspace B) { return cf_ray_to_halfspace(A, B); }
CF_INLINE float distance_sq(v2 a, v2 b, v2 p) { return cf_distance_sq(a, b, p); }

CF_INLINE v2 center_of_mass(CF_Poly poly) { return cf_center_of_mass(poly); }
CF_INLINE float calc_area(CF_Poly poly) { return cf_calc_area(poly); }
CF_INLINE CF_SliceOutput slice(CF_Halfspace slice_plane, CF_Poly slice_me, const float k_epsilon = 1.e-4f) { return cf_slice(slice_plane, slice_me, k_epsilon); }
CF_INLINE void inflate(void* shape, CF_ShapeType type, float skin_factor) { return cf_inflate(shape, type, skin_factor); }
CF_INLINE int hull(v2* verts, int count) { return cf_hull((v2*)verts, count); }
CF_INLINE void norms(v2* verts, v2* norms, int count) { return cf_norms((v2*)verts, (v2*)norms, count); }
CF_INLINE void make_poly(CF_Poly* p) { return cf_make_poly(p); }
CF_INLINE v2 centroid(const v2* verts, int count) { return cf_centroid((v2*)verts, count); }

CF_INLINE bool circle_to_circle(CF_Circle A, CF_Circle B) { return cf_circle_to_circle(A, B); }
CF_INLINE bool circle_to_aabb(CF_Circle A, CF_Aabb B) { return cf_circle_to_aabb(A, B); }
CF_INLINE bool circle_to_capsule(CF_Circle A, CF_Capsule B) { return cf_circle_to_capsule(A, B); }
CF_INLINE bool aabb_to_aabb(CF_Aabb A, CF_Aabb B) { return cf_aabb_to_aabb(A, B); }
CF_INLINE bool aabb_to_capsule(CF_Aabb A, CF_Capsule B) { return cf_aabb_to_capsule(A, B); }
CF_INLINE bool capsule_to_capsule(CF_Capsule A, CF_Capsule B) { return cf_capsule_to_capsule(A, B); }
CF_INLINE bool circle_to_poly(CF_Circle A, const CF_Poly* B, const CF_Transform* bx) { return cf_circle_to_poly(A, B, bx); }
CF_INLINE bool aabb_to_poly(CF_Aabb A, const CF_Poly* B, const CF_Transform* bx) { return cf_aabb_to_poly(A, B, bx); }
CF_INLINE bool capsule_to_poly(CF_Capsule A, const CF_Poly* B, const CF_Transform* bx) { return cf_capsule_to_poly(A, B, bx); }
CF_INLINE bool poly_to_poly(const CF_Poly* A, const CF_Transform* ax, const CF_Poly* B, const CF_Transform* bx) { return cf_poly_to_poly(A, ax, B, bx); }

CF_INLINE CF_Raycast ray_to_circle(CF_Ray A, CF_Circle B) { return cf_ray_to_circle(A, B); }
CF_INLINE CF_Raycast ray_to_aabb(CF_Ray A, CF_Aabb B) { return cf_ray_to_aabb(A, B); }
CF_INLINE CF_Raycast ray_to_capsule(CF_Ray A, CF_Capsule B) { return cf_ray_to_capsule(A, B); }
CF_INLINE CF_Raycast ray_to_poly(CF_Ray A, const CF_Poly* B, const CF_Transform* bx_ptr = NULL) { return cf_ray_to_poly(A, B, bx_ptr); }

CF_INLINE CF_Manifold circle_to_circle_manifold(CF_Circle A, CF_Circle B) { return cf_circle_to_circle_manifold(A, B); }
CF_INLINE CF_Manifold circle_to_aabb_manifold(CF_Circle A, CF_Aabb B) { return cf_circle_to_aabb_manifold(A, B); }
CF_INLINE CF_Manifold circle_to_capsule_manifold(CF_Circle A, CF_Capsule B) { return cf_circle_to_capsule_manifold(A, B); }
CF_INLINE CF_Manifold aabb_to_aabb_manifold(CF_Aabb A, CF_Aabb B) { return cf_aabb_to_aabb_manifold(A, B); }
CF_INLINE CF_Manifold aabb_to_capsule_manifold(CF_Aabb A, CF_Capsule B) { return cf_aabb_to_capsule_manifold(A, B); }
CF_INLINE CF_Manifold capsule_to_capsule_manifold(CF_Capsule A, CF_Capsule B) { return cf_capsule_to_capsule_manifold(A, B); }
CF_INLINE CF_Manifold circle_to_poly_manifold(CF_Circle A, const CF_Poly* B, const CF_Transform* bx) { return cf_circle_to_poly_manifold(A, B, bx); }
CF_INLINE CF_Manifold aabb_to_poly_manifold(CF_Aabb A, const CF_Poly* B, const CF_Transform* bx) { return cf_aabb_to_poly_manifold(A, B, bx); }
CF_INLINE CF_Manifold capsule_to_poly_manifold(CF_Capsule A, const CF_Poly* B, const CF_Transform* bx) { return cf_capsule_to_poly_manifold(A, B, bx); }
CF_INLINE CF_Manifold poly_to_poly_manifold(const CF_Poly* A, const CF_Transform* ax, const CF_Poly* B, const CF_Transform* bx) { return cf_poly_to_poly_manifold(A, ax, B, bx); }

CF_INLINE float gjk(const void* A, CF_ShapeType typeA, const CF_Transform* ax_ptr, const void* B, CF_ShapeType typeB, const CF_Transform* bx_ptr, v2* outA, v2* outB, int use_radius, int* iterations, CF_GjkCache* cache)
{
	return cf_gjk(A, typeA, ax_ptr, B, typeB, bx_ptr, (CF_V2*)outA, (CF_V2*)outB, use_radius, iterations, cache);
}

CF_INLINE CF_ToiResult toi(const void* A, CF_ShapeType typeA, const CF_Transform* ax_ptr, v2 vA, const void* B, CF_ShapeType typeB, const CF_Transform* bx_ptr, v2 vB, int use_radius)
{
	return cf_toi(A, typeA, ax_ptr, vA, B, typeB, bx_ptr, vB, use_radius);
}

CF_INLINE int collided(const void* A, const CF_Transform* ax, CF_ShapeType typeA, const void* B, const CF_Transform* bx, CF_ShapeType typeB) { return cf_collided(A, ax, typeA, B, bx, typeB); }
CF_INLINE void collide(const void* A, const CF_Transform* ax, CF_ShapeType typeA, const void* B, const CF_Transform* bx, CF_ShapeType typeB, CF_Manifold* m) { return cf_collide(A, ax, typeA, B, bx, typeB, m); }
CF_INLINE bool cast_ray(CF_Ray A, const void* B, const CF_Transform* bx, CF_ShapeType typeB, CF_Raycast* out) { return cf_cast_ray(A, B, bx, typeB, out); }

}

CF_INLINE Cute::v2 operator+(Cute::v2 a, Cute::v2 b) { return V2(a.x + b.x, a.y + b.y); }
CF_INLINE Cute::v2 operator-(Cute::v2 a, Cute::v2 b) { return V2(a.x - b.x, a.y - b.y); }
CF_INLINE Cute::v2& operator+=(Cute::v2& a, Cute::v2 b) { return a = a + b; }
CF_INLINE Cute::v2& operator-=(Cute::v2& a, Cute::v2 b) { return a = a - b; }
CF_INLINE Cute::v2 operator*(Cute::v2 a, float b) { return V2(a.x * b, a.y * b); }
CF_INLINE Cute::v2 operator*(float a, Cute::v2 b) { return V2(a * b.x, a * b.y); }
CF_INLINE Cute::v2 operator*(Cute::v2 a, Cute::v2 b) { return V2(a.x * b.x, a.y * b.y); }
CF_INLINE Cute::v2& operator*=(Cute::v2& a, float b) { return a = a * b; }
CF_INLINE Cute::v2& operator*=(float a, Cute::v2& b) { return b = a * b; }
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
CF_INLINE bool operator==(CF_Rect a, CF_Rect b) { return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h; }

#endif // CF_CPP

#endif // CF_MATH_H
