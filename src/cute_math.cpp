/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#include <cute_defines.h>

#define CUTE_C2_IMPLEMENTATION
#include <cute/cute_c2.h>

#include <cute_math.h>

CF_STATIC_ASSERT(CF_POLY_MAX_VERTS == C2_MAX_POLYGON_VERTS, "Must be equal.");

CF_STATIC_ASSERT(sizeof(CF_V2) == sizeof(c2v), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_SinCos) == sizeof(c2r), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Transform) == sizeof(c2x), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_M2x2) == sizeof(c2m), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Halfspace) == sizeof(c2h), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Ray) == sizeof(c2Ray), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Raycast) == sizeof(c2Raycast), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Manifold) == sizeof(c2Manifold), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_GjkCache) == sizeof(c2GJKCache), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Circle) == sizeof(c2Circle), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Aabb) == sizeof(c2AABB), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Capsule) == sizeof(c2Capsule), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Poly) == sizeof(c2Poly), "Must be equal.");

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

bool cf_circle_to_poly(CF_Circle A, const CF_Poly* B, const CF_Transform* bx)
{
	return !!c2CircletoPoly(*(c2Circle*)&A, (c2Poly*)B, (c2x*)bx);
}

bool cf_aabb_to_poly(CF_Aabb A, const CF_Poly* B, const CF_Transform* bx)
{
	return !!c2AABBtoPoly(*(c2AABB*)&A, (c2Poly*)B, (c2x*)bx);
}

bool cf_capsule_to_poly(CF_Capsule A, const CF_Poly* B, const CF_Transform* bx)
{
	return !!c2CapsuletoPoly(*(c2Capsule*)&A, (c2Poly*)B, (c2x*)bx);
}

bool cf_poly_to_poly(const CF_Poly* A, const CF_Transform* ax, const CF_Poly* B, const CF_Transform* bx)
{
	return !!c2PolytoPoly((c2Poly*)A, (c2x*)ax, (c2Poly*)B, (c2x*)bx);
}

bool cf_ray_to_circle(CF_Ray A, CF_Circle B, CF_Raycast* out)
{
	return !!c2RaytoCircle(*(c2Ray*)&A, *(c2Circle*)&B, (c2Raycast*)out);
}

bool cf_ray_to_aabb(CF_Ray A, CF_Aabb B, CF_Raycast* out)
{
	return !!c2RaytoAABB(*(c2Ray*)&A, *(c2AABB*)&B, (c2Raycast*)out);
}

bool cf_ray_to_capsule(CF_Ray A, CF_Capsule B, CF_Raycast* out)
{
	return !!c2RaytoCapsule(*(c2Ray*)&A, *(c2Capsule*)&B, (c2Raycast*)out);
}

bool cf_ray_to_poly(CF_Ray A, const CF_Poly* B, const CF_Transform* bx_ptr, CF_Raycast* out)
{
	return !!c2RaytoPoly(*(c2Ray*)&A, (c2Poly*)B, (c2x*)bx_ptr, (c2Raycast*)out);
}

void cf_circle_to_circle_manifold(CF_Circle A, CF_Circle B, CF_Manifold* m)
{
	c2CircletoCircleManifold(*(c2Circle*)&A, *(c2Circle*)&B, (c2Manifold*)m);
}

void cf_circle_to_aabb_manifold(CF_Circle A, CF_Aabb B, CF_Manifold* m)
{
	c2CircletoAABBManifold(*(c2Circle*)&A, *(c2AABB*)&B, (c2Manifold*)m);
}

void cf_circle_to_capsule_manifold(CF_Circle A, CF_Capsule B, CF_Manifold* m)
{
	c2CircletoCapsuleManifold(*(c2Circle*)&A, *(c2Capsule*)&B, (c2Manifold*)m);
}

void cf_aabb_to_aabb_manifold(CF_Aabb A, CF_Aabb B, CF_Manifold* m)
{
	c2AABBtoAABBManifold(*(c2AABB*)&A, *(c2AABB*)&B, (c2Manifold*)m);
}

void cf_aabb_to_capsule_manifold(CF_Aabb A, CF_Capsule B, CF_Manifold* m)
{
	c2AABBtoCapsuleManifold(*(c2AABB*)&A, *(c2Capsule*)&B, (c2Manifold*)m);
}

void cf_capsule_to_capsule_manifold(CF_Capsule A, CF_Capsule B, CF_Manifold* m)
{
	c2CapsuletoCapsuleManifold(*(c2Capsule*)&A, *(c2Capsule*)&B, (c2Manifold*)m);
}

void cf_circle_to_poly_manifold(CF_Circle A, const CF_Poly* B, const CF_Transform* bx, CF_Manifold* m)
{
	c2CircletoPolyManifold(*(c2Circle*)&A, (c2Poly*)B, (c2x*)bx, (c2Manifold*)m);
}

void cf_aabb_to_poly_manifold(CF_Aabb A, const CF_Poly* B, const CF_Transform* bx, CF_Manifold* m)
{
	c2AABBtoPolyManifold(*(c2AABB*)&A, (c2Poly*)B, (c2x*)bx, (c2Manifold*)m);
}

void cf_capsule_to_poly_manifold(CF_Capsule A, const CF_Poly* B, const CF_Transform* bx, CF_Manifold* m)
{
	c2CapsuletoPolyManifold(*(c2Capsule*)&A, (c2Poly*)B, (c2x*)bx, (c2Manifold*)m);
}

void cf_poly_to_poly_manifold(const CF_Poly* A, const CF_Transform* ax, const CF_Poly* B, const CF_Transform* bx, CF_Manifold* m)
{
	c2PolytoPolyManifold((c2Poly*)A, (c2x*)ax, (c2Poly*)B, (c2x*)bx, (c2Manifold*)m);
}

float cf_gjk(const void* A, CF_ShapeType typeA, const CF_Transform* ax_ptr, const void* B, CF_ShapeType typeB, const CF_Transform* bx_ptr, CF_V2* outA, CF_V2* outB, bool use_radius, int* iterations, CF_GjkCache* cache)
{
	return c2GJK(A, (C2_TYPE)typeA, (c2x*)ax_ptr, B, (C2_TYPE)typeB, (c2x*)bx_ptr, (c2v*)outA, (c2v*)outB, (int)use_radius, iterations, (c2GJKCache*)cache);
}

CF_ToiResult cf_toi(const void* A, CF_ShapeType typeA, const CF_Transform* ax_ptr, CF_V2 vA, const void* B, CF_ShapeType typeB, const CF_Transform* bx_ptr, CF_V2 vB, int use_radius)
{
	CF_ToiResult result;
	c2TOIResult c2result = c2TOI(A, (C2_TYPE)typeA, (c2x*)ax_ptr, *(c2v*)&vA, B, (C2_TYPE)typeB, (c2x*)bx_ptr, *(c2v*)&vB, use_radius);
	result = *(CF_ToiResult*)&c2result;
	return result;
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
	using namespace Cute;
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

int cf_collided(const void* A, const CF_Transform* ax, CF_ShapeType typeA, const void* B, const CF_Transform* bx, CF_ShapeType typeB)
{
	return c2Collided(A, (c2x*)ax, (C2_TYPE)typeA, B, (c2x*)bx, (C2_TYPE)typeB);
}

void cf_collide(const void* A, const CF_Transform* ax, CF_ShapeType typeA, const void* B, const CF_Transform* bx, CF_ShapeType typeB, CF_Manifold* m)
{
	c2Collide(A, (c2x*)ax, (C2_TYPE)typeA, B, (c2x*)bx, (C2_TYPE)typeB, (c2Manifold*)m);
}

bool cf_cast_ray(CF_Ray A, const void* B, const CF_Transform* bx, CF_ShapeType typeB, CF_Raycast* out)
{
	return c2CastRay(*(c2Ray*)&A, B, (c2x*)bx, (C2_TYPE)typeB, (c2Raycast*)out);
}
