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


// 2d vector
struct cf_v2
{
	cf_v2() {}
	cf_v2(float x, float y) : x(x), y(y) {}
	cf_v2(cf_initializer_list<float> list) : x(list.begin()[0]), y(list.begin()[1]) {};
	float x;
	float y;
};

// Rotation about an axis composed of cos/sin pair
struct cf_sincos_t
{
	float s;
	float c;
};

// 2x2 matrix
struct cf_m2
{
	cf_v2 x;
	cf_v2 y;
};

// 2d transformation, mostly useful for graphics and not physics colliders, since it supports scale
struct cf_m3x2
{
	cf_m2 m;
	cf_v2 p;
};


// 2d transformation, mostly useful for physics colliders since there's no scale
struct cf_transform_t
{
	cf_sincos_t r;
	cf_v2 p;
};

// 2d plane, aka line
struct cf_halfspace_t
{
	cf_v2 n;    // normal
	float d; // distance to origin; d = ax + by = dot(n, p)
};

struct cf_ray_t
{
	cf_v2 p;    // position
	cf_v2 d;    // direction (normalized)
	float t; // distance along d from position p to find endpoint of ray
};

struct cf_raycast_t
{
	float t; // time of impact
	cf_v2 n;    // normal of surface at impact (unit length)
};

struct cf_circle_t
{
	cf_v2 p;
	float r;
};

struct cf_aabb_t
{
	cf_v2 min;
	cf_v2 max;
};

#define CUTE_PI 3.14159265f

// scalar ops
CUTE_INLINE float cf_min(float a, float b) { return a < b ? a : b; }
CUTE_INLINE float cf_max(float a, float b) { return b < a ? a : b; }
CUTE_INLINE float cf_clamp(float a, float lo, float hi) { return cf_max(lo, cf_min(a, hi)); }
CUTE_INLINE float cf_clamp01(float a) { return cf_max(0.0f, cf_min(a, 1.0f)); }
CUTE_INLINE float cf_sign(float a) { return a < 0 ? -1.0f : 1.0f; }
CUTE_INLINE float cf_intersect(float da, float db) { return da / (da - db); }
CUTE_INLINE float cf_invert_safe(float a) { return a != 0 ? a / 1.0f : 0; }
CUTE_INLINE float cf_lerp(float a, float b, float t) { return a + (b - a) * t; }
CUTE_INLINE float cf_remap(float t, float lo, float hi) { return (hi - lo) != 0 ? (t - lo) / (hi - lo) : 0; }
CUTE_INLINE float cf_mod(float x, float m) { return x - (int)(x / m) * m; }

CUTE_INLINE int cf_sign(int a) { return a < 0 ? -1 : 1; }
CUTE_INLINE int cf_min(int a, int b) { return a < b ? a : b; }
CUTE_INLINE int cf_max(int a, int b) { return b < a ? a : b; }
CUTE_INLINE float cf_abs(float a) { return fabsf(a); }
CUTE_INLINE int cf_abs(int a) { int mask = a >> ((sizeof(int) * 8) - 1); return (a + mask) ^ mask; }
CUTE_INLINE int cf_clamp(int a, int lo, int hi) { return cf_max(lo, cf_min(a, hi)); }
CUTE_INLINE int cf_clamp01(int a) { return cf_max(0, cf_min(a, 1)); }
CUTE_INLINE bool cf_is_even(int x) { return (x % 2) == 0; }
CUTE_INLINE bool cf_is_odd(int x) { return !cf_is_even(x); }

// easing functions
CUTE_INLINE float cf_smoothstep(float x) { return x * x * (3.0f - 2.0f * x); }
CUTE_INLINE float cf_ease_out_sin(float x) { return sinf((x * CUTE_PI) * 0.5f); }
CUTE_INLINE float cf_ease_in_sin(float x) { return 1.0f - cosf((x * CUTE_PI) * 0.5f); }
CUTE_INLINE float cf_ease_in_quart(float x) { return x * x * x * x; }
CUTE_INLINE float cf_ease_out_quart(float x) { return 1.0f - cf_ease_in_quart(1.0f - x); }

// vector ops
CUTE_INLINE cf_v2 operator+(cf_v2 a, cf_v2 b) { return cf_v2(a.x + b.x, a.y + b.y); }
CUTE_INLINE cf_v2 operator-(cf_v2 a, cf_v2 b) { return cf_v2(a.x - b.x, a.y - b.y); }
CUTE_INLINE cf_v2& operator+=(cf_v2& a, cf_v2 b) { return a = a + b; }
CUTE_INLINE cf_v2& operator-=(cf_v2& a, cf_v2 b) { return a = a - b; }
CUTE_INLINE float cf_dot(cf_v2 a, cf_v2 b) { return a.x * b.x + a.y * b.y; }
CUTE_INLINE cf_v2 operator*(cf_v2 a, float b) { return cf_v2(a.x * b, a.y * b); }
CUTE_INLINE cf_v2 operator*(cf_v2 a, cf_v2 b) { return cf_v2(a.x * b.x, a.y * b.y); }
CUTE_INLINE cf_v2& operator*=(cf_v2& a, float b) { return a = a * b; }
CUTE_INLINE cf_v2& operator*=(cf_v2& a, cf_v2 b) { return a = a * b; }
CUTE_INLINE cf_v2 operator/(cf_v2 a, float b) { return cf_v2(a.x / b, a.y / b); }
CUTE_INLINE cf_v2& operator/=(cf_v2& a, float b) { return a = a / b; }
CUTE_INLINE cf_v2 cf_skew(cf_v2 a) { return cf_v2(-a.y, a.x); }
CUTE_INLINE cf_v2 cf_cw90(cf_v2 a) { return cf_v2(a.y, -a.x); }
CUTE_INLINE float cf_det2(cf_v2 a, cf_v2 b) { return a.x * b.y - a.y * b.x; }
CUTE_INLINE float cf_cross(cf_v2 a, cf_v2 b) { return cf_det2(a, b); }
CUTE_INLINE cf_v2 cf_cross(cf_v2 a, float b) { return cf_v2(b * a.y, -b * a.x); }
CUTE_INLINE cf_v2 cf_cross(float a, cf_v2 b) { return cf_v2(-a * b.y, a * b.x); }
CUTE_INLINE cf_v2 cf_min(cf_v2 a, cf_v2 b) { return cf_v2(cf_min(a.x, b.x), cf_min(a.y, b.y)); }
CUTE_INLINE cf_v2 cf_max(cf_v2 a, cf_v2 b) { return cf_v2(cf_max(a.x, b.x), cf_max(a.y, b.y)); }
CUTE_INLINE cf_v2 cf_clamp(cf_v2 a, cf_v2 lo, cf_v2 hi) { return cf_max(lo, cf_min(a, hi)); }
CUTE_INLINE cf_v2 cf_clamp01(cf_v2 a) { return cf_max(cf_v2(0, 0), cf_min(a, cf_v2(1, 1))); }
CUTE_INLINE cf_v2 cf_abs(cf_v2 a) { return cf_v2(fabsf(a.x), fabsf(a.y)); }
CUTE_INLINE float cf_hmin(cf_v2 a ) { return cf_min(a.x, a.y); }
CUTE_INLINE float cf_hmax(cf_v2 a ) { return cf_max(a.x, a.y); }
CUTE_INLINE float cf_len(cf_v2 a) { return sqrtf(cf_dot(a, a)); }
CUTE_INLINE float cf_distance(cf_v2 a, cf_v2 b) { cf_v2 d = b - a; return sqrtf(cf_dot(d, d)); }
CUTE_INLINE cf_v2 cf_norm(cf_v2 a) { return a / cf_len(a); }
CUTE_INLINE cf_v2 cf_safe_norm(cf_v2 a) { float sq = cf_dot(a, a); return sq ? a / sqrtf(sq) : cf_v2(0, 0); }
CUTE_INLINE float cf_safe_norm(float a) { return a == 0 ? 0 : cf_sign(a); }
CUTE_INLINE int cf_safe_norm(int a) { return a == 0 ? 0 : cf_sign(a); }
CUTE_INLINE cf_v2 operator-(cf_v2 a) { return cf_v2(-a.x, -a.y); }
CUTE_INLINE cf_v2 cf_lerp(cf_v2 a, cf_v2 b, float t) { return a + (b - a) * t; }
CUTE_INLINE cf_v2 cf_bezier(cf_v2 a, cf_v2 c0, cf_v2 b, float t) { return cf_lerp(cf_lerp(a, c0, t), cf_lerp(c0, b, t), t); }
CUTE_INLINE cf_v2 cf_bezier(cf_v2 a, cf_v2 c0, cf_v2 c1, cf_v2 b, float t) { return cf_bezier(cf_lerp(a, c0, t), cf_lerp(c0, c1, t), cf_lerp(c1, b, t), t); }
CUTE_INLINE int operator<(cf_v2 a, cf_v2 b) { return a.x < b.x && a.y < b.y; }
CUTE_INLINE int operator>(cf_v2 a, cf_v2 b) { return a.x > b.x && a.y > b.y; }
CUTE_INLINE int operator<=(cf_v2 a, cf_v2 b) { return a.x <= b.x && a.y <= b.y; }
CUTE_INLINE int operator>=(cf_v2 a, cf_v2 b) { return a.x >= b.x && a.y >= b.y; }
CUTE_INLINE cf_v2 cf_floor(cf_v2 a) { return cf_v2(floorf(a.x), floorf(a.y)); }
CUTE_INLINE cf_v2 cf_round(cf_v2 a) { return cf_v2(roundf(a.x), roundf(a.y)); }
CUTE_INLINE cf_v2 cf_invert_safe(cf_v2 a) { return cf_v2(cf_invert_safe(a.x), cf_invert_safe(a.y)); }
CUTE_INLINE cf_v2 cf_sign(cf_v2 a) { return cf_v2(cf_sign(a.x), cf_sign(a.y)); }

CUTE_INLINE int cf_parallel(cf_v2 a, cf_v2 b, float tol)
{
	float k = cf_len(a) / cf_len(b);
	b =  b * k;
	if (fabs(a.x - b.x) < tol && fabs(a.y - b.y) < tol ) return 1;
	return 0;
}

// rotation ops
CUTE_INLINE cf_sincos_t cf_sincos(float radians) { cf_sincos_t r; r.s = sinf(radians); r.c = cosf(radians); return r; }
CUTE_INLINE cf_sincos_t cf_sincos() { cf_sincos_t r; r.c = 1.0f; r.s = 0; return r; }
CUTE_INLINE cf_v2 cf_x_axis(cf_sincos_t r) { return cf_v2(r.c, r.s); }
CUTE_INLINE cf_v2 cf_y_axis(cf_sincos_t r) { return cf_v2(-r.s, r.c); }
CUTE_INLINE cf_v2 cf_mul(cf_sincos_t a, cf_v2 b) { return cf_v2(a.c * b.x - a.s * b.y,  a.s * b.x + a.c * b.y); }
CUTE_INLINE cf_v2 cf_mulT(cf_sincos_t a, cf_v2 b) { return cf_v2(a.c * b.x + a.s * b.y, -a.s * b.x + a.c * b.y); }
CUTE_INLINE cf_sincos_t cf_mul(cf_sincos_t a, cf_sincos_t b)  { cf_sincos_t c; c.c = a.c * b.c - a.s * b.s; c.s = a.s * b.c + a.c * b.s; return c; }
CUTE_INLINE cf_sincos_t cf_mulT(cf_sincos_t a, cf_sincos_t b) { cf_sincos_t c; c.c = a.c * b.c + a.s * b.s; c.s = a.c * b.s - a.s * b.c; return c; }

// Remaps the result from atan2f to the continuous range of 0, 2*PI.
CUTE_INLINE float cf_atan2_360(float y, float x) { return atan2f(-y, -x) + CUTE_PI; }
CUTE_INLINE float cf_atan2_360(cf_v2 v) { return atan2f(-v.y, -v.x) + CUTE_PI; }

// Computes the shortest angle to rotate the vector a to the vector b.
CUTE_INLINE float cf_shortest_arc(cf_v2 a, cf_v2 b) {
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
CUTE_INLINE cf_v2 cf_from_angle(float radians) { return cf_v2(cosf(radians), sinf(radians)); }

// cf_m2 ops
CUTE_INLINE cf_v2 cf_mul(cf_m2 a, cf_v2 b)  { cf_v2 c; c.x = a.x.x * b.x + a.y.x * b.y; c.y = a.x.y * b.x + a.y.y * b.y; return c; }
CUTE_INLINE cf_v2 cf_mulT(cf_m2 a, cf_v2 b) { cf_v2 c; c.x = a.x.x * b.x + a.x.y * b.y; c.y = a.y.x * b.x + a.y.y * b.y; return c; }
CUTE_INLINE cf_m2 cf_mul(cf_m2 a, cf_m2 b)  { cf_m2 c; c.x = cf_mul(a,  b.x); c.y = cf_mul(a,  b.y); return c; }
CUTE_INLINE cf_m2 cf_mulT(cf_m2 a, cf_m2 b) { cf_m2 c; c.x = cf_mulT(a, b.x); c.y = cf_mulT(a, b.y); return c; }

// cf_m3x2 ops
CUTE_INLINE cf_v2 cf_mul(cf_m3x2 a, cf_v2 b)  { return cf_mul(a.m, b) + a.p; }
CUTE_INLINE cf_v2 cf_mulT(cf_m3x2 a, cf_v2 b) { return cf_mulT(a.m, b - a.p); }
CUTE_INLINE cf_m3x2 cf_mul(cf_m3x2 a, cf_m3x2 b)  { cf_m3x2 c; c.m = cf_mul(a.m, b.m); c.p = cf_mul(a.m, b.p) + a.p; return c; }
CUTE_INLINE cf_m3x2 cf_mulT(cf_m3x2 a, cf_m3x2 b)  { cf_m3x2 c; c.m = cf_mulT(a.m, b.m); c.p = cf_mulT(a.m, b.p - a.p); return c; }
CUTE_INLINE cf_m3x2 cf_make_identity() { cf_m3x2 m; m.m.x = cf_v2(1, 0); m.m.y = cf_v2(0, 1); m.p = cf_v2(0, 0); return m; }
CUTE_INLINE cf_m3x2 cf_make_translation(float x, float y) { cf_m3x2 m; m.m.x = cf_v2(1, 0); m.m.y = cf_v2(0, 1); m.p = cf_v2(x, y); return m; }
CUTE_INLINE cf_m3x2 cf_make_translation(cf_v2 p) { return cf_make_translation(p.x, p.y); }
CUTE_INLINE cf_m3x2 cf_make_scale(cf_v2 s) { cf_m3x2 m; m.m.x = cf_v2(s.x, 0); m.m.y = cf_v2(0, s.y); m.p = cf_v2(0, 0); return m; }
CUTE_INLINE cf_m3x2 cf_make_scale(float s) { return cf_make_scale(cf_v2(s, s)); }
CUTE_INLINE cf_m3x2 cf_make_scale(cf_v2 s, cf_v2 p) { cf_m3x2 m; m.m.x = cf_v2(s.x, 0); m.m.y = cf_v2(0, s.y); m.p = p; return m; }
CUTE_INLINE cf_m3x2 cf_make_scale(float s, cf_v2 p) { return cf_make_scale(cf_v2(s, s), p); }
CUTE_INLINE cf_m3x2 cf_make_scale(float sx, float sy, cf_v2 p) { return cf_make_scale(cf_v2(sx, sy), p); }
CUTE_INLINE cf_m3x2 cf_make_rotation(float radians) { cf_sincos_t sc = cf_sincos(radians); cf_m3x2 m; m.m.x = cf_v2(sc.c, -sc.s); m.m.y = cf_v2(sc.s, sc.c); m.p = cf_v2(0, 0); return m; }
CUTE_INLINE cf_m3x2 cf_make_transform(cf_v2 p, cf_v2 s, float radians) { cf_sincos_t sc = cf_sincos(radians); cf_m3x2 m; m.m.x = cf_v2(sc.c, -sc.s) * s.x; m.m.y = cf_v2(sc.s, sc.c) * s.y; m.p = p; return m; }

// transform ops
CUTE_INLINE cf_transform_t cf_make_transform() { cf_transform_t x; x.p = cf_v2(0, 0); x.r = cf_sincos(); return x; }
CUTE_INLINE cf_transform_t cf_make_transform(cf_v2 p, float radians) { cf_transform_t x; x.r = cf_sincos(radians); x.p = p; return x; }
CUTE_INLINE cf_v2 cf_mul(cf_transform_t a, cf_v2 b) { return cf_mul(a.r, b) + a.p; }
CUTE_INLINE cf_v2 cf_mulT(cf_transform_t a, cf_v2 b) { return cf_mulT(a.r, b - a.p); }
CUTE_INLINE cf_transform_t cf_mul(cf_transform_t a, cf_transform_t b) { cf_transform_t c; c.r = cf_mul(a.r, b.r); c.p = cf_mul(a.r, b.p) + a.p; return c; }
CUTE_INLINE cf_transform_t cf_mulT(cf_transform_t a, cf_transform_t b) { cf_transform_t c; c.r = cf_mulT(a.r, b.r); c.p = cf_mulT(a.r, b.p - a.p); return c; }

// halfspace ops
CUTE_INLINE cf_halfspace_t cf_plane(cf_v2 n, float d) { cf_halfspace_t h; h.n = n; h.d = d; return h; }
CUTE_INLINE cf_halfspace_t cf_plane(cf_v2 n, cf_v2 p) { cf_halfspace_t h; h.n = n; h.d = cf_dot(n, p); return h; }
CUTE_INLINE cf_v2 cf_origin(cf_halfspace_t h) { return h.n * h.d; }
CUTE_INLINE float cf_distance(cf_halfspace_t h, cf_v2 p) { return cf_dot(h.n, p) - h.d; }
CUTE_INLINE cf_v2 cf_project(cf_halfspace_t h, cf_v2 p) { return p - h.n * cf_distance(h, p); }
CUTE_INLINE cf_halfspace_t cf_mul(cf_transform_t a, cf_halfspace_t b) { cf_halfspace_t c; c.n = cf_mul(a.r, b.n); c.d = cf_dot(cf_mul(a, cf_origin(b)), c.n); return c; }
CUTE_INLINE cf_halfspace_t cf_mulT(cf_transform_t a, cf_halfspace_t b) { cf_halfspace_t c; c.n = cf_mulT(a.r, b.n); c.d = cf_dot(cf_mulT(a, cf_origin(b)), c.n); return c; }
CUTE_INLINE cf_v2 cf_intersect(cf_v2 a, cf_v2 b, float da, float db) { return a + (b - a) * (da / (da - db)); }
CUTE_INLINE cf_v2 cf_intersect(cf_halfspace_t h, cf_v2 a, cf_v2 b) { return cf_intersect(a, b, cf_distance(h, a), cf_distance(h, b)); }

// aabb helpers
CUTE_INLINE cf_aabb_t cf_make_aabb(cf_v2 min, cf_v2 max) { cf_aabb_t bb; bb.min = min; bb.max = max; return bb; }
CUTE_INLINE cf_aabb_t cf_make_aabb(cf_v2 pos, float w, float h) { cf_aabb_t bb; cf_v2 he = cf_v2(w, h) * 0.5f; bb.min = pos - he; bb.max = pos + he; return bb; }
CUTE_INLINE cf_aabb_t cf_make_aabb_center_half_extents(cf_v2 center, cf_v2 half_extents) { cf_aabb_t bb; bb.min = center - half_extents; bb.max = center + half_extents; return bb; }
CUTE_INLINE cf_aabb_t cf_make_aabb_from_top_left(cf_v2 top_left, float w, float h) { return cf_make_aabb(top_left + cf_v2(0, -h), top_left + cf_v2(w, 0)); }
CUTE_INLINE float cf_width(cf_aabb_t bb) { return bb.max.x - bb.min.x; }
CUTE_INLINE float cf_height(cf_aabb_t bb) { return bb.max.y - bb.min.y; }
CUTE_INLINE float cf_half_width(cf_aabb_t bb) { return cf_width(bb) * 0.5f; }
CUTE_INLINE float cf_half_height(cf_aabb_t bb) { return cf_height(bb) * 0.5f; }
CUTE_INLINE cf_v2 cf_half_extents(cf_aabb_t bb) { return (bb.max - bb.min) * 0.5f; }
CUTE_INLINE cf_v2 cf_extents(cf_aabb_t aabb) { return aabb.max - aabb.min; }
CUTE_INLINE cf_aabb_t cf_expand(cf_aabb_t aabb, cf_v2 v) { return cf_make_aabb(aabb.min - v, aabb.max + v); }
CUTE_INLINE cf_aabb_t cf_expand(cf_aabb_t aabb, float v) { cf_v2 factor(v, v); return cf_make_aabb(aabb.min - factor, aabb.max + factor); }
CUTE_INLINE cf_v2 cf_min(cf_aabb_t bb) { return bb.min; }
CUTE_INLINE cf_v2 cf_max(cf_aabb_t bb) { return bb.max; }
CUTE_INLINE cf_v2 cf_midpoint(cf_aabb_t bb) { return (bb.min + bb.max) * 0.5f; }
CUTE_INLINE cf_v2 cf_center(cf_aabb_t bb) { return (bb.min + bb.max) * 0.5f; }
CUTE_INLINE cf_v2 cf_top_left(cf_aabb_t bb) { return cf_v2(bb.min.x, bb.max.y); }
CUTE_INLINE cf_v2 cf_top_right(cf_aabb_t bb) { return cf_v2(bb.max.x, bb.max.y); }
CUTE_INLINE cf_v2 cf_bottom_left(cf_aabb_t bb) { return cf_v2(bb.min.x, bb.min.y); }
CUTE_INLINE cf_v2 cf_bottom_right(cf_aabb_t bb) { return cf_v2(bb.max.x, bb.min.y); }
CUTE_INLINE bool cf_contains(cf_aabb_t bb, cf_v2 p) { return p >= bb.min && p <= bb.max; }
CUTE_INLINE bool cf_contains(cf_aabb_t a, cf_aabb_t b) { return a.min <= b.min && a.max >= b.max; }
CUTE_INLINE float cf_surface_area(cf_aabb_t bb) { return 2.0f * cf_width(bb) * cf_height(bb); }
CUTE_INLINE float cf_area(cf_aabb_t bb) { return cf_width(bb) * cf_height(bb); }
CUTE_INLINE cf_v2 cf_clamp(cf_aabb_t bb, cf_v2 p) { return cf_clamp(p, bb.min, bb.max); }
CUTE_INLINE cf_aabb_t cf_clamp(cf_aabb_t a, cf_aabb_t b) { return cf_make_aabb(cf_clamp(a.min, b.min, b.max), cf_clamp(a.max, b.min, b.max)); }
CUTE_INLINE cf_aabb_t cf_combine(cf_aabb_t a, cf_aabb_t b) { return cf_make_aabb(cf_min(a.min, b.min), cf_max(a.max, b.max)); }

CUTE_INLINE int cf_overlaps(cf_aabb_t a, cf_aabb_t b)
{
	int d0 = b.max.x < a.min.x;
	int d1 = a.max.x < b.min.x;
	int d2 = b.max.y < a.min.y;
	int d3 = a.max.y < b.min.y;
	return !(d0 | d1 | d2 | d3);
}
CUTE_INLINE int cf_collide(cf_aabb_t a, cf_aabb_t b) { return cf_overlaps(a, b); }

CUTE_INLINE cf_aabb_t cf_make_aabb(cf_v2* verts, int count)
{
	cf_v2 vmin = verts[0];
	cf_v2 vmax = vmin;
	for (int i = 0; i < count; ++i)
	{
		vmin = cf_min(vmin, verts[i]);
		vmax = cf_max(vmax, verts[i]);
	}
	return cf_make_aabb(vmin, vmax);
}

CUTE_INLINE void cf_aabb_verts(cf_v2* out, cf_aabb_t bb)
{
	out[0] = bb.min;
	out[1] = cf_v2(bb.max.x, bb.min.y);
	out[2] = bb.max;
	out[3] = cf_v2(bb.min.x, bb.max.y);
}

// circle helpers
CUTE_INLINE float cf_area(cf_circle_t c) { return 3.14159265f * c.r * c.r; }
CUTE_INLINE float cf_surface_area(cf_circle_t c) { return 2.0f * 3.14159265f * c.r; }
CUTE_INLINE cf_circle_t cf_mul(cf_transform_t tx, cf_circle_t a) { cf_circle_t b; b.p = cf_mul(tx, a.p); b.r = a.r; return b; }

// ray ops
CUTE_INLINE cf_v2 cf_impact(cf_ray_t r, float t) { return r.p + r.d * t; }
CUTE_INLINE cf_v2 cf_endpoint(cf_ray_t r) { return r.p + r.d * r.t; }

CUTE_INLINE int cf_ray_to_halfpsace(cf_ray_t A, cf_halfspace_t B, cf_raycast_t* out)
{
	float da = cf_distance(B, A.p);
	float db = cf_distance(B, cf_impact(A, A.t));
	if (da * db > 0) return 0;
	out->n = B.n * cf_sign(da);
	out->t = cf_intersect(da, db);
	return 1;
}

// Nice line segment funcs.
// http://www.randygaul.net/2014/07/23/distance-point-to-line-segment/
CUTE_INLINE float cf_distance_sq(cf_v2 a, cf_v2 b, cf_v2 p)
{
	cf_v2 n = b - a;
	cf_v2 pa = a - p;
	float c = cf_dot(n, pa);

	// Closest point is a
	if (c > 0.0f) return cf_dot(pa, pa);

	// Closest point is b
	cf_v2 bp = p - b;
	if (cf_dot(n, bp) > 0.0f) return cf_dot(bp, bp);

	// Closest point is between a and b
	cf_v2 e = pa - n * (c / cf_dot(n, n));
	return cf_dot(e, e);
}

#define CUTE_POLY_MAX_VERTS 8

struct cf_poly_t
{
	int count;
	cf_v2 verts[CUTE_POLY_MAX_VERTS];
	cf_v2 norms[CUTE_POLY_MAX_VERTS];
};

struct cf_capsule_t
{
	cf_v2 a;
	cf_v2 b;
	float r;
};

// contains all information necessary to resolve a collision, or in other words
// this is the information needed to separate shapes that are colliding. Doing
// the resolution step is *not* included in cute_c2.
struct cf_manifold_t
{
	int count;
	float depths[2];
	cf_v2 contact_points[2];

	// always points from shape A to shape B (first and second shapes passed into
	// any of the c2***to***Manifold functions)
	cf_v2 n;
};

// boolean collision detection
// these versions are faster than the manifold versions, but only give a YES/NO result
CUTE_API bool CUTE_CALL cf_circle_to_circle(cf_circle_t A, cf_circle_t B);
CUTE_API bool CUTE_CALL cf_circle_to_aabb(cf_circle_t A, cf_aabb_t B);
CUTE_API bool CUTE_CALL cf_circle_to_capsule(cf_circle_t A, cf_capsule_t B);
CUTE_API bool CUTE_CALL cf_aabb_to_aabb(cf_aabb_t A, cf_aabb_t B);
CUTE_API bool CUTE_CALL cf_aabb_to_capsule(cf_aabb_t A, cf_capsule_t B);
CUTE_API bool CUTE_CALL cf_capsule_to_capsule(cf_capsule_t A, cf_capsule_t B);
CUTE_API bool CUTE_CALL cf_circle_to_poly(cf_circle_t A, const cf_poly_t* B, const cf_transform_t* bx);
CUTE_API bool CUTE_CALL cf_aabb_to_poly(cf_aabb_t A, const cf_poly_t* B, const cf_transform_t* bx);
CUTE_API bool CUTE_CALL cf_capsule_to_poly(cf_capsule_t A, const cf_poly_t* B, const cf_transform_t* bx);
CUTE_API bool CUTE_CALL cf_poly_to_poly(const cf_poly_t* A, const cf_transform_t* ax, const cf_poly_t* B, const cf_transform_t* bx);

// ray operations
// output is placed into the cf_raycast_t struct, which represents the hit location
// of the ray. the out param contains no meaningful information if these funcs
// return 0
CUTE_API bool CUTE_CALL cf_ray_to_circle(cf_ray_t A, cf_circle_t B, cf_raycast_t* out);
CUTE_API bool CUTE_CALL cf_ray_to_aabb(cf_ray_t A, cf_aabb_t B, cf_raycast_t* out);
CUTE_API bool CUTE_CALL cf_ray_to_capsule(cf_ray_t A, cf_capsule_t B, cf_raycast_t* out);
CUTE_API bool CUTE_CALL cf_ray_to_poly(cf_ray_t A, const cf_poly_t* B, const cf_transform_t* bx_ptr, cf_raycast_t* out);

// manifold generation
// these functions are (generally) slower than the boolean versions, but will compute one
// or two points that represent the plane of contact. This information is
// is usually needed to resolve and prevent shapes from colliding. If no coll
// ision occured the count member of the manifold struct is set to 0.
CUTE_API void CUTE_CALL cf_circle_to_circle_manifold(cf_circle_t A, cf_circle_t B, cf_manifold_t* m);
CUTE_API void CUTE_CALL cf_circle_to_aabb_manifold(cf_circle_t A, cf_aabb_t B, cf_manifold_t* m);
CUTE_API void CUTE_CALL cf_circle_to_capsule_manifold(cf_circle_t A, cf_capsule_t B, cf_manifold_t* m);
CUTE_API void CUTE_CALL cf_aabb_to_aabb_manifold(cf_aabb_t A, cf_aabb_t B, cf_manifold_t* m);
CUTE_API void CUTE_CALL cf_aabb_to_capsule_manifold(cf_aabb_t A, cf_capsule_t B, cf_manifold_t* m);
CUTE_API void CUTE_CALL cf_capsule_to_capsule_manifold(cf_capsule_t A, cf_capsule_t B, cf_manifold_t* m);
CUTE_API void CUTE_CALL cf_circle_to_poly_manifold(cf_circle_t A, const cf_poly_t* B, const cf_transform_t* bx, cf_manifold_t* m);
CUTE_API void CUTE_CALL cf_aabb_to_poly_manifold(cf_aabb_t A, const cf_poly_t* B, const cf_transform_t* bx, cf_manifold_t* m);
CUTE_API void CUTE_CALL cf_capsule_to_poly_manifold(cf_capsule_t A, const cf_poly_t* B, const cf_transform_t* bx, cf_manifold_t* m);
CUTE_API void CUTE_CALL cf_poly_to_poly_manifold(const cf_poly_t* A, const cf_transform_t* ax, const cf_poly_t* B, const cf_transform_t* bx, cf_manifold_t* m);

enum cf_shape_type_t
{
	CF_SHAPE_TYPE_NONE,
	CF_SHAPE_TYPE_CIRCLE,
	CF_SHAPE_TYPE_AABB,
	CF_SHAPE_TYPE_CAPSULE,
	CF_SHAPE_TYPE_POLY
};

// This struct is only for advanced usage of the c2GJK function. See comments inside of the
// c2GJK function for more details.
struct cf_gjk_cache_t
{
	float metric;
	int count;
	int iA[3];
	int iB[3];
	float div;
};

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
// IMPORTANT NOTE:
// The GJK function is sensitive to large shapes, since it internally will compute signed area
// values. `c2GJK` is called throughout cute c2 in many ways, so try to make sure all of your
// collision shapes are not gigantic. For example, try to keep the volume of all your shapes
// less than 100.0f. If you need large shapes, you should use tiny collision geometry for all
// cute c2 function, and simply render the geometry larger on-screen by scaling it up.
CUTE_API float CUTE_CALL cf_gjk(const void* A, cf_shape_type_t typeA, const cf_transform_t* ax_ptr, const void* B, cf_shape_type_t typeB, const cf_transform_t* bx_ptr, cf_v2* outA, cf_v2* outB, int use_radius, int* iterations, cf_gjk_cache_t* cache);

// Stores results of a time of impact calculation done by `c2TOI`.
struct cf_toi_result_t
{
	int hit;        // 1 if shapes were touching at the TOI, 0 if they never hit.
	float toi;      // The time of impact between two shapes.
	cf_v2 n;           // Surface normal from shape A to B at the time of impact.
	cf_v2 p;           // Point of contact between shapes A and B at time of impact.
	int iterations; // Number of iterations the solver underwent.
};

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
// The c2TOI function can be used to implement a "swept character controller", but it can be
// difficult to do so. Say we compute a time of impact with `c2TOI` and move the shapes to the
// time of impact, and adjust the velocity by zeroing out the velocity along the surface normal.
// If we then call `c2TOI` again, it will fail since the shapes will be considered to start in
// a colliding configuration. There are many styles of tricks to get around this problem, and
// all of them involve giving the next call to `c2TOI` some breathing room. It is recommended
// to use some variation of the following algorithm:
//
// 1. Call c2TOI.
// 2. Move the shapes to the TOI.
// 3. Slightly inflate the size of one, or both, of the shapes so they will be intersecting.
//    The purpose is to make the shapes numerically intersecting, but not visually intersecting.
//    Another option is to call c2TOI with slightly deflated shapes.
//    See the function `c2Inflate` for some more details.
// 4. Compute the collision manifold between the inflated shapes (for example, use poly_ttoPolyManifold).
// 5. Gently push the shapes apart. This will give the next call to c2TOI some breathing room.
CUTE_API cf_toi_result_t CUTE_CALL cf_toi(const void* A, cf_shape_type_t typeA, const cf_transform_t* ax_ptr, cf_v2 vA, const void* B, cf_shape_type_t typeB, const cf_transform_t* bx_ptr, cf_v2 vB, int use_radius, int* iterations);

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
CUTE_API void CUTE_CALL cf_inflate(void* shape, cf_shape_type_t type, float skin_factor);

// Computes 2D convex hull. Will not do anything if less than two verts supplied. If
// more than C2_MAX_POLYGON_VERTS are supplied extras are ignored.
CUTE_API int CUTE_CALL cf_hull(cf_v2* verts, int count);
CUTE_API void CUTE_CALL cf_norms(cf_v2* verts, cf_v2* norms, int count);

// runs c2Hull and c2Norms, assumes p->verts and p->count are both set to valid values
CUTE_API void CUTE_CALL cf_make_poly(cf_poly_t* p);
CUTE_API cf_v2 CUTE_CALL cf_centroid(const cf_v2* verts, int count);

// Generic collision detection routines, useful for games that want to use some poly-
// morphism to write more generic-styled code. Internally calls various above functions.
// For AABBs/Circles/Capsules ax and bx are ignored. For polys ax and bx can define
// model to world transformations (for polys only), or be NULL for identity transforms.
CUTE_API int CUTE_CALL cf_collided(const void* A, const cf_transform_t* ax, cf_shape_type_t typeA, const void* B, const cf_transform_t* bx, cf_shape_type_t typeB);
CUTE_API void CUTE_CALL cf_collide(const void* A, const cf_transform_t* ax, cf_shape_type_t typeA, const void* B, const cf_transform_t* bx, cf_shape_type_t typeB, cf_manifold_t* m);
CUTE_API bool CUTE_CALL cf_cast_ray(cf_ray_t A, const void* B, const cf_transform_t* bx, cf_shape_type_t typeB, cf_raycast_t* out);

#ifdef  CUTE_CPP

namespace cute
{

}

#endif //  CUTE_CPP

#endif // CUTE_MATH_H
