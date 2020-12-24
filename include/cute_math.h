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

#include <cute_defines.h>

#include <initializer_list>
#include <math.h>

namespace cute
{

// 2d vector
struct v2
{
	v2() {}
	v2(float x, float y) : x(x), y(y) {}
	v2(std::initializer_list<float> list) : x(list.begin()[0]), y(list.begin()[1]) {};
	float x;
	float y;
};

// Rotation about an axis composed of cos/sin pair
struct sincos_t
{
	float s;
	float c;
};

// 2x2 matrix
struct m2
{
	v2 x;
	v2 y;
};

// 2d transformation, mostly useful for graphics and not physics colliders, since it supports scale
struct m3x2
{
	m2 m;
	v2 p;
};


// 2d transformation, mostly useful for physics colliders since there's no scale
struct transform_t
{
	sincos_t r;
	v2 p;
};

// 2d plane, aka line
struct halfspace_t
{
	v2 n;    // normal
	float d; // distance to origin; d = ax + by = dot(n, p)
};

struct ray_t
{
	v2 p;    // position
	v2 d;    // direction (normalized)
	float t; // distance along d from position p to find endpoint of ray
};

struct raycast_t
{
	float t; // time of impact
	v2 n;    // normal of surface at impact (unit length)
};

struct circle_t
{
	float r;
	v2 p;
};

struct aabb_t
{
	v2 min;
	v2 max;
};

#define CUTE_PI 3.14159265f

// scalar ops
CUTE_INLINE float min(float a, float b) { return a < b ? a : b; }
CUTE_INLINE float max(float a, float b) { return b < a ? a : b; }
CUTE_INLINE float clamp(float a, float lo, float hi) { return max(lo, min(a, hi)); }
CUTE_INLINE float clamp01(float a) { return max(0.0f, min(a, 1.0f)); }
CUTE_INLINE float sign(float a) { return a < 0 ? -1.0f : 1.0f; }
CUTE_INLINE float intersect(float da, float db) { return da / (da - db); }
CUTE_INLINE float invert_safe(float a) { return a != 0 ? a / 1.0f : 0; }
CUTE_INLINE float lerp(float a, float b, float t) { return a + (b - a) * t; }
CUTE_INLINE float remap(float t, float lo, float hi) { return (hi - lo) != 0 ? (t - lo) / (hi - lo) : 0; }
CUTE_INLINE float mod(float x, float m) { return x - (int)(x / m) * m; }
CUTE_INLINE int sign(int a) { return a < 0 ? -1 : 1; }
CUTE_INLINE int min(int a, int b) { return a < b ? a : b; }
CUTE_INLINE int max(int a, int b) { return b < a ? a : b; }
CUTE_INLINE int abs(int a) { int mask = a >> ((sizeof(int) * 8) - 1); return (a + mask) ^ mask; }
CUTE_INLINE int clamp(int a, int lo, int hi) { return max(lo, min(a, hi)); }
CUTE_INLINE int clamp01(int a) { return max(0, min(a, 1)); }
CUTE_INLINE bool is_even(int x) { return (x % 2) == 0; }
CUTE_INLINE bool is_odd(int x) { return !is_even(x); }

// easing functions
CUTE_INLINE float smoothstep(float x) { return x * x * (3.0f - 2.0f * x); }
CUTE_INLINE float ease_out_sin(float x) { return sinf((x * CUTE_PI) * 0.5f); }
CUTE_INLINE float ease_in_sin(float x) { return 1.0f - cosf((x * CUTE_PI) * 0.5f); }
CUTE_INLINE float ease_in_quart(float x) { return x * x * x * x; }
CUTE_INLINE float ease_out_quart(float x) { return 1.0f - ease_in_quart(1.0f - x); }

// vector ops
CUTE_INLINE v2 operator+(v2 a, v2 b) { return v2(a.x + b.x, a.y + b.y); }
CUTE_INLINE v2 operator-(v2 a, v2 b) { return v2(a.x - b.x, a.y - b.y); }
CUTE_INLINE v2& operator+=(v2& a, v2 b) { return a = a + b; }
CUTE_INLINE v2& operator-=(v2& a, v2 b) { return a = a - b; }
CUTE_INLINE float dot(v2 a, v2 b) { return a.x * b.x + a.y * b.y; }
CUTE_INLINE v2 operator*(v2 a, float b) { return v2(a.x * b, a.y * b); }
CUTE_INLINE v2 operator*(v2 a, v2 b) { return v2(a.x * b.x, a.y * b.y); }
CUTE_INLINE v2& operator*=(v2& a, float b) { return a = a * b; }
CUTE_INLINE v2& operator*=(v2& a, v2 b) { return a = a * b; }
CUTE_INLINE v2 operator/(v2 a, float b) { return v2(a.x / b, a.y / b); }
CUTE_INLINE v2& operator/=(v2& a, float b) { return a = a / b; }
CUTE_INLINE v2 skew(v2 a) { return v2(-a.y, a.x); }
CUTE_INLINE v2 ccw90(v2 a) { return v2(a.y, -a.x); }
CUTE_INLINE float det2(v2 a, v2 b) { return a.x * b.y - a.y * b.x; }
CUTE_INLINE v2 min(v2 a, v2 b) { return v2(min(a.x, b.x), min(a.y, b.y)); }
CUTE_INLINE v2 max(v2 a, v2 b) { return v2(max(a.x, b.x), max(a.y, b.y)); }
CUTE_INLINE v2 clamp(v2 a, v2 lo, v2 hi) { return max(lo, min(a, hi)); }
CUTE_INLINE v2 clamp01(v2 a) { return max(v2(0, 0), min(a, v2(1, 1))); }
CUTE_INLINE v2 abs(v2 a) { return v2(fabsf(a.x), fabsf(a.y)); }
CUTE_INLINE float hmin(v2 a ) { return min(a.x, a.y); }
CUTE_INLINE float hmax(v2 a ) { return max(a.x, a.y); }
CUTE_INLINE float len(v2 a) { return sqrtf(dot(a, a)); }
CUTE_INLINE float distance(v2 a, v2 b) { return sqrtf(powf((a.x - b.x), 2) + powf((a.y - b.y), 2)); }
CUTE_INLINE v2 norm(v2 a) { return a / len(a); }
CUTE_INLINE v2 safe_norm(v2 a) { float sq = dot(a, a); return sq ? a / sqrtf(sq) : v2(0, 0); }
CUTE_INLINE float safe_norm(float a) { return a == 0 ? 0 : sign(a); }
CUTE_INLINE int safe_norm(int a) { return a == 0 ? 0 : sign(a); }
CUTE_INLINE v2 operator-(v2 a) { return v2(-a.x, -a.y); }
CUTE_INLINE v2 lerp(v2 a, v2 b, float t) { return a + (b - a) * t; }
CUTE_INLINE v2 bezier(v2 a, v2 c0, v2 b, float t) { return lerp(lerp(a, c0, t), lerp(c0, b, t), t); }
CUTE_INLINE v2 bezier(v2 a, v2 c0, v2 c1, v2 b, float t) { return bezier(lerp(a, c0, t), lerp(c0, c1, t), lerp(c1, b, t), t); }
CUTE_INLINE int operator<(v2 a, v2 b) { return a.x < b.x && a.y < b.y; }
CUTE_INLINE int operator>(v2 a, v2 b) { return a.x > b.x && a.y > b.y; }
CUTE_INLINE int operator<=(v2 a, v2 b) { return a.x <= b.x && a.y <= b.y; }
CUTE_INLINE int operator>=(v2 a, v2 b) { return a.x >= b.x && a.y >= b.y; }
CUTE_INLINE v2 floor(v2 a) { return v2(floorf(a.x), floorf(a.y)); }
CUTE_INLINE v2 round(v2 a) { return v2(roundf(a.x), roundf(a.y)); }
CUTE_INLINE v2 invert_safe(v2 a) { return v2(invert_safe(a.x), invert_safe(a.y)); }

CUTE_INLINE int parallel(v2 a, v2 b, float tol)
{
	float k = len(a) / len(b);
	b =  b * k;
	if (fabs(a.x - b.x) < tol && fabs(a.y - b.y) < tol ) return 1;
	return 0;
}

// rotation ops
CUTE_INLINE sincos_t sincos(float radians) { sincos_t r; r.s = sinf(radians); r.c = cosf(radians); return r; }
CUTE_INLINE sincos_t sincos() { sincos_t r; r.c = 1.0f; r.s = 0; return r; }
CUTE_INLINE v2 x_axis(sincos_t r) { return v2(r.c, r.s); }
CUTE_INLINE v2 y_axis(sincos_t r) { return v2(-r.s, r.c); }
CUTE_INLINE v2 mul(sincos_t a, v2 b) { return v2(a.c * b.x - a.s * b.y,  a.s * b.x + a.c * b.y); }
CUTE_INLINE v2 mulT(sincos_t a, v2 b) { return v2(a.c * b.x + a.s * b.y, -a.s * b.x + a.c * b.y); }
CUTE_INLINE sincos_t mul(sincos_t a, sincos_t b)  { sincos_t c; c.c = a.c * b.c - a.s * b.s; c.s = a.s * b.c + a.c * b.s; return c; }
CUTE_INLINE sincos_t mulT(sincos_t a, sincos_t b) { sincos_t c; c.c = a.c * b.c + a.s * b.s; c.s = a.c * b.s - a.s * b.c; return c; }

// Remaps the result from atan2f to the continuous range of 0, 2*PI.
CUTE_INLINE float atan2_360(float y, float x) { return atan2f(-y, -x) + CUTE_PI; }

// Computes the shortest angle to rotate the vector a to the vector b.
CUTE_INLINE float shortest_arc(v2 a, v2 b) {
	float c = dot(a, b);
	float s = det2(a, b);
	float theta = acosf(c);
	if (s > 0) {
		return theta;
	} else {
		return -theta;
	}
}

CUTE_INLINE float angle_diff(float radians_a, float radians_b) { return mod((radians_b - radians_a) + CUTE_PI, 2.0f * CUTE_PI) - CUTE_PI; }
CUTE_INLINE v2 from_angle(float radians) { return v2(cosf(radians), sinf(radians)); }

// m2 ops
CUTE_INLINE v2 mul(m2 a, v2 b)  { v2 c; c.x = a.x.x * b.x + a.y.x * b.y; c.y = a.x.y * b.x + a.y.y * b.y; return c; }
CUTE_INLINE v2 mulT(m2 a, v2 b) { v2 c; c.x = a.x.x * b.x + a.x.y * b.y; c.y = a.y.x * b.x + a.y.y * b.y; return c; }
CUTE_INLINE m2 mul(m2 a, m2 b)  { m2 c; c.x = mul(a,  b.x); c.y = mul(a,  b.y); return c; }
CUTE_INLINE m2 mulT(m2 a, m2 b) { m2 c; c.x = mulT(a, b.x); c.y = mulT(a, b.y); return c; }

// m3x2 ops
CUTE_INLINE v2 mul(m3x2 a, v2 b)  { return mul(a.m, b) + a.p; }
CUTE_INLINE v2 mulT(m3x2 a, v2 b) { return mulT(a.m, b - a.p); }
CUTE_INLINE m3x2 mul(m3x2 a, m3x2 b)  { m3x2 c; c.m = mul(a.m, b.m); c.p = mul(a.m, b.p) + a.p; return c; }
CUTE_INLINE m3x2 mulT(m3x2 a, m3x2 b)  { m3x2 c; c.m = mulT(a.m, b.m); c.p = mulT(a.m, b.p - a.p); return c; }
CUTE_INLINE m3x2 make_identity() { m3x2 m; m.m.x = v2(1, 0); m.m.y = v2(0, 1); m.p = v2(0, 0); return m; }
CUTE_INLINE m3x2 make_translation(float x, float y) { m3x2 m; m.m.x = v2(1, 0); m.m.y = v2(0, 1); m.p = v2(x, y); return m; }
CUTE_INLINE m3x2 make_translation(v2 p) { return make_translation(p.x, p.y); }
CUTE_INLINE m3x2 make_scale(v2 s) { m3x2 m; m.m.x = v2(s.x, 0); m.m.y = v2(0, s.y); m.p = v2(0, 0); return m; }
CUTE_INLINE m3x2 make_scale(float s) { return make_scale(v2(s, s)); }
CUTE_INLINE m3x2 make_scale(v2 s, v2 p) { m3x2 m; m.m.x = v2(s.x, 0); m.m.y = v2(0, s.y); m.p = p; return m; }
CUTE_INLINE m3x2 make_scale(float s, v2 p) { return make_scale(v2(s, s), p); }
CUTE_INLINE m3x2 make_scale(float sx, float sy, v2 p) { return make_scale(v2(sx, sy), p); }
CUTE_INLINE m3x2 make_rotation(float radians) { sincos_t sc = sincos(radians); m3x2 m; m.m.x = v2(sc.c, -sc.s); m.m.y = v2(sc.s, sc.c); m.p = v2(0, 0); return m; }
CUTE_INLINE m3x2 make_transform(v2 p, v2 s, float radians) { sincos_t sc = sincos(radians); m3x2 m; m.m.x = v2(sc.c, -sc.s) * s.x; m.m.y = v2(sc.s, sc.c) * s.y; m.p = p; return m; }

// transform ops
CUTE_INLINE transform_t make_transform() { transform_t x; x.p = v2(0, 0); x.r = sincos(); return x; }
CUTE_INLINE transform_t make_transform(v2 p, float radians) { transform_t x; x.r = sincos(radians); x.p = p; return x; }
CUTE_INLINE v2 mul(transform_t a, v2 b) { return mul(a.r, b) + a.p; }
CUTE_INLINE v2 mulT(transform_t a, v2 b) { return mulT(a.r, b - a.p); }
CUTE_INLINE transform_t mul(transform_t a, transform_t b) { transform_t c; c.r = mul(a.r, b.r); c.p = mul(a.r, b.p) + a.p; return c; }
CUTE_INLINE transform_t mulT(transform_t a, transform_t b) { transform_t c; c.r = mulT(a.r, b.r); c.p = mulT(a.r, b.p - a.p); return c; }

// halfspace ops
CUTE_INLINE v2 origin(halfspace_t h) { return h.n * h.d; }
CUTE_INLINE float distance(halfspace_t h, v2 p) { return dot(h.n, p) - h.d; }
CUTE_INLINE v2 project(halfspace_t h, v2 p) { return p - h.n * distance(h, p); }
CUTE_INLINE halfspace_t mul(transform_t a, halfspace_t b) { halfspace_t c; c.n = mul(a.r, b.n); c.d = dot(mul(a, origin(b)), c.n); return c; }
CUTE_INLINE halfspace_t mulT(transform_t a, halfspace_t b) { halfspace_t c; c.n = mulT(a.r, b.n); c.d = dot(mulT(a, origin(b)), c.n); return c; }
CUTE_INLINE v2 intersect(v2 a, v2 b, float da, float db) { return a + (b - a) * (da / (da - db)); }

// aabb helpers
CUTE_INLINE aabb_t make_aabb(v2 min, v2 max) { aabb_t bb; bb.min = min; bb.max = max; return bb; }
CUTE_INLINE aabb_t make_aabb(v2 pos, float w, float h) { aabb_t bb; v2 he = v2(w, h) * 0.5f; bb.min = pos - he; bb.max = pos + he; return bb; }
CUTE_INLINE aabb_t make_aabb_center_half_extents(v2 center, v2 half_extents) { aabb_t bb; bb.min = center - half_extents; bb.max = center + half_extents; return bb; }
CUTE_INLINE aabb_t make_aabb_from_top_left(v2 top_left, float w, float h) { return make_aabb(top_left + v2(0, -h), top_left + v2(w, 0)); }
CUTE_INLINE float width(aabb_t bb) { return bb.max.x - bb.min.x; }
CUTE_INLINE float height(aabb_t bb) { return bb.max.y - bb.min.y; }
CUTE_INLINE float half_width(aabb_t bb) { return width(bb) * 0.5f; }
CUTE_INLINE float half_height(aabb_t bb) { return height(bb) * 0.5f; }
CUTE_INLINE v2 half_extents(aabb_t bb) { return (bb.max - bb.min) * 0.5f; }
CUTE_INLINE v2 extents(aabb_t aabb) { return aabb.max - aabb.min; }
CUTE_INLINE aabb_t expand(aabb_t aabb, v2 v) { return make_aabb(aabb.min - v, aabb.max + v); }
CUTE_INLINE aabb_t expand(aabb_t aabb, float v) { v2 factor(v, v); return make_aabb(aabb.min - factor, aabb.max + factor); }
CUTE_INLINE v2 min(aabb_t bb) { return bb.min; }
CUTE_INLINE v2 max(aabb_t bb) { return bb.max; }
CUTE_INLINE v2 midpoint(aabb_t bb) { return (bb.min + bb.max) * 0.5f; }
CUTE_INLINE v2 center(aabb_t bb) { return (bb.min + bb.max) * 0.5f; }
CUTE_INLINE v2 top_left(aabb_t bb) { return v2(bb.min.x, bb.max.y); }
CUTE_INLINE v2 top_right(aabb_t bb) { return v2(bb.max.x, bb.max.y); }
CUTE_INLINE v2 bottom_left(aabb_t bb) { return v2(bb.min.x, bb.min.y); }
CUTE_INLINE v2 bottom_right(aabb_t bb) { return v2(bb.max.x, bb.min.y); }
CUTE_INLINE bool contains(aabb_t bb, v2 p) { return p >= bb.min && p <= bb.max; }
CUTE_INLINE bool contains(aabb_t a, aabb_t b) { return a.min <= b.min && a.max >= b.max; }
CUTE_INLINE float surface_area(aabb_t bb) { return 2.0f * width(bb) * height(bb); }
CUTE_INLINE float area(aabb_t bb) { return width(bb) * height(bb); }
CUTE_INLINE v2 clamp(aabb_t bb, v2 p) { return clamp(p, bb.min, bb.max); }
CUTE_INLINE aabb_t clamp(aabb_t a, aabb_t b) { return make_aabb(clamp(a.min, b.min, b.max), clamp(a.max, b.min, b.max)); }
CUTE_INLINE aabb_t combine(aabb_t a, aabb_t b) { return make_aabb(min(a.min, b.min), max(a.max, b.max)); }

CUTE_INLINE int overlaps(aabb_t a, aabb_t b)
{
	int d0 = b.max.x < a.min.x;
	int d1 = a.max.x < b.min.x;
	int d2 = b.max.y < a.min.y;
	int d3 = a.max.y < b.min.y;
	return !(d0 | d1 | d2 | d3);
}
CUTE_INLINE int collide(aabb_t a, aabb_t b) { return overlaps(a, b); }

CUTE_INLINE aabb_t make_aabb(v2* verts, int count)
{
	v2 vmin = verts[0];
	v2 vmax = vmin;
	for (int i = 0; i < count; ++i)
	{
		vmin = min(vmin, verts[i]);
		vmax = max(vmax, verts[i]);
	}
	return make_aabb(vmin, vmax);
}

CUTE_INLINE void aabb_verts(v2* out, aabb_t bb)
{
	out[0] = bb.min;
	out[1] = v2(bb.max.x, bb.min.y);
	out[2] = bb.max;
	out[3] = v2(bb.min.x, bb.max.y);
}

// circle helpers
CUTE_INLINE float area(circle_t c) { return 3.14159265f * c.r * c.r; }
CUTE_INLINE float surface_area(circle_t c) { return 2.0f * 3.14159265f * c.r; }
CUTE_INLINE circle_t mul(transform_t tx, circle_t a) { circle_t b; b.p = mul(tx, a.p); b.r = a.r; return b; }

// ray ops
CUTE_INLINE v2 impact(ray_t r, float t) { return r.p + r.d * t; }
CUTE_INLINE v2 endpoint(ray_t r) { return r.p + r.d * r.t; }

CUTE_INLINE int ray_to_halfpsace(ray_t A, halfspace_t B, raycast_t* out)
{
	float da = distance(B, A.p);
	float db = distance(B, impact(A, A.t));
	if (da * db > 0) return 0;
	out->n = B.n * sign(da);
	out->t = intersect(da, db);
	return 1;
}

#define CUTE_POLY_MAX_VERTS 8

struct poly_t
{
	int count;
	v2 verts[CUTE_POLY_MAX_VERTS];
	v2 norms[CUTE_POLY_MAX_VERTS];
};

struct capsule_t
{
	v2 a;
	v2 b;
	float r;
};

// contains all information necessary to resolve a collision, or in other words
// this is the information needed to separate shapes that are colliding. Doing
// the resolution step is *not* included in cute_c2.
struct manifold_t
{
	int count;
	float depths[2];
	v2 contact_points[2];

	// always points from shape A to shape B (first and second shapes passed into
	// any of the c2***to***Manifold functions)
	v2 n;
};

// boolean collision detection
// these versions are faster than the manifold versions, but only give a YES/NO result
CUTE_API bool CUTE_CALL circle_to_circle(circle_t A, circle_t B);
CUTE_API bool CUTE_CALL circle_to_aabb(circle_t A, aabb_t B);
CUTE_API bool CUTE_CALL circle_to_capsule(circle_t A, capsule_t B);
CUTE_API bool CUTE_CALL aabb_to_aabb(aabb_t A, aabb_t B);
CUTE_API bool CUTE_CALL aabb_to_capsule(aabb_t A, capsule_t B);
CUTE_API bool CUTE_CALL capsule_to_capsule(capsule_t A, capsule_t B);
CUTE_API bool CUTE_CALL circle_to_poly(circle_t A, const poly_t* B, const transform_t* bx);
CUTE_API bool CUTE_CALL aabb_to_poly(aabb_t A, const poly_t* B, const transform_t* bx);
CUTE_API bool CUTE_CALL capsule_to_poly(capsule_t A, const poly_t* B, const transform_t* bx);
CUTE_API bool CUTE_CALL poly_to_poly(const poly_t* A, const transform_t* ax, const poly_t* B, const transform_t* bx);

// ray operations
// output is placed into the raycast_t struct, which represents the hit location
// of the ray. the out param contains no meaningful information if these funcs
// return 0
CUTE_API bool CUTE_CALL ray_to_circle(ray_t A, circle_t B, raycast_t* out);
CUTE_API bool CUTE_CALL ray_to_aabb(ray_t A, aabb_t B, raycast_t* out);
CUTE_API bool CUTE_CALL ray_to_capsule(ray_t A, capsule_t B, raycast_t* out);
CUTE_API bool CUTE_CALL ray_to_poly(ray_t A, const poly_t* B, const transform_t* bx_ptr, raycast_t* out);

// manifold generation
// these functions are (generally) slower than the boolean versions, but will compute one
// or two points that represent the plane of contact. This information is
// is usually needed to resolve and prevent shapes from colliding. If no coll
// ision occured the count member of the manifold struct is set to 0.
CUTE_API void CUTE_CALL circle_to_circle_manifold(circle_t A, circle_t B, manifold_t* m);
CUTE_API void CUTE_CALL circle_to_aabb_manifold(circle_t A, aabb_t B, manifold_t* m);
CUTE_API void CUTE_CALL circle_to_capsule_manifold(circle_t A, capsule_t B, manifold_t* m);
CUTE_API void CUTE_CALL aabb_to_aabb_manifold(aabb_t A, aabb_t B, manifold_t* m);
CUTE_API void CUTE_CALL aabb_to_capsule_manifold(aabb_t A, capsule_t B, manifold_t* m);
CUTE_API void CUTE_CALL capsule_to_capsule_manifold(capsule_t A, capsule_t B, manifold_t* m);
CUTE_API void CUTE_CALL circle_to_poly_manifold(circle_t A, const poly_t* B, const transform_t* bx, manifold_t* m);
CUTE_API void CUTE_CALL aabb_to_poly_manifold(aabb_t A, const poly_t* B, const transform_t* bx, manifold_t* m);
CUTE_API void CUTE_CALL capsule_to_poly_manifold(capsule_t A, const poly_t* B, const transform_t* bx, manifold_t* m);
CUTE_API void CUTE_CALL poly_to_poly_manifold(const poly_t* A, const transform_t* ax, const poly_t* B, const transform_t* bx, manifold_t* m);

enum shape_type_t
{
	SHAPE_TYPE_NONE,
	SHAPE_TYPE_CIRCLE,
	SHAPE_TYPE_AABB,
	SHAPE_TYPE_CAPSULE,
	SHAPE_TYPE_POLY
};

// This struct is only for advanced usage of the c2GJK function. See comments inside of the
// c2GJK function for more details.
struct gjk_cache_t
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
CUTE_API float CUTE_CALL gjk(const void* A, shape_type_t typeA, const transform_t* ax_ptr, const void* B, shape_type_t typeB, const transform_t* bx_ptr, v2* outA, v2* outB, int use_radius, int* iterations, gjk_cache_t* cache);

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
CUTE_API float CUTE_CALL toi(const void* A, shape_type_t typeA, const transform_t* ax_ptr, v2 vA, const void* B, shape_type_t typeB, const transform_t* bx_ptr, v2 vB, int use_radius, int* iterations);

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
CUTE_API void CUTE_CALL inflate(void* shape, shape_type_t type, float skin_factor);

// Computes 2D convex hull. Will not do anything if less than two verts supplied. If
// more than C2_MAX_POLYGON_VERTS are supplied extras are ignored.
CUTE_API int CUTE_CALL hull(v2* verts, int count);
CUTE_API void CUTE_CALL norms(v2* verts, v2* norms, int count);

// runs c2Hull and c2Norms, assumes p->verts and p->count are both set to valid values
CUTE_API void CUTE_CALL make_poly(poly_t* p);

// Generic collision detection routines, useful for games that want to use some poly-
// morphism to write more generic-styled code. Internally calls various above functions.
// For AABBs/Circles/Capsules ax and bx are ignored. For polys ax and bx can define
// model to world transformations (for polys only), or be NULL for identity transforms.
CUTE_API int CUTE_CALL collided(const void* A, const transform_t* ax, shape_type_t typeA, const void* B, const transform_t* bx, shape_type_t typeB);
CUTE_API void CUTE_CALL collide(const void* A, const transform_t* ax, shape_type_t typeA, const void* B, const transform_t* bx, shape_type_t typeB, manifold_t* m);
CUTE_API bool CUTE_CALL cast_ray(ray_t A, const void* B, const transform_t* bx, shape_type_t typeB, raycast_t* out);

}

#endif // CUTE_MATH_H
