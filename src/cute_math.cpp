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

namespace cute
{

CUTE_STATIC_ASSERT(sizeof(v2) == sizeof(c2v), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(sincos_t) == sizeof(c2r), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(transform_t) == sizeof(c2x), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(m2) == sizeof(c2m), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(halfspace_t) == sizeof(c2h), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(ray_t) == sizeof(c2Ray), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(raycast_t) == sizeof(c2Raycast), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(manifold_t) == sizeof(c2Manifold), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(gjk_cache_t) == sizeof(c2GJKCache), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(circle_t) == sizeof(c2Circle), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(aabb_t) == sizeof(c2AABB), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(capsule_t) == sizeof(c2Capsule), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(poly_t) == sizeof(c2Poly), "Must be equal.");

bool circle_to_circle(circle_t A, circle_t B)
{
	return !!c2CircletoCircle(*(c2Circle*)&A, *(c2Circle*)&B);
}

bool circle_to_aabb(circle_t A, aabb_t B)
{
	return !!c2CircletoAABB(*(c2Circle*)&A, *(c2AABB*)&B);
}

bool circle_to_capsule(circle_t A, capsule_t B)
{
	return !!c2CircletoCapsule(*(c2Circle*)&A, *(c2Capsule*)&B);
}

bool aabb_to_aabb(aabb_t A, aabb_t B)
{
	return !!c2AABBtoAABB(*(c2AABB*)&A, *(c2AABB*)&B);
}

bool aabb_to_capsule(aabb_t A, capsule_t B)
{
	return !!c2AABBtoCapsule(*(c2AABB*)&A, *(c2Capsule*)&B);
}

bool capsule_to_capsule(capsule_t A, capsule_t B)
{
	return !!c2CapsuletoCapsule(*(c2Capsule*)&A, *(c2Capsule*)&B);
}

bool circle_to_poly(circle_t A, const poly_t* B, const transform_t* bx)
{
	return !!c2CircletoPoly(*(c2Circle*)&A, (c2Poly*)B, (c2x*)bx);
}

bool aabb_to_poly(aabb_t A, const poly_t* B, const transform_t* bx)
{
	return !!c2AABBtoPoly(*(c2AABB*)&A, (c2Poly*)B, (c2x*)bx);
}

bool capsule_to_poly(capsule_t A, const poly_t* B, const transform_t* bx)
{
	return !!c2CapsuletoPoly(*(c2Capsule*)&A, (c2Poly*)B, (c2x*)bx);
}

bool poly_to_poly(const poly_t* A, const transform_t* ax, const poly_t* B, const transform_t* bx)
{
	return !!c2PolytoPoly((c2Poly*)A, (c2x*)ax, (c2Poly*)B, (c2x*)bx);
}

bool ray_to_circle(ray_t A, circle_t B, raycast_t* out)
{
	return !!c2RaytoCircle(*(c2Ray*)&A, *(c2Circle*)&B, (c2Raycast*)out);
}

bool ray_to_aabb(ray_t A, aabb_t B, raycast_t* out)
{
	return !!c2RaytoAABB(*(c2Ray*)&A, *(c2AABB*)&B, (c2Raycast*)out);
}

bool ray_to_capsule(ray_t A, capsule_t B, raycast_t* out)
{
	return !!c2RaytoCapsule(*(c2Ray*)&A, *(c2Capsule*)&B, (c2Raycast*)out);
}

bool ray_to_poly(ray_t A, const poly_t* B, const transform_t* bx_ptr, raycast_t* out)
{
	return !!c2RaytoPoly(*(c2Ray*)&A, (c2Poly*)B, (c2x*)bx_ptr, (c2Raycast*)out);
}

void circle_to_circle_manifold(circle_t A, circle_t B, manifold_t* m)
{
	c2CircletoCircleManifold(*(c2Circle*)&A, *(c2Circle*)&B, (c2Manifold*)m);
}

void circle_to_aabb_manifold(circle_t A, aabb_t B, manifold_t* m)
{
	c2CircletoAABBManifold(*(c2Circle*)&A, *(c2AABB*)&B, (c2Manifold*)m);
}

void circle_to_capsule_manifold(circle_t A, capsule_t B, manifold_t* m)
{
	c2CircletoCapsuleManifold(*(c2Circle*)&A, *(c2Capsule*)&B, (c2Manifold*)m);
}

void aabb_to_aabb_manifold(aabb_t A, aabb_t B, manifold_t* m)
{
	c2AABBtoAABBManifold(*(c2AABB*)&A, *(c2AABB*)&B, (c2Manifold*)m);
}

void aabb_to_capsule_manifold(aabb_t A, capsule_t B, manifold_t* m)
{
	c2AABBtoCapsuleManifold(*(c2AABB*)&A, *(c2Capsule*)&B, (c2Manifold*)m);
}

void capsule_to_capsule_manifold(capsule_t A, capsule_t B, manifold_t* m)
{
	c2CapsuletoCapsuleManifold(*(c2Capsule*)&A, *(c2Capsule*)&B, (c2Manifold*)m);
}

void circle_to_poly_manifold(circle_t A, const poly_t* B, const transform_t* bx, manifold_t* m)
{
	c2CircletoPolyManifold(*(c2Circle*)&A, (c2Poly*)B, (c2x*)bx, (c2Manifold*)m);
}

void aabb_to_poly_manifold(aabb_t A, const poly_t* B, const transform_t* bx, manifold_t* m)
{
	c2AABBtoPolyManifold(*(c2AABB*)&A, (c2Poly*)B, (c2x*)bx, (c2Manifold*)m);
}

void capsule_to_poly_manifold(capsule_t A, const poly_t* B, const transform_t* bx, manifold_t* m)
{
	c2CapsuletoPolyManifold(*(c2Capsule*)&A, (c2Poly*)B, (c2x*)bx, (c2Manifold*)m);
}

void poly_to_poly_manifold(const poly_t* A, const transform_t* ax, const poly_t* B, const transform_t* bx, manifold_t* m)
{
	c2PolytoPolyManifold((c2Poly*)A, (c2x*)ax, (c2Poly*)B, (c2x*)bx, (c2Manifold*)m);
}

float gjk(const void* A, cute_shape_type_t typeA, const transform_t* ax_ptr, const void* B, cute_shape_type_t typeB, const transform_t* bx_ptr, v2* outA, v2* outB, int use_radius, int* iterations, gjk_cache_t* cache)
{
	return c2GJK(A, (C2_TYPE)typeA, (c2x*)ax_ptr, B, (C2_TYPE)typeB, (c2x*)bx_ptr, (c2v*)outA, (c2v*)outB, use_radius, iterations, (c2GJKCache*)cache);
}

float toi(const void* A, cute_shape_type_t typeA, const transform_t* ax_ptr, v2 vA, const void* B, cute_shape_type_t typeB, const transform_t* bx_ptr, v2 vB, int use_radius, int* iterations)
{
	return c2TOI(A, (C2_TYPE)typeA, (c2x*)ax_ptr, *(c2v*)&vA, B, (C2_TYPE)typeB, (c2x*)bx_ptr, *(c2v*)&vB, use_radius, iterations);
}

void inflate(void* shape, cute_shape_type_t type, float skin_factor)
{
	c2Inflate(shape, (C2_TYPE)type, skin_factor);
}

int hull(v2* verts, int count)
{
	return c2Hull((c2v*)verts, count);
}

void norms(v2* verts, v2* norms, int count)
{
	c2Norms((c2v*)verts, (c2v*)norms, count);
}

void make_poly(poly_t* p)
{
	c2MakePoly((c2Poly*)p);
}

v2 centroid(v2* verts, int count)
{
	if (count == 0) return v2(0, 0);
	else if (count == 1) return verts[0];
	else if (count == 2) return (verts[0] + verts[1]) * 0.5f;
	v2 c = v2(0, 0);
	float area = 0;
	v2 p0 = verts[0];
	for (int i = 0; i < count; ++i) {
		v2 p1 = verts[0] - p0;
		v2 p2 = verts[i] - p0;
		v2 p3 = (i + 1 == count ? verts[0] : verts[i + 1]) - p0;
		v2 e1 = p2 - p1;
		v2 e2 = p3 - p1;
		area += 0.5f * cross(e1, e2);
		c += (p1 + p2 + p3) * area * (1.0f/3.0f);
	}
	return c = c * (1.0f / area) + p0;
}

int collided(const void* A, const transform_t* ax, cute_shape_type_t typeA, const void* B, const transform_t* bx, cute_shape_type_t typeB)
{
	return c2Collided(A, (c2x*)ax, (C2_TYPE)typeA, B, (c2x*)bx, (C2_TYPE)typeB);
}

void collide(const void* A, const transform_t* ax, cute_shape_type_t typeA, const void* B, const transform_t* bx, cute_shape_type_t typeB, manifold_t* m)
{
	c2Collide(A, (c2x*)ax, (C2_TYPE)typeA, B, (c2x*)bx, (C2_TYPE)typeB, (c2Manifold*)m);
}

bool cast_ray(ray_t A, const void* B, const transform_t* bx, cute_shape_type_t typeB, raycast_t* out)
{
	return c2CastRay(*(c2Ray*)&A, B, (c2x*)bx, (C2_TYPE)typeB, (c2Raycast*)out);
}

}
