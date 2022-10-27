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

#define CUTE_C2_IMPLEMENTATION
#include <cute/cute_c2.h>

#include <cute_math.h>

CUTE_STATIC_ASSERT(CUTE_POLY_MAX_VERTS == C2_MAX_POLYGON_VERTS, "Must be equal.");

CUTE_STATIC_ASSERT(sizeof(cf_v2) == sizeof(c2v), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(cf_sincos_t) == sizeof(c2r), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(cf_transform_t) == sizeof(c2x), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(cf_m2) == sizeof(c2m), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(cf_halfspace_t) == sizeof(c2h), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(cf_ray_t) == sizeof(c2Ray), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(cf_raycast_t) == sizeof(c2Raycast), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(cf_manifold_t) == sizeof(c2Manifold), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(cf_gjk_cache_t) == sizeof(c2GJKCache), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(cf_circle_t) == sizeof(c2Circle), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(cf_aabb_t) == sizeof(c2AABB), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(cf_capsule_t) == sizeof(c2Capsule), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(cf_poly_t) == sizeof(c2Poly), "Must be equal.");

bool cf_circle_to_circle(cf_circle_t A, cf_circle_t B)
{
	return !!c2CircletoCircle(*(c2Circle*)&A, *(c2Circle*)&B);
}

bool cf_circle_to_aabb(cf_circle_t A, cf_aabb_t B)
{
	return !!c2CircletoAABB(*(c2Circle*)&A, *(c2AABB*)&B);
}

bool cf_circle_to_capsule(cf_circle_t A, cf_capsule_t B)
{
	return !!c2CircletoCapsule(*(c2Circle*)&A, *(c2Capsule*)&B);
}

bool cf_aabb_to_aabb(cf_aabb_t A, cf_aabb_t B)
{
	return !!c2AABBtoAABB(*(c2AABB*)&A, *(c2AABB*)&B);
}

bool cf_aabb_to_capsule(cf_aabb_t A, cf_capsule_t B)
{
	return !!c2AABBtoCapsule(*(c2AABB*)&A, *(c2Capsule*)&B);
}

bool cf_capsule_to_capsule(cf_capsule_t A, cf_capsule_t B)
{
	return !!c2CapsuletoCapsule(*(c2Capsule*)&A, *(c2Capsule*)&B);
}

bool cf_circle_to_poly(cf_circle_t A, const cf_poly_t* B, const cf_transform_t* bx)
{
	return !!c2CircletoPoly(*(c2Circle*)&A, (c2Poly*)B, (c2x*)bx);
}

bool cf_aabb_to_poly(cf_aabb_t A, const cf_poly_t* B, const cf_transform_t* bx)
{
	return !!c2AABBtoPoly(*(c2AABB*)&A, (c2Poly*)B, (c2x*)bx);
}

bool cf_capsule_to_poly(cf_capsule_t A, const cf_poly_t* B, const cf_transform_t* bx)
{
	return !!c2CapsuletoPoly(*(c2Capsule*)&A, (c2Poly*)B, (c2x*)bx);
}

bool cf_poly_to_poly(const cf_poly_t* A, const cf_transform_t* ax, const cf_poly_t* B, const cf_transform_t* bx)
{
	return !!c2PolytoPoly((c2Poly*)A, (c2x*)ax, (c2Poly*)B, (c2x*)bx);
}

bool cf_ray_to_circle(cf_ray_t A, cf_circle_t B, cf_raycast_t* out)
{
	return !!c2RaytoCircle(*(c2Ray*)&A, *(c2Circle*)&B, (c2Raycast*)out);
}

bool cf_ray_to_aabb(cf_ray_t A, cf_aabb_t B, cf_raycast_t* out)
{
	return !!c2RaytoAABB(*(c2Ray*)&A, *(c2AABB*)&B, (c2Raycast*)out);
}

bool cf_ray_to_capsule(cf_ray_t A, cf_capsule_t B, cf_raycast_t* out)
{
	return !!c2RaytoCapsule(*(c2Ray*)&A, *(c2Capsule*)&B, (c2Raycast*)out);
}

bool cf_ray_to_poly(cf_ray_t A, const cf_poly_t* B, const cf_transform_t* bx_ptr, cf_raycast_t* out)
{
	return !!c2RaytoPoly(*(c2Ray*)&A, (c2Poly*)B, (c2x*)bx_ptr, (c2Raycast*)out);
}

void cf_circle_to_circle_manifold(cf_circle_t A, cf_circle_t B, cf_manifold_t* m)
{
	c2CircletoCircleManifold(*(c2Circle*)&A, *(c2Circle*)&B, (c2Manifold*)m);
}

void cf_circle_to_aabb_manifold(cf_circle_t A, cf_aabb_t B, cf_manifold_t* m)
{
	c2CircletoAABBManifold(*(c2Circle*)&A, *(c2AABB*)&B, (c2Manifold*)m);
}

void cf_circle_to_capsule_manifold(cf_circle_t A, cf_capsule_t B, cf_manifold_t* m)
{
	c2CircletoCapsuleManifold(*(c2Circle*)&A, *(c2Capsule*)&B, (c2Manifold*)m);
}

void cf_aabb_to_aabb_manifold(cf_aabb_t A, cf_aabb_t B, cf_manifold_t* m)
{
	c2AABBtoAABBManifold(*(c2AABB*)&A, *(c2AABB*)&B, (c2Manifold*)m);
}

void cf_aabb_to_capsule_manifold(cf_aabb_t A, cf_capsule_t B, cf_manifold_t* m)
{
	c2AABBtoCapsuleManifold(*(c2AABB*)&A, *(c2Capsule*)&B, (c2Manifold*)m);
}

void cf_capsule_to_capsule_manifold(cf_capsule_t A, cf_capsule_t B, cf_manifold_t* m)
{
	c2CapsuletoCapsuleManifold(*(c2Capsule*)&A, *(c2Capsule*)&B, (c2Manifold*)m);
}

void cf_circle_to_poly_manifold(cf_circle_t A, const cf_poly_t* B, const cf_transform_t* bx, cf_manifold_t* m)
{
	c2CircletoPolyManifold(*(c2Circle*)&A, (c2Poly*)B, (c2x*)bx, (c2Manifold*)m);
}

void cf_aabb_to_poly_manifold(cf_aabb_t A, const cf_poly_t* B, const cf_transform_t* bx, cf_manifold_t* m)
{
	c2AABBtoPolyManifold(*(c2AABB*)&A, (c2Poly*)B, (c2x*)bx, (c2Manifold*)m);
}

void cf_capsule_to_poly_manifold(cf_capsule_t A, const cf_poly_t* B, const cf_transform_t* bx, cf_manifold_t* m)
{
	c2CapsuletoPolyManifold(*(c2Capsule*)&A, (c2Poly*)B, (c2x*)bx, (c2Manifold*)m);
}

void cf_poly_to_poly_manifold(const cf_poly_t* A, const cf_transform_t* ax, const cf_poly_t* B, const cf_transform_t* bx, cf_manifold_t* m)
{
	c2PolytoPolyManifold((c2Poly*)A, (c2x*)ax, (c2Poly*)B, (c2x*)bx, (c2Manifold*)m);
}

float cf_gjk(const void* A, cf_shape_type_t typeA, const cf_transform_t* ax_ptr, const void* B, cf_shape_type_t typeB, const cf_transform_t* bx_ptr, cf_v2* outA, cf_v2* outB, int use_radius, int* iterations, cf_gjk_cache_t* cache)
{
	return c2GJK(A, (C2_TYPE)typeA, (c2x*)ax_ptr, B, (C2_TYPE)typeB, (c2x*)bx_ptr, (c2v*)outA, (c2v*)outB, use_radius, iterations, (c2GJKCache*)cache);
}

cf_toi_result_t cf_toi(const void* A, cf_shape_type_t typeA, const cf_transform_t* ax_ptr, cf_v2 vA, const void* B, cf_shape_type_t typeB, const cf_transform_t* bx_ptr, cf_v2 vB, int use_radius)
{
	cf_toi_result_t result;
	c2TOIResult c2result = c2TOI(A, (C2_TYPE)typeA, (c2x*)ax_ptr, *(c2v*)&vA, B, (C2_TYPE)typeB, (c2x*)bx_ptr, *(c2v*)&vB, use_radius);
	result = *(cf_toi_result_t*)&c2result;
	return result;
}

void cf_inflate(void* shape, cf_shape_type_t type, float skin_factor)
{
	c2Inflate(shape, (C2_TYPE)type, skin_factor);
}

int cf_hull(cf_v2* verts, int count)
{
	return c2Hull((c2v*)verts, count);
}

void cf_norms(cf_v2* verts, cf_v2* norms, int count)
{
	c2Norms((c2v*)verts, (c2v*)norms, count);
}

void cf_make_poly(cf_poly_t* p)
{
	c2MakePoly((c2Poly*)p);
}

cf_v2 cf_centroid(const cf_v2* cf_verts, int count)
{
	using namespace cute;
	const v2* verts = (const v2*)cf_verts;
	if (count == 0) return cf_V2(0, 0);
	else if (count == 1) return verts[0];
	else if (count == 2) return (verts[0] + verts[1]) * 0.5f;
	cf_v2 c = cf_V2(0, 0);
	float area_sum = 0;
	cf_v2 p0 = verts[0];
	for (int i = 0; i < count; ++i) {
		cf_v2 p1 = verts[0] - p0;
		cf_v2 p2 = verts[i] - p0;
		cf_v2 p3 = (i + 1 == count ? verts[0] : verts[i + 1]) - p0;
		cf_v2 e1 = p2 - p1;
		cf_v2 e2 = p3 - p1;
		float area = 0.5f * cf_cross(e1, e2);
		area_sum += area;
		c = c + (p1 + p2 + p3) * area * (1.0f/3.0f);
	}
	return c * (1.0f / area_sum) + p0;
}

int cf_collided(const void* A, const cf_transform_t* ax, cf_shape_type_t typeA, const void* B, const cf_transform_t* bx, cf_shape_type_t typeB)
{
	return c2Collided(A, (c2x*)ax, (C2_TYPE)typeA, B, (c2x*)bx, (C2_TYPE)typeB);
}

void cf_collide(const void* A, const cf_transform_t* ax, cf_shape_type_t typeA, const void* B, const cf_transform_t* bx, cf_shape_type_t typeB, cf_manifold_t* m)
{
	c2Collide(A, (c2x*)ax, (C2_TYPE)typeA, B, (c2x*)bx, (C2_TYPE)typeB, (c2Manifold*)m);
}

bool cf_cast_ray(cf_ray_t A, const void* B, const cf_transform_t* bx, cf_shape_type_t typeB, cf_raycast_t* out)
{
	return c2CastRay(*(c2Ray*)&A, B, (c2x*)bx, (C2_TYPE)typeB, (c2Raycast*)out);
}
