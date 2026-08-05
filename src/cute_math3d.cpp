/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_defines.h>
#include <cute_c_runtime.h>

#include <cute_math3d.h>

// The 3d stateless collision queries route to Box3D's freestanding geometry layer
// (box3d/collision.h) -- no b3World or simulation state anywhere in
// this file, mirroring how cute_math.cpp rides Box2D's geometry layer in 2d. CF shapes are
// world-space, so every relative transform below is the identity and Box3D's frame-A local
// results are already world-space.
#include <box3d/collision.h>
#include <box3d/math_functions.h>

CF_STATIC_ASSERT(sizeof(CF_V3) == sizeof(b3Vec3), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Sphere) == sizeof(b3Sphere), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Capsule3) == sizeof(b3Capsule), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Aabb3) == sizeof(b3AABB), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_GjkCache3) == sizeof(b3SimplexCache), "Must be equal.");

using namespace Cute;

CF_INLINE b3Vec3 s_b3(CF_V3 v) { b3Vec3 out; out.x = v.x; out.y = v.y; out.z = v.z; return out; }
CF_INLINE CF_V3 s_cf(b3Vec3 v) { return cf_v3(v.x, v.y, v.z); }

// Enough for any single-pair manifold; Box3D reduces to at most 4 points.
#define CF_B3_POINT_CAPACITY 8

// Box3D's manifolds are speculative: nearby-but-separated points can be reported. A
// CF_Manifold3 only ever describes actual touching, so separated points drop and
// separation negates into depth. `flip` restores the normal to point from A to B after a
// canonicalizing pair swap.
static CF_Manifold3 s_manifold3(const b3LocalManifold* m, bool flip)
{
	CF_Manifold3 out = { };
	v3 n = s_cf(m->normal);
	out.n = flip ? -n : n;
	int count = m->pointCount > CF_MANIFOLD3_MAX_POINTS ? CF_MANIFOLD3_MAX_POINTS : m->pointCount;
	for (int i = 0; i < count; ++i) {
		if (m->points[i].separation <= 0) {
			out.contact_points[out.count] = s_cf(m->points[i].point);
			out.depths[out.count] = -m->points[i].separation;
			++out.count;
		}
	}
	return out;
}

// A CF shape lifted into Box3D terms. AABBs ride as box hulls since Box3D's box hull is a
// self-contained value type (offset-based, no heap). Kinds rank sphere < capsule < hull <
// triangle so pairs canonicalize to Box3D's argument order (bigger shape first, triangles
// always the receiver).
struct CF_B3Shape
{
	int kind; // 0 sphere, 1 capsule, 2 hull (aabb), 3 triangle.
	b3Sphere sphere;
	b3Capsule capsule;
	b3BoxHull box;
	CF_Triangle3 tri;
};

static CF_B3Shape s_shape3(const void* shape, CF_ShapeType3 type)
{
	CF_B3Shape s;
	s.kind = -1;
	switch (type) {
	case CF_SHAPE3_TYPE_SPHERE:   s.kind = 0; s.sphere = *(const b3Sphere*)shape; break;
	case CF_SHAPE3_TYPE_CAPSULE:  s.kind = 1; s.capsule = *(const b3Capsule*)shape; break;
	case CF_SHAPE3_TYPE_AABB:
	{
		s.kind = 2;
		CF_Aabb3 bb = *(const CF_Aabb3*)shape;
		CF_V3 he = cf_half_extents_aabb3(bb);
		s.box = b3MakeOffsetBoxHull(he.x, he.y, he.z, s_b3(cf_center_aabb3(bb)));
	}	break;
	case CF_SHAPE3_TYPE_TRIANGLE: s.kind = 3; s.tri = *(const CF_Triangle3*)shape; break;
	default: break;
	}
	return s;
}

static CF_Manifold3 s_collide3(const CF_B3Shape* A, const CF_B3Shape* B)
{
	CF_Manifold3 none = { };
	if (A->kind < 0 || B->kind < 0) return none;
	if (A->kind == 3 && B->kind == 3) return none; // Triangle-vs-triangle unsupported.

	// Canonicalize: triangles always collide as the B side (they are receivers in Box3D);
	// among the convex shapes the bigger kind goes first, matching Box3D's argument order.
	bool flip = false;
	if (A->kind == 3) {
		flip = true; // B is not a triangle here (both-triangles already returned above).
	} else if (B->kind != 3 && A->kind < B->kind) {
		flip = true;
	}
	if (flip) {
		const CF_B3Shape* t = A;
		A = B;
		B = t;
	}

	b3LocalManifoldPoint point_buffer[CF_B3_POINT_CAPACITY];
	b3LocalManifold m = { };
	m.points = point_buffer;
	b3Transform identity = b3Transform_identity;
	bool normal_flip = flip;

	if (B->kind == 3) {
		// Shape versus a one-sided triangle. Box3D's triangle receivers report the normal
		// pointing from the TRIANGLE toward the shape (the push-out direction), opposite
		// the convex pairs' A-to-B convention -- so the flip inverts here to keep
		// CF_Manifold3's contract.
		normal_flip = !flip;
		b3Vec3 tri[3] = { s_b3(B->tri.a), s_b3(B->tri.b), s_b3(B->tri.c) };
		switch (A->kind) {
		case 0: b3CollideSphereAndTriangle(&m, CF_B3_POINT_CAPACITY, &A->sphere, tri); break;
		case 1:
		{
			b3SimplexCache cache = { };
			b3CollideCapsuleAndTriangle(&m, CF_B3_POINT_CAPACITY, &A->capsule, tri, &cache);
		}	break;
		case 2:
		{
			b3SATCache cache = { };
			b3CollideHullAndTriangle(&m, CF_B3_POINT_CAPACITY, &A->box.base, tri[0], tri[1], tri[2], 0, &cache);
		}	break;
		default: return none;
		}
	} else {
		switch (A->kind * 3 + B->kind) {
		case 0 * 3 + 0: b3CollideSpheres(&m, CF_B3_POINT_CAPACITY, &A->sphere, &B->sphere, identity); break;
		case 1 * 3 + 0: b3CollideCapsuleAndSphere(&m, CF_B3_POINT_CAPACITY, &A->capsule, &B->sphere, identity); break;
		case 1 * 3 + 1: b3CollideCapsules(&m, CF_B3_POINT_CAPACITY, &A->capsule, &B->capsule, identity); break;
		case 2 * 3 + 0:
		{
			b3SimplexCache cache = { };
			b3CollideHullAndSphere(&m, CF_B3_POINT_CAPACITY, &A->box.base, &B->sphere, identity, &cache);
		}	break;
		case 2 * 3 + 1:
		{
			b3SimplexCache cache = { };
			b3CollideHullAndCapsule(&m, CF_B3_POINT_CAPACITY, &A->box.base, &B->capsule, identity, &cache);
		}	break;
		case 2 * 3 + 2:
		{
			b3SATCache cache = { };
			b3CollideHulls(&m, CF_B3_POINT_CAPACITY, &A->box.base, &B->box.base, identity, &cache);
		}	break;
		default: return none;
		}
	}

	return s_manifold3(&m, normal_flip);
}

static CF_Manifold3 s_collide3(const void* A, CF_ShapeType3 typeA, const void* B, CF_ShapeType3 typeB)
{
	CF_B3Shape a = s_shape3(A, typeA);
	CF_B3Shape b = s_shape3(B, typeB);
	return s_collide3(&a, &b);
}

//--------------------------------------------------------------------------------------------------
// Boolean tests. The sphere/aabb pairs already exist as header inlines; these cover the
// capsule and triangle pairs.

bool cf_sphere_to_capsule3(CF_Sphere A, CF_Capsule3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_SPHERE, &B, CF_SHAPE3_TYPE_CAPSULE).count > 0;
}

bool cf_aabb3_to_capsule3(CF_Aabb3 A, CF_Capsule3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_AABB, &B, CF_SHAPE3_TYPE_CAPSULE).count > 0;
}

bool cf_capsule3_to_capsule3(CF_Capsule3 A, CF_Capsule3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_CAPSULE, &B, CF_SHAPE3_TYPE_CAPSULE).count > 0;
}

bool cf_sphere_to_triangle3(CF_Sphere A, CF_Triangle3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_SPHERE, &B, CF_SHAPE3_TYPE_TRIANGLE).count > 0;
}

bool cf_aabb3_to_triangle3(CF_Aabb3 A, CF_Triangle3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_AABB, &B, CF_SHAPE3_TYPE_TRIANGLE).count > 0;
}

bool cf_capsule3_to_triangle3(CF_Capsule3 A, CF_Triangle3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_CAPSULE, &B, CF_SHAPE3_TYPE_TRIANGLE).count > 0;
}

//--------------------------------------------------------------------------------------------------
// Manifolds.

CF_Manifold3 cf_sphere_to_sphere_manifold(CF_Sphere A, CF_Sphere B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_SPHERE, &B, CF_SHAPE3_TYPE_SPHERE);
}

CF_Manifold3 cf_sphere_to_aabb3_manifold(CF_Sphere A, CF_Aabb3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_SPHERE, &B, CF_SHAPE3_TYPE_AABB);
}

CF_Manifold3 cf_sphere_to_capsule3_manifold(CF_Sphere A, CF_Capsule3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_SPHERE, &B, CF_SHAPE3_TYPE_CAPSULE);
}

CF_Manifold3 cf_aabb3_to_aabb3_manifold(CF_Aabb3 A, CF_Aabb3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_AABB, &B, CF_SHAPE3_TYPE_AABB);
}

CF_Manifold3 cf_aabb3_to_capsule3_manifold(CF_Aabb3 A, CF_Capsule3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_AABB, &B, CF_SHAPE3_TYPE_CAPSULE);
}

CF_Manifold3 cf_capsule3_to_capsule3_manifold(CF_Capsule3 A, CF_Capsule3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_CAPSULE, &B, CF_SHAPE3_TYPE_CAPSULE);
}

CF_Manifold3 cf_sphere_to_triangle3_manifold(CF_Sphere A, CF_Triangle3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_SPHERE, &B, CF_SHAPE3_TYPE_TRIANGLE);
}

CF_Manifold3 cf_aabb3_to_triangle3_manifold(CF_Aabb3 A, CF_Triangle3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_AABB, &B, CF_SHAPE3_TYPE_TRIANGLE);
}

CF_Manifold3 cf_capsule3_to_triangle3_manifold(CF_Capsule3 A, CF_Triangle3 B)
{
	return s_collide3(&A, CF_SHAPE3_TYPE_CAPSULE, &B, CF_SHAPE3_TYPE_TRIANGLE);
}

//--------------------------------------------------------------------------------------------------
// Raycasts. CF rays are origin + normalized direction + length; Box3D casts are origin +
// translation, so `t` converts through the ray length, exactly like the 2d routing.

static CF_Raycast3 s_raycast3(b3CastOutput out, float ray_t)
{
	CF_Raycast3 result = { };
	if (out.hit) {
		result.hit = true;
		result.t = out.fraction * ray_t;
		result.n = s_cf(out.normal);
	}
	return result;
}

static b3RayCastInput s_ray3(CF_Ray3 ray)
{
	b3RayCastInput in;
	in.origin = s_b3(ray.p);
	in.translation = s_b3(ray.d * ray.t);
	in.maxFraction = 1.0f;
	return in;
}

CF_Raycast3 cf_ray3_to_capsule3(CF_Ray3 A, CF_Capsule3 B)
{
	b3RayCastInput in = s_ray3(A);
	return s_raycast3(b3RayCastCapsule((const b3Capsule*)&B, &in), A.t);
}

CF_Raycast3 cf_ray3_to_triangle3(CF_Ray3 A, CF_Triangle3 B)
{
	// Moller-Trumbore, two-sided: picking rays should hit the level from either side.
	CF_Raycast3 result = { };
	v3 e1 = B.b - B.a;
	v3 e2 = B.c - B.a;
	v3 p = cross(A.d, e2);
	float det = dot(e1, p);
	if (cf_abs(det) < 1e-12f) return result; // Parallel to the triangle plane.
	float inv_det = 1.0f / det;
	v3 s = A.p - B.a;
	float u = dot(s, p) * inv_det;
	if (u < 0 || u > 1.0f) return result;
	v3 q = cross(s, e1);
	float v = dot(A.d, q) * inv_det;
	if (v < 0 || u + v > 1.0f) return result;
	float t = dot(e2, q) * inv_det;
	if (t < 0 || t > A.t) return result;
	result.hit = true;
	result.t = t;
	v3 n = norm(cross(e1, e2));
	result.n = dot(n, A.d) > 0 ? -n : n; // Face back along the ray.
	return result;
}

//--------------------------------------------------------------------------------------------------
// Generic dispatch, distance, and sweeps.

// Any CF shape as a GJK point cloud. The proxy references this struct's storage, so keep
// it alive for the duration of the query.
struct CF_B3Proxy
{
	b3Vec3 points[8];
	b3ShapeProxy proxy;
};

static void s_proxy3(CF_B3Proxy* p, const void* shape, CF_ShapeType3 type, bool use_radius)
{
	p->proxy.points = p->points;
	p->proxy.count = 0;
	p->proxy.radius = 0;
	switch (type) {
	case CF_SHAPE3_TYPE_SPHERE:
	{
		const CF_Sphere* sphere = (const CF_Sphere*)shape;
		p->points[0] = s_b3(sphere->p);
		p->proxy.count = 1;
		p->proxy.radius = sphere->r;
	}	break;

	case CF_SHAPE3_TYPE_AABB:
	{
		CF_Aabb3 bb = *(const CF_Aabb3*)shape;
		p->points[0] = s_b3(cf_v3(bb.min.x, bb.min.y, bb.min.z));
		p->points[1] = s_b3(cf_v3(bb.max.x, bb.min.y, bb.min.z));
		p->points[2] = s_b3(cf_v3(bb.max.x, bb.max.y, bb.min.z));
		p->points[3] = s_b3(cf_v3(bb.min.x, bb.max.y, bb.min.z));
		p->points[4] = s_b3(cf_v3(bb.min.x, bb.min.y, bb.max.z));
		p->points[5] = s_b3(cf_v3(bb.max.x, bb.min.y, bb.max.z));
		p->points[6] = s_b3(cf_v3(bb.max.x, bb.max.y, bb.max.z));
		p->points[7] = s_b3(cf_v3(bb.min.x, bb.max.y, bb.max.z));
		p->proxy.count = 8;
	}	break;

	case CF_SHAPE3_TYPE_CAPSULE:
	{
		const CF_Capsule3* capsule = (const CF_Capsule3*)shape;
		p->points[0] = s_b3(capsule->a);
		p->points[1] = s_b3(capsule->b);
		p->proxy.count = 2;
		p->proxy.radius = capsule->r;
	}	break;

	case CF_SHAPE3_TYPE_TRIANGLE:
	{
		const CF_Triangle3* tri = (const CF_Triangle3*)shape;
		p->points[0] = s_b3(tri->a);
		p->points[1] = s_b3(tri->b);
		p->points[2] = s_b3(tri->c);
		p->proxy.count = 3;
	}	break;

	default: break;
	}
	if (!use_radius) p->proxy.radius = 0;
}

float cf_gjk3(const void* A, CF_ShapeType3 typeA, const void* B, CF_ShapeType3 typeB, CF_V3* outA, CF_V3* outB, bool use_radius, int* iterations, CF_GjkCache3* cache)
{
	CF_B3Proxy pa, pb;
	s_proxy3(&pa, A, typeA, use_radius);
	s_proxy3(&pb, B, typeB, use_radius);
	b3DistanceInput in = { };
	in.proxyA = pa.proxy;
	in.proxyB = pb.proxy;
	in.transform = b3Transform_identity;
	in.useRadii = use_radius;
	b3SimplexCache local = { };
	b3SimplexCache* c = cache ? (b3SimplexCache*)cache : &local;
	b3DistanceOutput out = b3ShapeDistance(&in, c, NULL, 0);
	if (outA) *outA = s_cf(out.pointA);
	if (outB) *outB = s_cf(out.pointB);
	if (iterations) *iterations = out.iterations;
	return out.distance;
}

CF_ToiResult3 cf_toi3(const void* A, CF_ShapeType3 typeA, CF_V3 vA, const void* B, CF_ShapeType3 typeB, CF_V3 vB, bool use_radius)
{
	// A linear shape cast of B against A under their relative motion, same contract as the
	// 2d cf_toi: initially-touching shapes report a miss with toi 1.
	CF_B3Proxy pa, pb;
	s_proxy3(&pa, A, typeA, use_radius);
	s_proxy3(&pb, B, typeB, use_radius);
	b3ShapeCastPairInput in = { };
	in.proxyA = pa.proxy;
	in.proxyB = pb.proxy;
	in.transform = b3Transform_identity;
	in.translationB = s_b3(vB - vA);
	in.maxFraction = 1.0f;
	b3CastOutput out = b3ShapeCast(&in);
	CF_ToiResult3 result = { };
	result.hit = out.hit;
	result.toi = out.hit ? out.fraction : 1.0f;
	result.n = s_cf(out.normal);
	result.p = s_cf(out.point);
	result.iterations = out.iterations;
	return result;
}

int cf_collided3(const void* A, CF_ShapeType3 typeA, const void* B, CF_ShapeType3 typeB)
{
	return s_collide3(A, typeA, B, typeB).count > 0;
}

void cf_collide3(const void* A, CF_ShapeType3 typeA, const void* B, CF_ShapeType3 typeB, CF_Manifold3* m)
{
	*m = s_collide3(A, typeA, B, typeB);
}

bool cf_cast_ray3(CF_Ray3 A, const void* B, CF_ShapeType3 typeB, CF_Raycast3* out)
{
	switch (typeB) {
	case CF_SHAPE3_TYPE_SPHERE:
	{
		*out = cf_ray3_to_sphere(A, *(const CF_Sphere*)B);
	}	break;

	case CF_SHAPE3_TYPE_AABB:
	{
		*out = cf_ray3_to_aabb3(A, *(const CF_Aabb3*)B);
	}	break;

	case CF_SHAPE3_TYPE_CAPSULE:
	{
		*out = cf_ray3_to_capsule3(A, *(const CF_Capsule3*)B);
	}	break;

	case CF_SHAPE3_TYPE_TRIANGLE:
	{
		*out = cf_ray3_to_triangle3(A, *(const CF_Triangle3*)B);
	}	break;

	default:
	{
		CF_Raycast3 none = { };
		*out = none;
	}	break;
	}
	return out->hit;
}
