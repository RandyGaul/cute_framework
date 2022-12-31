/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#ifndef CUTE_MATH_H
#define CUTE_MATH_H

#include "cute_defines.h"

#include <math.h>

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// 2d vector.
typedef struct CF_V2
{
	float x;
	float y;
} CF_V2;

// Use this to create a v2 struct.
// The C++ API uses V2(x, y).
CUTE_INLINE CF_V2 cf_v2(float x, float y)
{
	CF_V2 result;
	result.x = x;
	result.y = y;
	return result;
}

// Rotation about an axis composed of cos/sin pair.
typedef struct CF_SinCos
{
	float s;
	float c;
} CF_SinCos;

// 2x2 matrix.
typedef struct CF_M2x2
{
	CF_V2 x;
	CF_V2 y;
} CF_M2x2;

// 2d transformation, mostly useful for graphics and not physics colliders, since it supports scale.
typedef struct CF_M3x2
{
	CF_M2x2 m;
	CF_V2 p;
} CF_M3x2;


// 2d transformation, mostly useful for physics colliders since there's no scale.
typedef struct CF_Transform
{
	CF_SinCos r;
	CF_V2 p;
} CF_Transform;

// 2d plane, aka line.
typedef struct CF_Halfspace
{
	CF_V2 n; // normal
	float d; // distance to origin; d = ax + by = dot(n, p)
} CF_Halfspace;

// A ray is a directional line segment. It starts at an endpoint and extends into another direction
// for a specified distance (defined by t).
typedef struct CF_Ray
{
	CF_V2 p; // position
	CF_V2 d; // direction (normalized)
	float t; // distance along d from position p to find endpoint of ray
} CF_Ray;

// The results for a raycast query.
typedef struct CF_Raycast
{
	float t; // time of impact
	CF_V2 n; // normal of surface at impact (unit length)
} CF_Raycast;

typedef struct CF_Circle
{
	CF_V2 p;
	float r;
} CF_Circle;

// Axis-aligned bounding box. A box that cannot rotate.
typedef struct CF_Aabb
{
	CF_V2 min;
	CF_V2 max;
} CF_Aabb;

// Box that cannot rotate defined with integers instead of floats. Not used for collision detection,
// but still sometimes useful.
typedef struct CF_Rect
{
	int w, h, x, y;
} CF_Rect;

#define CUTE_PI 3.14159265f

//--------------------------------------------------------------------------------------------------
// Scalar float ops.

CUTE_INLINE float cf_min(float a, float b) { return a < b ? a : b; }
CUTE_INLINE float cf_max(float a, float b) { return b < a ? a : b; }
CUTE_INLINE float cf_clamp(float a, float lo, float hi) { return cf_max(lo, cf_min(a, hi)); }
CUTE_INLINE float cf_clamp01(float a) { return cf_max(0.0f, cf_min(a, 1.0f)); }
CUTE_INLINE float cf_sign(float a) { return a < 0 ? -1.0f : 1.0f; }
CUTE_INLINE float cf_intersect(float da, float db) { return da / (da - db); }
CUTE_INLINE float cf_safe_invert(float a) { return a != 0 ? 1.0f / a : 0; }
CUTE_INLINE float cf_lerp(float a, float b, float t) { return a + (b - a) * t; }
CUTE_INLINE float cf_remap(float t, float lo, float hi) { return (hi - lo) != 0 ? (t - lo) / (hi - lo) : 0; }
CUTE_INLINE float cf_mod(float x, float m) { return x - (int)(x / m) * m; }
CUTE_INLINE float cf_fract(float x) { return x - floorf(x); }

CUTE_INLINE int cf_sign_int(int a) { return a < 0 ? -1 : 1; }
#define cf_min(a, b) ((a) < (b) ? (a) : (b))
#define cf_max(a, b) ((b) < (a) ? (a) : (b))
CUTE_INLINE float cf_abs(float a) { return fabsf(a); }
CUTE_INLINE int cf_abs_int(int a) { int mask = a >> ((sizeof(int) * 8) - 1); return (a + mask) ^ mask; }
CUTE_INLINE int cf_clamp_int(int a, int lo, int hi) { return cf_max(lo, cf_min(a, hi)); }
CUTE_INLINE int cf_clamp01_int(int a) { return cf_max(0, cf_min(a, 1)); }
CUTE_INLINE bool cf_is_even(int x) { return (x % 2) == 0; }
CUTE_INLINE bool cf_is_odd(int x) { return !cf_is_even(x); }

//--------------------------------------------------------------------------------------------------
// Bit manipulation.

CUTE_INLINE bool cf_is_power_of_two(int a) { return a != 0 && (a & (a - 1)) == 0; }
CUTE_INLINE bool cf_is_power_of_two_uint(uint64_t a) { return a != 0 && (a & (a - 1)) == 0; }
CUTE_INLINE int cf_fit_power_of_two(int a) { a--; a |= a >> 1; a |= a >> 2; a |= a >> 4; a |= a >> 8; a |= a >> 16; a++; return a; }

//--------------------------------------------------------------------------------------------------
// Easing functions.
// Adapted from Noel Berry: https://github.com/NoelFB/blah/blob/master/include/blah_ease.h

CUTE_INLINE float cf_smoothstep(float x) { return x * x * (3.0f - 2.0f * x); }
CUTE_INLINE float cf_quad_in(float x) { return x * x; }
CUTE_INLINE float cf_quad_out(float x) { return -(x * (x - 2.0f)); }
CUTE_INLINE float cf_quad_in_out(float x) { if (x < 0.5f) return 2.0f * x * x; else return (-2.0f * x * x) + (4.0f * x) - 1.0f; }
CUTE_INLINE float cf_cube_in(float x) { return x * x * x; }
CUTE_INLINE float cf_cube_out(float x) { float f = (x - 1); return f * f * f + 1.0f; }
CUTE_INLINE float cf_cube_in_out(float x) { if (x < 0.5f) return 4.0f * x * x * x; else { float f = ((2.0f * x) - 2.0f); return 0.5f * x * x * x + 1.0f; } }
CUTE_INLINE float cf_quart_in(float x) { return x * x * x * x; }
CUTE_INLINE float cf_quart_out(float x) { float f = (x - 1.0f); return f * f * f * (1.0f - x) + 1.0f; }
CUTE_INLINE float cf_quart_in_out(float x) { if (x < 0.5f) return 8.0f * x * x * x * x; else { float f = (x - 1); return -8.0f * f * f * f * f + 1.0f; } }
CUTE_INLINE float cf_quint_in(float x) { return x * x * x * x * x; }
CUTE_INLINE float cf_quint_out(float x) { float f = (x - 1); return f * f * f * f * f + 1.0f; }
CUTE_INLINE float cf_quint_in_out(float x) { if (x < 0.5f) return 16.0f * x * x * x * x * x; else { float f = ((2.0f * x) - 2.0f); return  0.5f * f * f * f * f * f + 1.0f; } }
CUTE_INLINE float cf_sin_in(float x) { return sinf((x - 1.0f) * CUTE_PI * 0.5f) + 1.0f; }
CUTE_INLINE float cf_sin_out(float x) { return sinf(x * (CUTE_PI * 0.5f)); }
CUTE_INLINE float cf_sin_in_out(float x) { return 0.5f * (1.0f - cosf(x * CUTE_PI)); }
CUTE_INLINE float cf_circle_in(float x) { return 1.0f - sqrtf(1.0f - (x * x)); }
CUTE_INLINE float cf_circle_out(float x) { return sqrtf((2.0f - x) * x); }
CUTE_INLINE float cf_circle_in_out(float x) { if (x < 0.5f) return 0.5f * (1.0f - sqrtf(1.0f - 4.0f * (x * x))); else return 0.5f * (sqrtf(-((2.0f * x) - 3.0f) * ((2.0f * x) - 1.0f)) + 1.0f); }
CUTE_INLINE float cf_back_in(float x) { return x * x * x - x * sinf(x * CUTE_PI); }
CUTE_INLINE float cf_back_out(float x) { float f = (1.0f - x); return 1.0f - (x * x * x - x * sinf(f * CUTE_PI)); }
CUTE_INLINE float cf_back_in_out(float x) { if (x < 0.5f) { float f = 2.0f * x; return 0.5f * (f * f * f - f * sinf(f * CUTE_PI)); } else { float f = (1.0f - (2.0f * x - 1.0f)); return 0.5f * (1.0f - (f * f * f - f * sinf(f * CUTE_PI))) + 0.5f; } }

//--------------------------------------------------------------------------------------------------
// 2D vector ops.

CUTE_INLINE CF_V2 cf_add_v2(CF_V2 a, CF_V2 b) { return cf_v2(a.x + b.x, a.y + b.y); }
CUTE_INLINE CF_V2 cf_sub_v2(CF_V2 a, CF_V2 b) { return cf_v2(a.x - b.x, a.y - b.y); }

CUTE_INLINE float cf_dot(CF_V2 a, CF_V2 b) { return a.x * b.x + a.y * b.y; }

CUTE_INLINE CF_V2 cf_mul_v2_f(CF_V2 a, float b) { return cf_v2(a.x * b, a.y * b); }
CUTE_INLINE CF_V2 cf_mul_v2(CF_V2 a, CF_V2 b) { return cf_v2(a.x * b.x, a.y * b.y); }
CUTE_INLINE CF_V2 cf_div_v2_f(CF_V2 a, float b) { return cf_v2(a.x / b, a.y / b); }

CUTE_INLINE CF_V2 cf_skew(CF_V2 a) { return cf_v2(-a.y, a.x); }
CUTE_INLINE CF_V2 cf_cw90(CF_V2 a) { return cf_v2(a.y, -a.x); }
CUTE_INLINE float cf_det2(CF_V2 a, CF_V2 b) { return a.x * b.y - a.y * b.x; }
CUTE_INLINE float cf_cross(CF_V2 a, CF_V2 b) { return cf_det2(a, b); }
CUTE_INLINE CF_V2 cf_cross_v2_f(CF_V2 a, float b) { return cf_v2(b * a.y, -b * a.x); }
CUTE_INLINE CF_V2 cf_cross_f_v2(float a, CF_V2 b) { return cf_v2(-a * b.y, a * b.x); }
CUTE_INLINE CF_V2 cf_min_v2(CF_V2 a, CF_V2 b) { return cf_v2(cf_min(a.x, b.x), cf_min(a.y, b.y)); }
CUTE_INLINE CF_V2 cf_max_v2(CF_V2 a, CF_V2 b) { return cf_v2(cf_max(a.x, b.x), cf_max(a.y, b.y)); }
CUTE_INLINE CF_V2 cf_clamp_v2(CF_V2 a, CF_V2 lo, CF_V2 hi) { return cf_max_v2(lo, cf_min_v2(a, hi)); }
CUTE_INLINE CF_V2 cf_clamp01_v2(CF_V2 a) { return cf_max_v2(cf_v2(0, 0), cf_min_v2(a, cf_v2(1, 1))); }
CUTE_INLINE CF_V2 cf_abs_v2(CF_V2 a) { return cf_v2(fabsf(a.x), fabsf(a.y)); }
CUTE_INLINE float cf_hmin(CF_V2 a) { return cf_min(a.x, a.y); }
CUTE_INLINE float cf_hmax(CF_V2 a) { return cf_max(a.x, a.y); }
CUTE_INLINE float cf_len(CF_V2 a) { return sqrtf(cf_dot(a, a)); }
CUTE_INLINE float cf_length(CF_V2 a) { return sqrtf(cf_dot(a, a)); }
CUTE_INLINE float cf_distance(CF_V2 a, CF_V2 b) { CF_V2 d = cf_sub_v2(b, a); return sqrtf(cf_dot(d, d)); }
CUTE_INLINE CF_V2 cf_norm(CF_V2 a) { return cf_div_v2_f(a, cf_len(a)); }
CUTE_INLINE CF_V2 cf_safe_norm(CF_V2 a) { float sq = cf_dot(a, a); return sq ? cf_div_v2_f(a, sqrtf(sq)) : cf_v2(0, 0); }
CUTE_INLINE float cf_safe_norm_f(float a) { return a == 0 ? 0 : cf_sign(a); }
CUTE_INLINE int cf_safe_norm_int(int a) { return a == 0 ? 0 : cf_sign_int(a); }
CUTE_INLINE CF_V2 cf_neg_v2(CF_V2 a) { return cf_v2(-a.x, -a.y); }
CUTE_INLINE CF_V2 cf_lerp_v2(CF_V2 a, CF_V2 b, float t) { return cf_add_v2(a, cf_mul_v2_f(cf_sub_v2(b, a), t)); }
CUTE_INLINE CF_V2 cf_bezier(CF_V2 a, CF_V2 c0, CF_V2 b, float t) { return cf_lerp_v2(cf_lerp_v2(a, c0, t), cf_lerp_v2(c0, b, t), t); }
CUTE_INLINE CF_V2 cf_bezier2(CF_V2 a, CF_V2 c0, CF_V2 c1, CF_V2 b, float t) { return cf_bezier(cf_lerp_v2(a, c0, t), cf_lerp_v2(c0, c1, t), cf_lerp_v2(c1, b, t), t); }
CUTE_INLINE int cf_lesser_v2(CF_V2 a, CF_V2 b) { return a.x < b.x&& a.y < b.y; }
CUTE_INLINE int cf_greater_v2(CF_V2 a, CF_V2 b) { return a.x > b.x && a.y > b.y; }
CUTE_INLINE int cf_lesser_equal_v2(CF_V2 a, CF_V2 b) { return a.x <= b.x && a.y <= b.y; }
CUTE_INLINE int cf_greater_equal_v2(CF_V2 a, CF_V2 b) { return a.x >= b.x && a.y >= b.y; }
CUTE_INLINE CF_V2 cf_floor(CF_V2 a) { return cf_v2(floorf(a.x), floorf(a.y)); }
CUTE_INLINE CF_V2 cf_round(CF_V2 a) { return cf_v2(roundf(a.x), roundf(a.y)); }
CUTE_INLINE CF_V2 cf_safe_invert_v2(CF_V2 a) { return cf_v2(cf_safe_invert(a.x), cf_safe_invert(a.y)); }
CUTE_INLINE CF_V2 cf_sign_v2(CF_V2 a) { return cf_v2(cf_sign(a.x), cf_sign(a.y)); }

CUTE_INLINE int cf_parallel(CF_V2 a, CF_V2 b, float tol)
{
	float k = cf_len(a) / cf_len(b);
	b = cf_mul_v2_f(b, k);
	if (fabs(a.x - b.x) < tol && fabs(a.y - b.y) < tol) return 1;
	return 0;
}

//--------------------------------------------------------------------------------------------------
// CF_SinCos rotation ops.

CUTE_INLINE CF_SinCos cf_sincos_f(float radians) { CF_SinCos r; r.s = sinf(radians); r.c = cosf(radians); return r; }
CUTE_INLINE CF_SinCos cf_sincos() { CF_SinCos r; r.c = 1.0f; r.s = 0; return r; }
CUTE_INLINE CF_V2 cf_x_axis(CF_SinCos r) { return cf_v2(r.c, r.s); }
CUTE_INLINE CF_V2 cf_y_axis(CF_SinCos r) { return cf_v2(-r.s, r.c); }
CUTE_INLINE CF_V2 cf_mul_sc_v2(CF_SinCos a, CF_V2 b) { return cf_v2(a.c * b.x - a.s * b.y, a.s * b.x + a.c * b.y); }
CUTE_INLINE CF_V2 cf_mulT_sc_v2(CF_SinCos a, CF_V2 b) { return cf_v2(a.c * b.x + a.s * b.y, -a.s * b.x + a.c * b.y); }
CUTE_INLINE CF_SinCos cf_mul_sc(CF_SinCos a, CF_SinCos b) { CF_SinCos c; c.c = a.c * b.c - a.s * b.s; c.s = a.s * b.c + a.c * b.s; return c; }
CUTE_INLINE CF_SinCos cf_mulT_sc(CF_SinCos a, CF_SinCos b) { CF_SinCos c; c.c = a.c * b.c + a.s * b.s; c.s = a.c * b.s - a.s * b.c; return c; }

// Remaps the result from atan2f to the continuous range of 0, 2*PI.
CUTE_INLINE float cf_atan2_360(float y, float x) { return atan2f(-y, -x) + CUTE_PI; }
CUTE_INLINE float cf_atan2_360_sc(CF_SinCos r) { return cf_atan2_360(r.s, r.c); }
CUTE_INLINE float cf_atan2_360_v2(CF_V2 v) { return atan2f(-v.y, -v.x) + CUTE_PI; }

// Computes the shortest angle to rotate the vector a to the vector b.
CUTE_INLINE float cf_shortest_arc(CF_V2 a, CF_V2 b)
{
	float c = cf_dot(a, b);
	float s = cf_det2(a, b);
	float theta = acosf(c);
	if (s > 0) {
		return theta;
	} else {
		return -theta;
	}
}

CUTE_INLINE float cf_angle_diff(float radians_a, float radians_b) { return cf_mod((radians_b - radians_a) + CUTE_PI, 2.0f * CUTE_PI) - CUTE_PI; }
CUTE_INLINE CF_V2 cf_from_angle(float radians) { return cf_v2(cosf(radians), sinf(radians)); }

//--------------------------------------------------------------------------------------------------
// m2 ops.
// 2D graphics matrix for only scale + rotate.

CUTE_INLINE CF_M2x2 cf_mul_m2_f(CF_M2x2 a, float b) { CF_M2x2 c; c.x = cf_mul_v2_f(a.x, b); c.y = cf_mul_v2_f(a.y, b); return c; }
CUTE_INLINE CF_V2 cf_mul_m2_v2(CF_M2x2 a, CF_V2 b) { CF_V2 c; c.x = a.x.x * b.x + a.y.x * b.y; c.y = a.x.y * b.x + a.y.y * b.y; return c; }
CUTE_INLINE CF_M2x2 cf_mul_m2(CF_M2x2 a, CF_M2x2 b) { CF_M2x2 c; c.x = cf_mul_m2_v2(a, b.x); c.y = cf_mul_m2_v2(a, b.y); return c; }

//--------------------------------------------------------------------------------------------------
// m3x2 ops.
// General purpose 2D graphics matrix; scale + rotate + translate.

CUTE_INLINE CF_V2 cf_mul_m32_v2(CF_M3x2 a, CF_V2 b) { return cf_add_v2(cf_mul_m2_v2(a.m, b), a.p); }
CUTE_INLINE CF_M3x2 cf_mul_m32(CF_M3x2 a, CF_M3x2 b) { CF_M3x2 c; c.m = cf_mul_m2(a.m, b.m); c.p = cf_add_v2(cf_mul_m2_v2(a.m, b.p), a.p); return c; }
CUTE_INLINE CF_M3x2 cf_make_identity() { CF_M3x2 m; m.m.x = cf_v2(1, 0); m.m.y = cf_v2(0, 1); m.p = cf_v2(0, 0); return m; }
CUTE_INLINE CF_M3x2 cf_make_translation_f(float x, float y) { CF_M3x2 m; m.m.x = cf_v2(1, 0); m.m.y = cf_v2(0, 1); m.p = cf_v2(x, y); return m; }
CUTE_INLINE CF_M3x2 cf_make_translation(CF_V2 p) { return cf_make_translation_f(p.x, p.y); }
CUTE_INLINE CF_M3x2 cf_make_scale(CF_V2 s) { CF_M3x2 m; m.m.x = cf_v2(s.x, 0); m.m.y = cf_v2(0, s.y); m.p = cf_v2(0, 0); return m; }
CUTE_INLINE CF_M3x2 cf_make_scale_f(float s) { return cf_make_scale(cf_v2(s, s)); }
CUTE_INLINE CF_M3x2 cf_make_scale_translation(CF_V2 s, CF_V2 p) { CF_M3x2 m; m.m.x = cf_v2(s.x, 0); m.m.y = cf_v2(0, s.y); m.p = p; return m; }
CUTE_INLINE CF_M3x2 cf_make_scale_translation_f(float s, CF_V2 p) { return cf_make_scale_translation(cf_v2(s, s), p); }
CUTE_INLINE CF_M3x2 cf_make_scale_translation_f_f(float sx, float sy, CF_V2 p) { return cf_make_scale_translation(cf_v2(sx, sy), p); }
CUTE_INLINE CF_M3x2 cf_make_rotation(float radians) { CF_SinCos sc = cf_sincos_f(radians); CF_M3x2 m; m.m.x = cf_v2(sc.c, -sc.s); m.m.y = cf_v2(sc.s, sc.c); m.p = cf_v2(0, 0); return m; }
CUTE_INLINE CF_M3x2 cf_make_transform_TSR(CF_V2 p, CF_V2 s, float radians) { CF_SinCos sc = cf_sincos_f(radians); CF_M3x2 m; m.m.x = cf_mul_v2_f(cf_v2(sc.c, -sc.s), s.x); m.m.y = cf_mul_v2_f(cf_v2(sc.s, sc.c), s.y); m.p = p; return m; }

CUTE_INLINE CF_M3x2 cf_invert(CF_M3x2 a)
{
	float id = cf_safe_invert(cf_det2(a.m.x, a.m.y));
	CF_M3x2 b;
	b.m.x = cf_v2(a.m.y.y * id, -a.m.x.y * id);
	b.m.y = cf_v2(-a.m.y.x * id, a.m.x.x * id);
	b.p.x = (a.m.y.x * a.p.y - a.p.x * a.m.y.y) * id;
	b.p.y = (a.p.x * a.m.x.y - a.m.x.x * a.p.y) * id;
	return b;
}

//--------------------------------------------------------------------------------------------------
// Transform ops.
// No scale factor allowed here, good for physics + colliders.

CUTE_INLINE CF_Transform cf_make_transform() { CF_Transform x; x.p = cf_v2(0, 0); x.r = cf_sincos(); return x; }
CUTE_INLINE CF_Transform cf_make_transform_TR(CF_V2 p, float radians) { CF_Transform x; x.r = cf_sincos_f(radians); x.p = p; return x; }
CUTE_INLINE CF_V2 cf_mul_tf_v2(CF_Transform a, CF_V2 b) { return cf_add_v2(cf_mul_sc_v2(a.r, b), a.p); }
CUTE_INLINE CF_V2 cf_mulT_tf_v2(CF_Transform a, CF_V2 b) { return cf_mulT_sc_v2(a.r, cf_sub_v2(b, a.p)); }
CUTE_INLINE CF_Transform cf_mul_tf(CF_Transform a, CF_Transform b) { CF_Transform c; c.r = cf_mul_sc(a.r, b.r); c.p = cf_add_v2(cf_mul_sc_v2(a.r, b.p), a.p); return c; }
CUTE_INLINE CF_Transform cf_mulT_tf(CF_Transform a, CF_Transform b) { CF_Transform c; c.r = cf_mulT_sc(a.r, b.r); c.p = cf_mulT_sc_v2(a.r, cf_sub_v2(b.p, a.p)); return c; }

//--------------------------------------------------------------------------------------------------
// Halfspace (plane/line) ops.
// Functions for infinite lines.

CUTE_INLINE CF_Halfspace cf_plane(CF_V2 n, float d) { CF_Halfspace h; h.n = n; h.d = d; return h; }
CUTE_INLINE CF_Halfspace cf_plane2(CF_V2 n, CF_V2 p) { CF_Halfspace h; h.n = n; h.d = cf_dot(n, p); return h; }
CUTE_INLINE CF_V2 cf_origin(CF_Halfspace h) { return cf_mul_v2_f(h.n, h.d); }
CUTE_INLINE float cf_distance_hs(CF_Halfspace h, CF_V2 p) { return cf_dot(h.n, p) - h.d; }
CUTE_INLINE CF_V2 cf_project(CF_Halfspace h, CF_V2 p) { return cf_sub_v2(p, cf_mul_v2_f(h.n, cf_distance_hs(h, p))); }
CUTE_INLINE CF_Halfspace cf_mul_tf_hs(CF_Transform a, CF_Halfspace b) { CF_Halfspace c; c.n = cf_mul_sc_v2(a.r, b.n); c.d = cf_dot(cf_mul_tf_v2(a, cf_origin(b)), c.n); return c; }
CUTE_INLINE CF_Halfspace cf_mulT_tf_hs(CF_Transform a, CF_Halfspace b) { CF_Halfspace c; c.n = cf_mulT_sc_v2(a.r, b.n); c.d = cf_dot(cf_mulT_tf_v2(a, cf_origin(b)), c.n); return c; }
CUTE_INLINE CF_V2 cf_intersect_halfspace(CF_V2 a, CF_V2 b, float da, float db) { return cf_add_v2(a, cf_mul_v2_f(cf_sub_v2(b, a), (da / (da - db)))); }
CUTE_INLINE CF_V2 cf_intersect_halfspace2(CF_Halfspace h, CF_V2 a, CF_V2 b) { return cf_intersect_halfspace(a, b, cf_distance_hs(h, a), cf_distance_hs(h, b)); }

//--------------------------------------------------------------------------------------------------
// AABB helpers.

CUTE_INLINE CF_Aabb cf_make_aabb(CF_V2 min, CF_V2 max) { CF_Aabb bb; bb.min = min; bb.max = max; return bb; }
CUTE_INLINE CF_Aabb cf_make_aabb_pos_w_h(CF_V2 pos, float w, float h) { CF_Aabb bb; CF_V2 he = cf_mul_v2_f(cf_v2(w, h), 0.5f); bb.min = cf_sub_v2(pos, he); bb.max = cf_add_v2(pos, he); return bb; }
CUTE_INLINE CF_Aabb cf_make_aabb_center_half_extents(CF_V2 center, CF_V2 half_extents) { CF_Aabb bb; bb.min = cf_sub_v2(center, half_extents); bb.max = cf_add_v2(center, half_extents); return bb; }
CUTE_INLINE CF_Aabb cf_make_aabb_from_top_left(CF_V2 top_left, float w, float h) { return cf_make_aabb(cf_add_v2(top_left, cf_v2(0, -h)), cf_add_v2(top_left, cf_v2(w, 0))); }
CUTE_INLINE float cf_width(CF_Aabb bb) { return bb.max.x - bb.min.x; }
CUTE_INLINE float cf_height(CF_Aabb bb) { return bb.max.y - bb.min.y; }
CUTE_INLINE float cf_half_width(CF_Aabb bb) { return cf_width(bb) * 0.5f; }
CUTE_INLINE float cf_half_height(CF_Aabb bb) { return cf_height(bb) * 0.5f; }
CUTE_INLINE CF_V2 cf_half_extents(CF_Aabb bb) { return (cf_mul_v2_f(cf_sub_v2(bb.max, bb.min), 0.5f)); }
CUTE_INLINE CF_V2 cf_extents(CF_Aabb aabb) { return cf_sub_v2(aabb.max, aabb.min); }
CUTE_INLINE CF_Aabb cf_expand_aabb(CF_Aabb aabb, CF_V2 v) { return cf_make_aabb(cf_sub_v2(aabb.min, v), cf_add_v2(aabb.max, v)); }
CUTE_INLINE CF_Aabb cf_expand_aabb_f(CF_Aabb aabb, float v) { CF_V2 factor = cf_v2(v, v); return cf_make_aabb(cf_sub_v2(aabb.min, factor), cf_add_v2(aabb.max, factor)); }
CUTE_INLINE CF_V2 cf_min_aabb(CF_Aabb bb) { return bb.min; }
CUTE_INLINE CF_V2 cf_max_aabb(CF_Aabb bb) { return bb.max; }
CUTE_INLINE CF_V2 cf_midpoint(CF_Aabb bb) { return cf_mul_v2_f(cf_add_v2(bb.min, bb.max), 0.5f); }
CUTE_INLINE CF_V2 cf_center(CF_Aabb bb) { return cf_mul_v2_f(cf_add_v2(bb.min, bb.max), 0.5f); }
CUTE_INLINE CF_V2 cf_top_left(CF_Aabb bb) { return cf_v2(bb.min.x, bb.max.y); }
CUTE_INLINE CF_V2 cf_top_right(CF_Aabb bb) { return cf_v2(bb.max.x, bb.max.y); }
CUTE_INLINE CF_V2 cf_bottom_left(CF_Aabb bb) { return cf_v2(bb.min.x, bb.min.y); }
CUTE_INLINE CF_V2 cf_bottom_right(CF_Aabb bb) { return cf_v2(bb.max.x, bb.min.y); }
CUTE_INLINE bool cf_contains_point(CF_Aabb bb, CF_V2 p) { return cf_greater_equal_v2(p, bb.min) && cf_lesser_equal_v2(p, bb.max); }
CUTE_INLINE bool cf_contains_aabb(CF_Aabb a, CF_Aabb b) { return cf_lesser_equal_v2(a.min, b.min) && cf_greater_equal_v2(a.max, b.max); }
CUTE_INLINE float cf_surface_area_aabb(CF_Aabb bb) { return 2.0f * cf_width(bb) * cf_height(bb); }
CUTE_INLINE float cf_area_aabb(CF_Aabb bb) { return cf_width(bb) * cf_height(bb); }
CUTE_INLINE CF_V2 cf_clamp_aabb_v2(CF_Aabb bb, CF_V2 p) { return cf_clamp_v2(p, bb.min, bb.max); }
CUTE_INLINE CF_Aabb cf_clamp_aabb(CF_Aabb a, CF_Aabb b) { return cf_make_aabb(cf_clamp_v2(a.min, b.min, b.max), cf_clamp_v2(a.max, b.min, b.max)); }
CUTE_INLINE CF_Aabb cf_combine(CF_Aabb a, CF_Aabb b) { return cf_make_aabb(cf_min_v2(a.min, b.min), cf_max_v2(a.max, b.max)); }

CUTE_INLINE int cf_overlaps(CF_Aabb a, CF_Aabb b)
{
	int d0 = b.max.x < a.min.x;
	int d1 = a.max.x < b.min.x;
	int d2 = b.max.y < a.min.y;
	int d3 = a.max.y < b.min.y;
	return !(d0 | d1 | d2 | d3);
}

CUTE_INLINE int cf_collide_aabb(CF_Aabb a, CF_Aabb b) { return cf_overlaps(a, b); }

CUTE_INLINE CF_Aabb cf_make_aabb_verts(CF_V2* verts, int count)
{
	CF_V2 vmin = verts[0];
	CF_V2 vmax = vmin;
	for (int i = 0; i < count; ++i) {
		vmin = cf_min_v2(vmin, verts[i]);
		vmax = cf_max_v2(vmax, verts[i]);
	}
	return cf_make_aabb(vmin, vmax);
}

CUTE_INLINE void cf_aabb_verts(CF_V2* out, CF_Aabb bb)
{
	out[0] = bb.min;
	out[1] = cf_v2(bb.max.x, bb.min.y);
	out[2] = bb.max;
	out[3] = cf_v2(bb.min.x, bb.max.y);
}

//--------------------------------------------------------------------------------------------------
// Circle helpers.

CUTE_INLINE float cf_area_circle(CF_Circle c) { return 3.14159265f * c.r * c.r; }
CUTE_INLINE float cf_surface_area_circle(CF_Circle c) { return 2.0f * 3.14159265f * c.r; }
CUTE_INLINE CF_Circle cf_mul_tf_circle(CF_Transform tx, CF_Circle a) { CF_Circle b; b.p = cf_mul_tf_v2(tx, a.p); b.r = a.r; return b; }

//--------------------------------------------------------------------------------------------------
// Ray ops.
// Full raycasting suite is farther down below in this file.

CUTE_INLINE CF_V2 cf_impact(CF_Ray r, float t) { return cf_add_v2(r.p, cf_mul_v2_f(r.d, t)); }
CUTE_INLINE CF_V2 cf_endpoint(CF_Ray r) { return cf_add_v2(r.p, cf_mul_v2_f(r.d, r.t)); }

CUTE_INLINE int cf_ray_to_halfpsace(CF_Ray A, CF_Halfspace B, CF_Raycast* out)
{
	float da = cf_distance_hs(B, A.p);
	float db = cf_distance_hs(B, cf_impact(A, A.t));
	if (da * db > 0) return 0;
	out->n = cf_mul_v2_f(B.n, cf_sign(da));
	out->t = cf_intersect(da, db);
	return 1;
}

// http://www.randygaul.net/2014/07/23/distance-point-to-line-segment/
CUTE_INLINE float cf_distance_sq(CF_V2 a, CF_V2 b, CF_V2 p)
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
// Collision detection.

// It's quite common to limit the number of verts on polygons to a low number. Feel free to adjust
// this number if needed, but be warned: higher than 8 and shapes generally start to look more like
// circles/ovals; it becomes pointless beyond a certain point.
#define CUTE_POLY_MAX_VERTS 8

// 2D polygon. Verts are ordered in counter-clockwise order (CCW).
typedef struct CF_Poly
{
	int count;
	CF_V2 verts[CUTE_POLY_MAX_VERTS];
	CF_V2 norms[CUTE_POLY_MAX_VERTS]; // Pointing perpendicular along the poly's surface.
	                                  // Rotated vert[i] to vert[i + 1] 90 degrees CCW + normalized.
} CF_Poly;

// 2D capsule shape. It's like a shrink-wrap of 2 circles connected by a rod.
typedef struct CF_Capsule
{
	CF_V2 a;
	CF_V2 b;
	float r;
} CF_Capsule;

// Contains all information necessary to resolve a collision, or in other words
// this is the information needed to separate shapes that are colliding. Doing
// the resolution step is *not* included.
typedef struct CF_Manifold
{
	int count;
	float depths[2];
	CF_V2 contact_points[2];

	// Always points from shape A to shape B.
	CF_V2 n;
} CF_Manifold;

// Boolean collision detection functions.
// These versions are slightly faster/simpler than the manifold versions, but only give a YES/NO result.
CUTE_API bool CUTE_CALL cf_circle_to_circle(CF_Circle A, CF_Circle B);
CUTE_API bool CUTE_CALL cf_circle_to_aabb(CF_Circle A, CF_Aabb B);
CUTE_API bool CUTE_CALL cf_circle_to_capsule(CF_Circle A, CF_Capsule B);
CUTE_API bool CUTE_CALL cf_aabb_to_aabb(CF_Aabb A, CF_Aabb B);
CUTE_API bool CUTE_CALL cf_aabb_to_capsule(CF_Aabb A, CF_Capsule B);
CUTE_API bool CUTE_CALL cf_capsule_to_capsule(CF_Capsule A, CF_Capsule B);
CUTE_API bool CUTE_CALL cf_circle_to_poly(CF_Circle A, const CF_Poly* B, const CF_Transform* bx);
CUTE_API bool CUTE_CALL cf_aabb_to_poly(CF_Aabb A, const CF_Poly* B, const CF_Transform* bx);
CUTE_API bool CUTE_CALL cf_capsule_to_poly(CF_Capsule A, const CF_Poly* B, const CF_Transform* bx);
CUTE_API bool CUTE_CALL cf_poly_to_poly(const CF_Poly* A, const CF_Transform* ax, const CF_Poly* B, const CF_Transform* bx);

// Ray casting.
// Output is placed into the `CF_Raycast` struct, which represents the hit location
// of the ray. The out param contains no meaningful information if these funcs
// return false.
CUTE_API bool CUTE_CALL cf_ray_to_circle(CF_Ray A, CF_Circle B, CF_Raycast* out);
CUTE_API bool CUTE_CALL cf_ray_to_aabb(CF_Ray A, CF_Aabb B, CF_Raycast* out);
CUTE_API bool CUTE_CALL cf_ray_to_capsule(CF_Ray A, CF_Capsule B, CF_Raycast* out);
CUTE_API bool CUTE_CALL cf_ray_to_poly(CF_Ray A, const CF_Poly* B, const CF_Transform* bx_ptr, CF_Raycast* out);

// Manifold generation.
// These functions are (generally) slower + more complex than bool versions, but compute one
// or two points that represent the plane of contact. This information is
// is usually needed to resolve and prevent shapes from colliding. If no coll-
// ision occured the `count` member of the manifold typedef struct is set to 0.
CUTE_API void CUTE_CALL cf_circle_to_circle_manifold(CF_Circle A, CF_Circle B, CF_Manifold* m);
CUTE_API void CUTE_CALL cf_circle_to_aabb_manifold(CF_Circle A, CF_Aabb B, CF_Manifold* m);
CUTE_API void CUTE_CALL cf_circle_to_capsule_manifold(CF_Circle A, CF_Capsule B, CF_Manifold* m);
CUTE_API void CUTE_CALL cf_aabb_to_aabb_manifold(CF_Aabb A, CF_Aabb B, CF_Manifold* m);
CUTE_API void CUTE_CALL cf_aabb_to_capsule_manifold(CF_Aabb A, CF_Capsule B, CF_Manifold* m);
CUTE_API void CUTE_CALL cf_capsule_to_capsule_manifold(CF_Capsule A, CF_Capsule B, CF_Manifold* m);
CUTE_API void CUTE_CALL cf_circle_to_poly_manifold(CF_Circle A, const CF_Poly* B, const CF_Transform* bx, CF_Manifold* m);
CUTE_API void CUTE_CALL cf_aabb_to_poly_manifold(CF_Aabb A, const CF_Poly* B, const CF_Transform* bx, CF_Manifold* m);
CUTE_API void CUTE_CALL cf_capsule_to_poly_manifold(CF_Capsule A, const CF_Poly* B, const CF_Transform* bx, CF_Manifold* m);
CUTE_API void CUTE_CALL cf_poly_to_poly_manifold(const CF_Poly* A, const CF_Transform* ax, const CF_Poly* B, const CF_Transform* bx, CF_Manifold* m);

#define CF_SHAPE_TYPE_DEFS \
	CF_ENUM(SHAPE_TYPE_NONE,    0) \
	CF_ENUM(SHAPE_TYPE_CIRCLE,  1) \
	CF_ENUM(SHAPE_TYPE_AABB,    2) \
	CF_ENUM(SHAPE_TYPE_CAPSULE, 3) \
	CF_ENUM(SHAPE_TYPE_POLY,    4) \

typedef enum CF_ShapeType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_SHAPE_TYPE_DEFS
	#undef CF_ENUM
} CF_ShapeType;

CUTE_INLINE const char* cf_shape_type_to_string(CF_ShapeType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_SHAPE_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

// This typedef struct is only for advanced usage of the `cf_gjk` function. See comments inside of the
// `cf_gjk` function for more details.
typedef struct CF_GjkCache
{
	float metric;
	int count;
	int iA[3];
	int iB[3];
	float div;
} CF_GjkCache;

// This is an advanced function, intended to be used by people who know what they're doing.
//
// Runs the GJK algorithm to find closest points, returns distance between closest points.
// outA and outB can be NULL, in this case only distance is returned. ax_ptr and bx_ptr
// can be NULL, and represent local to world transformations for shapes A and B respectively.
// use_radius will apply radii for capsules and circles (if set to false, spheres are
// treated as points and capsules are treated as line segments i.e. rays). The cache parameter
// should be NULL, as it is only for advanced usage (unless you know what you're doing, then
// go ahead and use it). iterations is an optional parameter.
//
// 
// IMPORTANT NOTE
// 
//     The GJK function is sensitive to large shapes, since it internally will compute signed area
//     values. `cf_gjk` is called throughout this file in many ways, so try to make sure all of your
//     collision shapes are not gigantic. For example, try to keep the volume of all your shapes
//     less than 100.0f. If you need large shapes, you should use tiny collision geometry for all
//     function here, and simply render the geometry larger on-screen by scaling it up.
//
CUTE_API float CUTE_CALL cf_gjk(const void* A, CF_ShapeType typeA, const CF_Transform* ax_ptr, const void* B, CF_ShapeType typeB, const CF_Transform* bx_ptr, CF_V2* outA, CF_V2* outB, int use_radius, int* iterations, CF_GjkCache* cache);

// Stores results of a time of impact calculation done by `cf_toi`.
typedef struct CF_ToiResult
{
	int hit;        // 1 if shapes were touching at the TOI, 0 if they never hit.
	float toi;      // The time of impact between two shapes.
	CF_V2 n;        // Surface normal from shape A to B at the time of impact.
	CF_V2 p;        // Point of contact between shapes A and B at time of impact.
	int iterations; // Number of iterations the solver underwent.
} CF_ToiResult;

// This is an advanced function, intended to be used by people who know what they're doing.
//
// Computes the time of impact from shape A and shape B. The velocity of each shape is provided
// by vA and vB respectively. The shapes are *not* allowed to rotate over time. The velocity is
// assumed to represent the change in motion from time 0 to time 1, and so the return value will
// be a number from 0 to 1. To move each shape to the colliding configuration, multiply vA and vB
// each by the return value. ax_ptr and bx_ptr are optional parameters to transforms for each shape,
// and are typically used for polygon shapes to transform from model to world space. Set these to
// NULL to represent identity transforms. iterations is an optional parameter. use_radius
// will apply radii for capsules and circles (if set to false, spheres are treated as points and
// capsules are treated as line segments i.e. rays).
//
// IMPORTANT NOTE:
// The cf_toi function can be used to implement a "swept character controller", but it can be
// difficult to do so. Say we compute a time of impact with `cf_toi` and move the shapes to the
// time of impact, and adjust the velocity by zeroing out the velocity along the surface normal.
// If we then call `cf_toi` again, it will fail since the shapes will be considered to start in
// a colliding configuration. There are many styles of tricks to get around this problem, and
// all of them involve giving the next call to `cf_toi` some breathing room. It is recommended
// to use some variation of the following algorithm:
//
// 1. Call cf_toi.
// 2. Move the shapes to the TOI.
// 3. Slightly inflate the size of one, or both, of the shapes so they will be intersecting.
//    The purpose is to make the shapes numerically intersecting, but not visually intersecting.
//    Another option is to call cf_toi with slightly deflated shapes.
//    See the function `cf_inflate` for some more details.
// 4. Compute the collision manifold between the inflated shapes (for example, use poly_ttoPolyManifold).
// 5. Gently push the shapes apart. This will give the next call to cf_toi some breathing room.
CUTE_API CF_ToiResult CUTE_CALL cf_toi(const void* A, CF_ShapeType typeA, const CF_Transform* ax_ptr, CF_V2 vA, const void* B, CF_ShapeType typeB, const CF_Transform* bx_ptr, CF_V2 vB, int use_radius);

// Inflating a shape.
//
// This is useful to numerically grow or shrink a polytope. For example, when calling
// a time of impact function it can be good to use a slightly smaller shape. Then, once
// both shapes are moved to the time of impact a collision manifold can be made from the
// slightly larger (and now overlapping) shapes.
//
// IMPORTANT NOTE
// Inflating a shape with sharp corners can cause those corners to move dramatically.
// Deflating a shape can avoid this problem, but deflating a very small shape can invert
// the planes and result in something that is no longer convex. Make sure to pick an
// appropriately small skin factor, for example 1.0e-6f.
CUTE_API void CUTE_CALL cf_inflate(void* shape, CF_ShapeType type, float skin_factor);

// Computes 2D convex hull. Will not do anything if less than two verts supplied. If
// more than CUTE_POLY_MAX_VERTS are supplied extras are ignored.
CUTE_API int CUTE_CALL cf_hull(CF_V2* verts, int count);
CUTE_API void CUTE_CALL cf_norms(CF_V2* verts, CF_V2* norms, int count);

// runs cf_hull and cf_norms, assumes p->verts and p->count are both set to valid values
CUTE_API void CUTE_CALL cf_make_poly(CF_Poly* p);
CUTE_API CF_V2 CUTE_CALL cf_centroid(const CF_V2* verts, int count);

// Generic collision detection routines, useful for games that want to use some poly-
// morphism to write more generic-styled code. Internally calls various above functions.
// For AABBs/Circles/Capsules ax and bx are ignored. For polys ax and bx can define
// model to world transformations (for polys only), or be NULL for identity transforms.
CUTE_API int CUTE_CALL cf_collided(const void* A, const CF_Transform* ax, CF_ShapeType typeA, const void* B, const CF_Transform* bx, CF_ShapeType typeB);
CUTE_API void CUTE_CALL cf_collide(const void* A, const CF_Transform* ax, CF_ShapeType typeA, const void* B, const CF_Transform* bx, CF_ShapeType typeB, CF_Manifold* m);
CUTE_API bool CUTE_CALL cf_cast_ray(CF_Ray A, const void* B, const CF_Transform* bx, CF_ShapeType typeB, CF_Raycast* out);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using v2 = CF_V2;

CUTE_INLINE v2 V2(float x, float y) { v2 result; result.x = x; result.y = y; return result; }

CUTE_INLINE v2 operator+(v2 a, v2 b) { return V2(a.x + b.x, a.y + b.y); }
CUTE_INLINE v2 operator-(v2 a, v2 b) { return V2(a.x - b.x, a.y - b.y); }
CUTE_INLINE v2& operator+=(v2& a, v2 b) { return a = a + b; }
CUTE_INLINE v2& operator-=(v2& a, v2 b) { return a = a - b; }

CUTE_INLINE v2 operator*(v2 a, float b) { return V2(a.x * b, a.y * b); }
CUTE_INLINE v2 operator*(v2 a, v2 b) { return V2(a.x * b.x, a.y * b.y); }
CUTE_INLINE v2& operator*=(v2& a, float b) { return a = a * b; }
CUTE_INLINE v2& operator*=(v2& a, v2 b) { return a = a * b; }
CUTE_INLINE v2 operator/(v2 a, float b) { return V2(a.x / b, a.y / b); }
CUTE_INLINE v2 operator/(v2 a, v2 b) { return V2(a.x / b.x, a.y / b.y); }
CUTE_INLINE v2& operator/=(v2& a, float b) { return a = a / b; }
CUTE_INLINE v2& operator/=(v2& a, v2 b) { return a = a / b; }

CUTE_INLINE v2 operator-(v2 a) { return V2(-a.x, -a.y); }

CUTE_INLINE int operator<(v2 a, v2 b) { return a.x < b.x&& a.y < b.y; }
CUTE_INLINE int operator>(v2 a, v2 b) { return a.x > b.x && a.y > b.y; }
CUTE_INLINE int operator<=(v2 a, v2 b) { return a.x <= b.x && a.y <= b.y; }
CUTE_INLINE int operator>=(v2 a, v2 b) { return a.x >= b.x && a.y >= b.y; }

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
using Capsule = CF_Capsule;
using Manifold = CF_Manifold;
using GjkCache = CF_GjkCache;
using ToiResult = CF_ToiResult;

using ShapeType = CF_ShapeType;
#define CF_ENUM(K, V) CUTE_INLINE constexpr ShapeType K = CF_##K;
CF_SHAPE_TYPE_DEFS
#undef CF_ENUM

CUTE_INLINE const char* to_string(ShapeType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_SHAPE_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CUTE_INLINE float min(float a, float b) { return cf_min(a, b); }
CUTE_INLINE float max(float a, float b) { return cf_max(a, b); }
CUTE_INLINE float clamp(float a, float lo, float hi) { return cf_clamp(a, lo, hi); }
CUTE_INLINE float clamp01(float a) { return cf_clamp01(a); }
CUTE_INLINE float sign(float a) { return cf_sign(a); }
CUTE_INLINE float intersect(float da, float db) { return cf_intersect(da, db); }
CUTE_INLINE float safe_invert(float a) { return cf_safe_invert(a); }
CUTE_INLINE float lerp_f(float a, float b, float t) { return cf_lerp(a, b, t); }
CUTE_INLINE float remap(float t, float lo, float hi) { return cf_remap(t, lo, hi); }
CUTE_INLINE float mod(float x, float m) { return cf_mod(x, m); }
CUTE_INLINE float fract(float x) { return cf_fract(x); }

CUTE_INLINE int sign(int a) { return cf_sign_int(a); }
CUTE_INLINE int min(int a, int b) { return cf_min(a, b); }
CUTE_INLINE int max(int a, int b) { return cf_max(a, b); }
CUTE_INLINE uint64_t min(uint64_t a, uint64_t b) { return cf_min(a, b); }
CUTE_INLINE uint64_t max(uint64_t a, uint64_t b) { return cf_max(a, b); }
CUTE_INLINE float abs(float a) { return cf_abs(a); }
CUTE_INLINE int abs(int a) { return cf_abs_int(a); }
CUTE_INLINE int clamp(int a, int lo, int hi) { return cf_clamp_int(a, lo, hi); }
CUTE_INLINE int clamp01(int a) { return cf_clamp01_int(a); }
CUTE_INLINE bool is_even(int x) { return cf_is_even(x); }
CUTE_INLINE bool is_odd(int x) { return cf_is_odd(x); }

CUTE_INLINE bool is_power_of_two(int a) { return cf_is_power_of_two(a); }
CUTE_INLINE bool is_power_of_two(uint64_t a) { return cf_is_power_of_two_uint(a); }
CUTE_INLINE int fit_power_of_two(int a) { return cf_fit_power_of_two(a); }

CUTE_INLINE float smoothstep(float x) { return cf_smoothstep(x); }
CUTE_INLINE float quad_in(float x) { return cf_quad_in(x); }
CUTE_INLINE float quad_out(float x) { return cf_quad_out(x); }
CUTE_INLINE float quad_in_out(float x) { return cf_quad_in_out(x); }
CUTE_INLINE float cube_in(float x) { return cf_cube_in(x); }
CUTE_INLINE float cube_out(float x) { return cf_cube_out(x); }
CUTE_INLINE float cube_in_out(float x) { return cf_cube_in_out(x); }
CUTE_INLINE float quart_in(float x) { return cf_quart_in(x); }
CUTE_INLINE float quart_out(float x) { return cf_quart_out(x); }
CUTE_INLINE float quart_in_out(float x) { return cf_quart_in_out(x); }
CUTE_INLINE float quint_in(float x) { return cf_quint_in(x); }
CUTE_INLINE float quint_out(float x) { return cf_quint_out(x); }
CUTE_INLINE float quint_in_out(float x) { return cf_quint_in_out(x); }
CUTE_INLINE float sin_in(float x) { return cf_sin_in(x); }
CUTE_INLINE float sin_out(float x) { return cf_sin_out(x); }
CUTE_INLINE float sin_in_out(float x) { return cf_sin_in_out(x); }
CUTE_INLINE float circle_in(float x) { return cf_circle_in(x); }
CUTE_INLINE float circle_out(float x) { return cf_circle_out(x); }
CUTE_INLINE float circle_in_out(float x) { return cf_circle_in_out(x); }
CUTE_INLINE float back_in(float x) { return cf_back_in(x); }
CUTE_INLINE float back_out(float x) { return cf_back_out(x); }
CUTE_INLINE float back_in_out(float x) { return cf_back_in_out(x); }

CUTE_INLINE float dot(v2 a, v2 b) { return cf_dot(a, b); }

CUTE_INLINE v2 skew(v2 a) { return cf_skew(a); }
CUTE_INLINE v2 cw90(v2 a) { return cf_cw90(a); }
CUTE_INLINE float det2(v2 a, v2 b) { return cf_det2(a, b); }
CUTE_INLINE float cross(v2 a, v2 b) { return cf_cross(a, b); }
CUTE_INLINE v2 cross(v2 a, float b) { return cf_cross_v2_f(a, b); }
CUTE_INLINE v2 cross(float a, v2 b) { return cf_cross_f_v2(a, b); }
CUTE_INLINE v2 min(v2 a, v2 b) { return cf_min_v2(a, b); }
CUTE_INLINE v2 max(v2 a, v2 b) { return cf_max_v2(a, b); }
CUTE_INLINE v2 clamp(v2 a, v2 lo, v2 hi) { return cf_clamp_v2(a, lo, hi); }
CUTE_INLINE v2 clamp01(v2 a) { return cf_clamp01_v2(a); }
CUTE_INLINE v2 abs(v2 a) { return cf_abs_v2(a); }
CUTE_INLINE float hmin(v2 a) { return cf_hmin(a); }
CUTE_INLINE float hmax(v2 a) { return cf_hmax(a); }
CUTE_INLINE float len(v2 a) { return cf_len(a); }
CUTE_INLINE float length(v2 a) { return cf_len(a); }
CUTE_INLINE float distance(v2 a, v2 b) { return cf_distance(a, b); }
CUTE_INLINE v2 norm(v2 a) { return cf_norm(a); }
CUTE_INLINE v2 safe_norm(v2 a) { return cf_safe_norm(a); }
CUTE_INLINE float safe_norm(float a) { return cf_safe_norm_f(a); }
CUTE_INLINE int safe_norm(int a) { return cf_safe_norm_int(a); }

CUTE_INLINE v2 lerp_v2(v2 a, v2 b, float t) { return cf_lerp_v2(a, b, t); }
CUTE_INLINE v2 bezier(v2 a, v2 c0, v2 b, float t) { return cf_bezier(a, c0, b, t); }
CUTE_INLINE v2 bezier(v2 a, v2 c0, v2 c1, v2 b, float t) { return cf_bezier2(a, c0, c1, b, t); }
CUTE_INLINE v2 floor(v2 a) { return cf_floor(a); }
CUTE_INLINE v2 round(v2 a) { return cf_round(a); }
CUTE_INLINE v2 safe_invert(v2 a) { return cf_safe_invert_v2(a); }
CUTE_INLINE v2 sign(v2 a) { return cf_sign_v2(a); }

CUTE_INLINE int parallel(v2 a, v2 b, float tol) { return cf_parallel(a, b, tol); }

CUTE_INLINE SinCos sincos(float radians) { return cf_sincos_f(radians); }
CUTE_INLINE SinCos sincos() { return cf_sincos(); }
CUTE_INLINE v2 x_axis(SinCos r) { return cf_x_axis(r); }
CUTE_INLINE v2 y_axis(SinCos r) { return cf_y_axis(r); }
CUTE_INLINE v2 mul(SinCos a, v2 b) { return cf_mul_sc_v2(a, b); }
CUTE_INLINE v2 mulT(SinCos a, v2 b) { return cf_mulT_sc_v2(a, b); }
CUTE_INLINE SinCos mul(SinCos a, SinCos b) { return cf_mul_sc(a, b); }
CUTE_INLINE SinCos mulT(SinCos a, SinCos b) { return cf_mulT_sc(a, b); }

CUTE_INLINE float atan2_360(float y, float x) { return cf_atan2_360(y, x); }
CUTE_INLINE float atan2_360(v2 v) { return cf_atan2_360_v2(v); }
CUTE_INLINE float atan2_360(SinCos r) { return cf_atan2_360_sc(r); }

CUTE_INLINE float shortest_arc(v2 a, v2 b) { return cf_shortest_arc(a, b); }

CUTE_INLINE float angle_diff(float radians_a, float radians_b) { return cf_angle_diff(radians_a, radians_b); }
CUTE_INLINE v2 from_angle(float radians) { return cf_from_angle(radians); }

CUTE_INLINE v2 mul(M2x2 a, v2 b) { return cf_mul_m2_v2(a, b); }
CUTE_INLINE M2x2 mul(M2x2 a, M2x2 b) { return cf_mul_m2(a, b); }

CUTE_INLINE v2 mul(M3x2 a, v2 b) { return cf_mul_m32_v2(a, b); }
CUTE_INLINE M3x2 mul(M3x2 a, M3x2 b) { return cf_mul_m32(a, b); }
CUTE_INLINE M3x2 make_identity() { return cf_make_identity(); }
CUTE_INLINE M3x2 make_translation(float x, float y) { return cf_make_translation_f(x, y); }
CUTE_INLINE M3x2 make_translation(v2 p) { return cf_make_translation(p); }
CUTE_INLINE M3x2 make_scale(v2 s) { return cf_make_scale(s); }
CUTE_INLINE M3x2 make_scale(float s) { return cf_make_scale_f(s); }
CUTE_INLINE M3x2 make_scale(v2 s, v2 p) { return cf_make_scale_translation(s, p); }
CUTE_INLINE M3x2 make_scale(float s, v2 p) { return cf_make_scale_translation_f(s, p); }
CUTE_INLINE M3x2 make_scale(float sx, float sy, v2 p) { return cf_make_scale_translation_f_f(sx, sy, p); }
CUTE_INLINE M3x2 make_rotation(float radians) { return cf_make_rotation(radians); }
CUTE_INLINE M3x2 make_transform(v2 p, v2 s, float radians) { return cf_make_transform_TSR(p, s, radians); }
CUTE_INLINE M3x2 invert(M3x2 m) { return cf_invert(m); }

CUTE_INLINE Transform make_transform() { return cf_make_transform(); }
CUTE_INLINE Transform make_transform(v2 p, float radians) { return cf_make_transform_TR(p, radians); }
CUTE_INLINE v2 mul(Transform a, v2 b) { return cf_mul_tf_v2(a, b); }
CUTE_INLINE v2 mulT(Transform a, v2 b) { return cf_mulT_tf_v2(a, b); }
CUTE_INLINE Transform mul(Transform a, Transform b) { return cf_mul_tf(a, b); }
CUTE_INLINE Transform mulT(Transform a, Transform b) { return cf_mulT_tf(a, b); }

CUTE_INLINE Halfspace plane(v2 n, float d) { return cf_plane(n, d); }
CUTE_INLINE Halfspace plane(v2 n, v2 p) { return cf_plane2(n, p); }
CUTE_INLINE v2 origin(Halfspace h) { return cf_origin(h); }
CUTE_INLINE float distance(Halfspace h, v2 p) { return cf_distance_hs(h, p); }
CUTE_INLINE v2 project(Halfspace h, v2 p) { return cf_project(h, p); }
CUTE_INLINE Halfspace mul(Transform a, Halfspace b) { return cf_mul_tf_hs(a, b); }
CUTE_INLINE Halfspace mulT(Transform a, Halfspace b) { return cf_mulT_tf_hs(a, b); }
CUTE_INLINE v2 intersect(v2 a, v2 b, float da, float db) { return cf_intersect_halfspace(a, b, da, db); }
CUTE_INLINE v2 intersect(Halfspace h, v2 a, v2 b) { return cf_intersect_halfspace2(h, a, b); }

CUTE_INLINE Aabb make_aabb(v2 min, v2 max) { return cf_make_aabb(min, max); }
CUTE_INLINE Aabb make_aabb(v2 pos, float w, float h) { return cf_make_aabb_pos_w_h(pos, w, h); }
CUTE_INLINE Aabb make_aabb_center_half_extents(v2 center, v2 half_extents) { return cf_make_aabb_center_half_extents(center, half_extents); }
CUTE_INLINE Aabb make_aabb_from_top_left(v2 top_left, float w, float h) { return cf_make_aabb_from_top_left(top_left, w, h); }
CUTE_INLINE float width(Aabb bb) { return cf_width(bb); }
CUTE_INLINE float height(Aabb bb) { return cf_height(bb); }
CUTE_INLINE float half_width(Aabb bb) { return cf_half_width(bb); }
CUTE_INLINE float half_height(Aabb bb) { return cf_half_height(bb); }
CUTE_INLINE v2 half_extents(Aabb bb) { return cf_half_extents(bb); }
CUTE_INLINE v2 extents(Aabb aabb) { return cf_extents(aabb); }
CUTE_INLINE Aabb expand(Aabb aabb, v2 v) { return cf_expand_aabb(aabb, v); }
CUTE_INLINE Aabb expand(Aabb aabb, float v) { return cf_expand_aabb_f(aabb, v); }
CUTE_INLINE v2 min(Aabb bb) { return cf_min_aabb(bb); }
CUTE_INLINE v2 max(Aabb bb) { return cf_max_aabb(bb); }
CUTE_INLINE v2 midpoint(Aabb bb) { return cf_midpoint(bb); }
CUTE_INLINE v2 center(Aabb bb) { return cf_center(bb); }
CUTE_INLINE v2 top_left(Aabb bb) { return cf_top_left(bb); }
CUTE_INLINE v2 top_right(Aabb bb) { return cf_top_right(bb); }
CUTE_INLINE v2 bottom_left(Aabb bb) { return cf_bottom_left(bb); }
CUTE_INLINE v2 bottom_right(Aabb bb) { return cf_bottom_right(bb); }
CUTE_INLINE bool contains(Aabb bb, v2 p) { return cf_contains_point(bb, p); }
CUTE_INLINE bool contains(Aabb a, Aabb b) { return cf_contains_aabb(a, b); }
CUTE_INLINE float surface_area(Aabb bb) { return cf_surface_area_aabb(bb); }
CUTE_INLINE float area(Aabb bb) { return cf_area_aabb(bb); }
CUTE_INLINE v2 clamp(Aabb bb, v2 p) { return cf_clamp_aabb_v2(bb, p); }
CUTE_INLINE Aabb clamp(Aabb a, Aabb b) { return cf_clamp_aabb(a, b); }
CUTE_INLINE Aabb combine(Aabb a, Aabb b) { return cf_combine(a, b); }

CUTE_INLINE int overlaps(Aabb a, Aabb b) { return cf_overlaps(a, b); }
CUTE_INLINE int collide(Aabb a, Aabb b) { return cf_collide_aabb(a, b); }

CUTE_INLINE Aabb make_aabb(v2* verts, int count) { return cf_make_aabb_verts((CF_V2*)verts, count); }
CUTE_INLINE void aabb_verts(v2* out, Aabb bb) { return cf_aabb_verts((CF_V2*)out, bb); }

CUTE_INLINE float area(Circle c) { return cf_area_circle(c); }
CUTE_INLINE float surface_area(Circle c) { return cf_surface_area_circle(c); }
CUTE_INLINE Circle mul(Transform tx, Circle a) { return cf_mul_tf_circle(tx, a); }

CUTE_INLINE v2 impact(Ray r, float t) { return cf_impact(r, t); }
CUTE_INLINE v2 endpoint(Ray r) { return cf_endpoint(r); }

CUTE_INLINE int ray_to_halfpsace(Ray A, Halfspace B, Raycast* out) { return cf_ray_to_halfpsace(A, B, out); }
CUTE_INLINE float distance_sq(v2 a, v2 b, v2 p) { return cf_distance_sq(a, b, p); }

CUTE_INLINE bool circle_to_circle(Circle A, Circle B) { return cf_circle_to_circle(A, B); }
CUTE_INLINE bool circle_to_aabb(Circle A, Aabb B) { return cf_circle_to_aabb(A, B); }
CUTE_INLINE bool circle_to_capsule(Circle A, Capsule B) { return cf_circle_to_capsule(A, B); }
CUTE_INLINE bool aabb_to_aabb(Aabb A, Aabb B) { return cf_aabb_to_aabb(A, B); }
CUTE_INLINE bool aabb_to_capsule(Aabb A, Capsule B) { return cf_aabb_to_capsule(A, B); }
CUTE_INLINE bool capsule_to_capsule(Capsule A, Capsule B) { return cf_capsule_to_capsule(A, B); }
CUTE_INLINE bool circle_to_poly(Circle A, const Poly* B, const Transform* bx) { return cf_circle_to_poly(A, B, bx); }
CUTE_INLINE bool aabb_to_poly(Aabb A, const Poly* B, const Transform* bx) { return cf_aabb_to_poly(A, B, bx); }
CUTE_INLINE bool capsule_to_poly(Capsule A, const Poly* B, const Transform* bx) { return cf_capsule_to_poly(A, B, bx); }
CUTE_INLINE bool poly_to_poly(const Poly* A, const Transform* ax, const Poly* B, const Transform* bx) { return cf_poly_to_poly(A, ax, B, bx); }

CUTE_INLINE bool ray_to_circle(Ray A, Circle B, Raycast* out) { return cf_ray_to_circle(A, B, out); }
CUTE_INLINE bool ray_to_aabb(Ray A, Aabb B, Raycast* out) { return cf_ray_to_aabb(A, B, out); }
CUTE_INLINE bool ray_to_capsule(Ray A, Capsule B, Raycast* out) { return cf_ray_to_capsule(A, B, out); }
CUTE_INLINE bool ray_to_poly(Ray A, const Poly* B, const Transform* bx_ptr, Raycast* out) { return cf_ray_to_poly(A, B, bx_ptr, out); }

CUTE_INLINE void circle_to_circle_manifold(Circle A, Circle B, Manifold* m) { return cf_circle_to_circle_manifold(A, B, m); }
CUTE_INLINE void circle_to_aabb_manifold(Circle A, Aabb B, Manifold* m) { return cf_circle_to_aabb_manifold(A, B, m); }
CUTE_INLINE void circle_to_capsule_manifold(Circle A, Capsule B, Manifold* m) { return cf_circle_to_capsule_manifold(A, B, m); }
CUTE_INLINE void aabb_to_aabb_manifold(Aabb A, Aabb B, Manifold* m) { return cf_aabb_to_aabb_manifold(A, B, m); }
CUTE_INLINE void aabb_to_capsule_manifold(Aabb A, Capsule B, Manifold* m) { return cf_aabb_to_capsule_manifold(A, B, m); }
CUTE_INLINE void capsule_to_capsule_manifold(Capsule A, Capsule B, Manifold* m) { return cf_capsule_to_capsule_manifold(A, B, m); }
CUTE_INLINE void circle_to_poly_manifold(Circle A, const Poly* B, const Transform* bx, Manifold* m) { return cf_circle_to_poly_manifold(A, B, bx, m); }
CUTE_INLINE void aabb_to_poly_manifold(Aabb A, const Poly* B, const Transform* bx, Manifold* m) { return cf_aabb_to_poly_manifold(A, B, bx, m); }
CUTE_INLINE void capsule_to_poly_manifold(Capsule A, const Poly* B, const Transform* bx, Manifold* m) { return cf_capsule_to_poly_manifold(A, B, bx, m); }
CUTE_INLINE void poly_to_poly_manifold(const Poly* A, const Transform* ax, const Poly* B, const Transform* bx, Manifold* m) { return cf_poly_to_poly_manifold(A, ax, B, bx, m); }

CUTE_INLINE float gjk(const void* A, ShapeType typeA, const Transform* ax_ptr, const void* B, ShapeType typeB, const Transform* bx_ptr, v2* outA, v2* outB, int use_radius, int* iterations, GjkCache* cache)
{
	return cf_gjk(A, typeA, ax_ptr, B, typeB, bx_ptr, (CF_V2*)outA, (CF_V2*)outB, use_radius, iterations, cache);
}

CUTE_INLINE ToiResult toi(const void* A, ShapeType typeA, const Transform* ax_ptr, v2 vA, const void* B, ShapeType typeB, const Transform* bx_ptr, v2 vB, int use_radius, int* iterations)
{
	return cf_toi(A, typeA, ax_ptr, vA, B, typeB, bx_ptr, vB, use_radius);
}

CUTE_INLINE void inflate(void* shape, ShapeType type, float skin_factor) { return cf_inflate(shape, type, skin_factor); }

CUTE_INLINE int hull(v2* verts, int count) { return cf_hull((CF_V2*)verts, count); }
CUTE_INLINE void norms(v2* verts, v2* norms, int count) { return cf_norms((CF_V2*)verts, (CF_V2*)norms, count); }

CUTE_INLINE void make_poly(Poly* p) { return cf_make_poly(p); }
CUTE_INLINE v2 centroid(const v2* verts, int count) { return cf_centroid((CF_V2*)verts, count); }

CUTE_INLINE int collided(const void* A, const Transform* ax, ShapeType typeA, const void* B, const Transform* bx, ShapeType typeB) { return cf_collided(A, ax, typeA, B, bx, typeB); }
CUTE_INLINE void collide(const void* A, const Transform* ax, ShapeType typeA, const void* B, const Transform* bx, ShapeType typeB, Manifold* m) { return cf_collide(A, ax, typeA, B, bx, typeB, m); }
CUTE_INLINE bool cast_ray(Ray A, const void* B, const Transform* bx, ShapeType typeB, Raycast* out) { return cf_cast_ray(A, B, bx, typeB, out); }

}

#endif // CUTE_CPP

#endif // CUTE_MATH_H
