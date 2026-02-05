/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_defines.h>
#include <cute_c_runtime.h>

#define CUTE_C2_IMPLEMENTATION
#include <cute/cute_c2.h>

#include <cute_math.h>

CF_STATIC_ASSERT(CF_POLY_MAX_VERTS == C2_MAX_POLYGON_VERTS, "Must be equal.");

CF_STATIC_ASSERT(sizeof(CF_V2) == sizeof(c2v), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_M2x2) == sizeof(c2m), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Halfspace) == sizeof(c2h), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Ray) == sizeof(c2Ray), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Manifold) == sizeof(c2Manifold), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_GjkCache) == sizeof(c2GJKCache), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Circle) == sizeof(c2Circle), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Aabb) == sizeof(c2AABB), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Capsule) == sizeof(c2Capsule), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Poly) == sizeof(c2Poly), "Must be equal.");

using namespace Cute;

CF_V2 cf_center_of_mass(CF_Poly poly)
{
	v2 p0 = poly.verts[0];
	float area_sum = 0;
	const float inv3 = 1.0f / 3.0f;
	v2 center_of_mass = V2(0,0);

	// Triangle fan of p0, p1, and p2.
	for (int i = 0; i < poly.count; ++i) {
		v2 p1 = poly.verts[i];
		v2 p2 = poly.verts[i + 1 == poly.count ? 0 : i + 1];

		// Sum the area of all triangles.
		float area_of_triangle = det2(p1 - p0, p2 - p0) * 0.5f;
		area_sum += area_of_triangle;

		// Center of mass is the area-weighted centroid.
		// Centroid is the average of all vertices.
		center_of_mass += (p0 + p1 + p2) * (area_of_triangle * inv3);
	}

	center_of_mass *= 1.0f / area_sum;
	return center_of_mass;
}

float cf_calc_area(CF_Poly poly)
{
	v2 p0 = poly.verts[0];
	float area = 0;

	// Triangle fan of p0, p1, and p2.
	for (int i = 0; i < poly.count; ++i) {
		v2 p1 = poly.verts[i];
		v2 p2 = poly.verts[i + 1 == poly.count ? 0 : i + 1];
		area += det2(p1 - p0, p2 - p0) * 0.5f;
	}

	return area;
}

CF_INLINE bool in_front(float distance, float epsilon) { return distance > epsilon; }
CF_INLINE bool behind(float distance, float epsilon) { return distance < -epsilon; }
CF_INLINE bool on(float distance, float epsilon) { return !in_front(distance, epsilon) && !behind(distance, epsilon); }

CF_SliceOutput cf_slice(CF_Halfspace slice_plane, CF_Poly slice_me, const float k_epsilon)
{
	CF_ASSERT(slice_me.count);
	int front_count = 0;
	int back_count = 0;
	v2 front[CF_POLY_MAX_VERTS+1];
	v2 back[CF_POLY_MAX_VERTS+1];
	v2 a = slice_me.verts[slice_me.count - 1];
	float da = distance(slice_plane, a);

	for (int i = 0; i < slice_me.count; ++i) {
		v2 b = slice_me.verts[i];
		float db = distance(slice_plane, b);

		if (in_front(db, k_epsilon)) {
			if(behind(da, k_epsilon)) {
				v2 i = intersect(b, a, db, da);
				front[front_count++] = i;
				back[back_count++] = i;
			}
			front[front_count++] = b;
		} else if (behind(db, k_epsilon)) {
			if (in_front(da, k_epsilon)) {
				v2 i = intersect(a, b, da, db);
				front[front_count++] = i;
				back[back_count++] = i;
			} else if (on(da, k_epsilon)) {
				back[back_count++] = a;
			}
			back[back_count++] = b;
		} else {
			front[front_count++] = b;
			if (on(da, k_epsilon)) {
				back[back_count++] = b;
			}
		}

		a = b;
		da = db;
	}

	// CF_POLY_MAX_VERTS+1 verts potentially generated in a single polygon, truncate to CF_POLY_MAX_VERTS.
	CF_SliceOutput out = { };
	out.front.count = min(CF_POLY_MAX_VERTS, front_count);
	out.back.count = min(CF_POLY_MAX_VERTS, back_count);
	CF_MEMCPY(out.front.verts, front, sizeof(v2) * out.front.count);
	CF_MEMCPY(out.back.verts, back, sizeof(v2) * out.back.count);
	norms(out.front.verts, out.front.norms, out.front.count);
	norms(out.back.verts, out.back.norms, out.back.count);
	return out;
}

void cf_inflate(void* shape, CF_ShapeType type, float skin_factor)
{
	c2Inflate(shape, (C2_TYPE)type, skin_factor);
}

int cf_hull(CF_V2* verts, int count)
{
	return c2Hull((c2v*)verts, count);
}

void cf_norms(CF_V2* verts, CF_V2* norms, int count)
{
	c2Norms((c2v*)verts, (c2v*)norms, count);
}

void cf_make_poly(CF_Poly* p)
{
	c2MakePoly((c2Poly*)p);
}

CF_V2 cf_centroid(const CF_V2* cf_verts, int count)
{
	const v2* verts = (const v2*)cf_verts;
	if (count == 0) return cf_v2(0, 0);
	else if (count == 1) return verts[0];
	else if (count == 2) return (verts[0] + verts[1]) * 0.5f;
	CF_V2 c = cf_v2(0, 0);
	float area_sum = 0;
	CF_V2 p0 = verts[0];
	for (int i = 0; i < count; ++i) {
		CF_V2 p1 = verts[0] - p0;
		CF_V2 p2 = verts[i] - p0;
		CF_V2 p3 = (i + 1 == count ? verts[0] : verts[i + 1]) - p0;
		CF_V2 e1 = p2 - p1;
		CF_V2 e2 = p3 - p1;
		float area = 0.5f * cf_cross(e1, e2);
		area_sum += area;
		c = c + (p1 + p2 + p3) * area * (1.0f/3.0f);
	}
	return c * (1.0f / area_sum) + p0;
}

bool cf_circle_to_circle(CF_Circle A, CF_Circle B)
{
	return !!c2CircletoCircle(*(c2Circle*)&A, *(c2Circle*)&B);
}

bool cf_circle_to_aabb(CF_Circle A, CF_Aabb B)
{
	return !!c2CircletoAABB(*(c2Circle*)&A, *(c2AABB*)&B);
}

bool cf_circle_to_capsule(CF_Circle A, CF_Capsule B)
{
	return !!c2CircletoCapsule(*(c2Circle*)&A, *(c2Capsule*)&B);
}

bool cf_aabb_to_aabb(CF_Aabb A, CF_Aabb B)
{
	return !!c2AABBtoAABB(*(c2AABB*)&A, *(c2AABB*)&B);
}

bool cf_aabb_to_capsule(CF_Aabb A, CF_Capsule B)
{
	return !!c2AABBtoCapsule(*(c2AABB*)&A, *(c2Capsule*)&B);
}

bool cf_capsule_to_capsule(CF_Capsule A, CF_Capsule B)
{
	return !!c2CapsuletoCapsule(*(c2Capsule*)&A, *(c2Capsule*)&B);
}

bool cf_circle_to_poly(CF_Circle A, const CF_Poly* B)
{
	return !!c2CircletoPoly(*(c2Circle*)&A, (c2Poly*)B);
}

bool cf_aabb_to_poly(CF_Aabb A, const CF_Poly* B)
{
	return !!c2AABBtoPoly(*(c2AABB*)&A, (c2Poly*)B);
}

bool cf_capsule_to_poly(CF_Capsule A, const CF_Poly* B)
{
	return !!c2CapsuletoPoly(*(c2Capsule*)&A, (c2Poly*)B);
}

bool cf_poly_to_poly(const CF_Poly* A, const CF_Poly* B)
{
	return !!c2PolytoPoly((c2Poly*)A, (c2Poly*)B);
}

CF_Raycast cf_ray_to_circle(CF_Ray A, CF_Circle B)
{
	CF_Raycast result;
	c2Raycast cast;
	result.hit = !!c2RaytoCircle(*(c2Ray*)&A, *(c2Circle*)&B, (c2Raycast*)&cast);
	result.n = *(v2*)&cast.n;
	result.t = cast.t;
	return result;
}

CF_Raycast cf_ray_to_aabb(CF_Ray A, CF_Aabb B)
{
	CF_Raycast result;
	c2Raycast cast;
	result.hit = !!c2RaytoAABB(*(c2Ray*)&A, *(c2AABB*)&B, (c2Raycast*)&cast);
	result.n = *(v2*)&cast.n;
	result.t = cast.t;
	return result;
}

CF_Raycast cf_ray_to_capsule(CF_Ray A, CF_Capsule B)
{
	CF_Raycast result;
	c2Raycast cast;
	result.hit = !!c2RaytoCapsule(*(c2Ray*)&A, *(c2Capsule*)&B, (c2Raycast*)&cast);
	result.n = *(v2*)&cast.n;
	result.t = cast.t;
	return result;
}

CF_Raycast cf_ray_to_poly(CF_Ray A, const CF_Poly* B)
{
	CF_Raycast result;
	c2Raycast cast;
	result.hit = !!c2RaytoPoly(*(c2Ray*)&A, (c2Poly*)B, (c2Raycast*)&cast);
	result.n = *(v2*)&cast.n;
	result.t = cast.t;
	return result;
}
CF_Manifold cf_circle_to_circle_manifold(CF_Circle A, CF_Circle B)
{
	c2Manifold m;
	c2CircletoCircleManifold(*(c2Circle*)&A, *(c2Circle*)&B, &m);
	return *(CF_Manifold*)&m;
}

CF_Manifold cf_circle_to_aabb_manifold(CF_Circle A, CF_Aabb B)
{
	c2Manifold m;
	c2CircletoAABBManifold(*(c2Circle*)&A, *(c2AABB*)&B, &m);
	return *(CF_Manifold*)&m;
}

CF_Manifold cf_circle_to_capsule_manifold(CF_Circle A, CF_Capsule B)
{
	c2Manifold m;
	c2CircletoCapsuleManifold(*(c2Circle*)&A, *(c2Capsule*)&B, &m);
	return *(CF_Manifold*)&m;
}

CF_Manifold cf_aabb_to_aabb_manifold(CF_Aabb A, CF_Aabb B)
{
	c2Manifold m;
	c2AABBtoAABBManifold(*(c2AABB*)&A, *(c2AABB*)&B, &m);
	return *(CF_Manifold*)&m;
}

CF_Manifold cf_aabb_to_capsule_manifold(CF_Aabb A, CF_Capsule B)
{
	c2Manifold m;
	c2AABBtoCapsuleManifold(*(c2AABB*)&A, *(c2Capsule*)&B, &m);
	return *(CF_Manifold*)&m;
}

CF_Manifold cf_capsule_to_capsule_manifold(CF_Capsule A, CF_Capsule B)
{
	c2Manifold m;
	c2CapsuletoCapsuleManifold(*(c2Capsule*)&A, *(c2Capsule*)&B, &m);
	return *(CF_Manifold*)&m;
}

CF_Manifold cf_circle_to_poly_manifold(CF_Circle A, const CF_Poly* B)
{
	c2Manifold m;
	c2CircletoPolyManifold(*(c2Circle*)&A, (c2Poly*)B, &m);
	return *(CF_Manifold*)&m;
}

CF_Manifold cf_aabb_to_poly_manifold(CF_Aabb A, const CF_Poly* B)
{
	c2Manifold m;
	c2AABBtoPolyManifold(*(c2AABB*)&A, (c2Poly*)B, &m);
	return *(CF_Manifold*)&m;
}

CF_Manifold cf_capsule_to_poly_manifold(CF_Capsule A, const CF_Poly* B)
{
	c2Manifold m;
	c2CapsuletoPolyManifold(*(c2Capsule*)&A, (c2Poly*)B, &m);
	return *(CF_Manifold*)&m;
}

CF_Manifold cf_poly_to_poly_manifold(const CF_Poly* A, const CF_Poly* B)
{
	c2Manifold m;
	c2PolytoPolyManifold((c2Poly*)A, (c2Poly*)B, &m);
	return *(CF_Manifold*)&m;
}

float cf_gjk(const void* A, CF_ShapeType typeA, const void* B, CF_ShapeType typeB, CF_V2* outA, CF_V2* outB, bool use_radius, int* iterations, CF_GjkCache* cache)
{
	return c2GJK(A, (C2_TYPE)typeA, B, (C2_TYPE)typeB, (c2v*)outA, (c2v*)outB, (int)use_radius, iterations, (c2GJKCache*)cache);
}

CF_ToiResult cf_toi(const void* A, CF_ShapeType typeA, CF_V2 vA, const void* B, CF_ShapeType typeB, CF_V2 vB, int use_radius)
{
	CF_ToiResult result;
	c2TOIResult c2result = c2TOI(A, (C2_TYPE)typeA, *(c2v*)&vA, B, (C2_TYPE)typeB, *(c2v*)&vB, use_radius);
	result = *(CF_ToiResult*)&c2result;
	return result;
}

int cf_collided(const void* A, CF_ShapeType typeA, const void* B, CF_ShapeType typeB)
{
	return c2Collided(A, (C2_TYPE)typeA, B, (C2_TYPE)typeB);
}

void cf_collide(const void* A, CF_ShapeType typeA, const void* B, CF_ShapeType typeB, CF_Manifold* m)
{
	c2Collide(A, (C2_TYPE)typeA, B, (C2_TYPE)typeB, (c2Manifold*)m);
}

bool cf_cast_ray(CF_Ray A, const void* B, CF_ShapeType typeB, CF_Raycast* out)
{
	c2Raycast cast;
	out->hit = !!c2CastRay(*(c2Ray*)&A, B, (C2_TYPE)typeB, (c2Raycast*)&cast);
	out->n = *(v2*)&cast.n;
	out->t = cast.t;
	return out->hit;
}
