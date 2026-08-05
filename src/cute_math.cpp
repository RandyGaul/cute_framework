/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_defines.h>
#include <cute_c_runtime.h>

#include <cute_math.h>

// The stateless collision queries route to Box2D's freestanding geometry layer
// (box2d/collision.h) -- no b2World or simulation state is involved anywhere in this file. CF shapes stay the API currency; where layouts are not
// bit-identical (polygons, manifolds, cast results) real conversions happen here.
#include <box2d/collision.h>
#include <box2d/math_functions.h>

CF_STATIC_ASSERT(CF_POLY_MAX_VERTS == B2_MAX_POLYGON_VERTICES, "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_V2) == sizeof(b2Vec2), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Circle) == sizeof(b2Circle), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Capsule) == sizeof(b2Capsule), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Aabb) == sizeof(b2AABB), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_GjkCache) == sizeof(b2SimplexCache), "Must be equal.");

using namespace Cute;

CF_V2 cf_center_of_mass(CF_Poly poly)
{
	if (poly.count <= 0) return V2(0,0);
	if (poly.count == 1) return poly.verts[0];
	if (poly.count == 2) return (poly.verts[0] + poly.verts[1]) * 0.5f;
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

	// Degenerate (zero-area) polygon -- fall back to the vertex average.
	if (area_sum == 0) {
		v2 average = V2(0,0);
		for (int i = 0; i < poly.count; ++i) average += poly.verts[i];
		return average * (1.0f / poly.count);
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
	// Each iteration pushes at most 2 verts per buffer (intersection + vertex).
	// The documented contract is a convex polygon (at most CF_POLY_MAX_VERTS+1
	// pushes), but out-of-contract non-convex input can cross the plane on every
	// edge, so size for the true worst case rather than smashing the stack.
	v2 front[CF_POLY_MAX_VERTS*2];
	v2 back[CF_POLY_MAX_VERTS*2];
	v2 a = slice_me.verts[slice_me.count - 1];
	float da = distance(slice_plane, a);

	for (int i = 0; i < slice_me.count; ++i) {
		v2 b = slice_me.verts[i];
		float db = distance(slice_plane, b);

		if (in_front(db, k_epsilon)) {
			if(behind(da, k_epsilon)) {
				v2 p = intersect(b, a, db, da);
				front[front_count++] = p;
				back[back_count++] = p;
			}
			front[front_count++] = b;
		} else if (behind(db, k_epsilon)) {
			if (in_front(da, k_epsilon)) {
				v2 p = intersect(a, b, da, db);
				front[front_count++] = p;
				back[back_count++] = p;
			}
			back[back_count++] = b;
		} else {
			// On-plane vertices lie on the boundary of both halves.
			front[front_count++] = b;
			back[back_count++] = b;
		}

		a = b;
		da = db;
	}

	// More than CF_POLY_MAX_VERTS verts can be generated in a single polygon, truncate to CF_POLY_MAX_VERTS.
	CF_SliceOutput out = { };
	out.front.count = min(CF_POLY_MAX_VERTS, front_count);
	out.back.count = min(CF_POLY_MAX_VERTS, back_count);
	CF_MEMCPY(out.front.verts, front, sizeof(v2) * out.front.count);
	CF_MEMCPY(out.back.verts, back, sizeof(v2) * out.back.count);
	norms(out.front.verts, out.front.norms, out.front.count);
	norms(out.back.verts, out.back.norms, out.back.count);
	return out;
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
	// Degenerate (zero-area) polygon -- fall back to the vertex average.
	if (area_sum == 0) {
		CF_V2 average = cf_v2(0, 0);
		for (int i = 0; i < count; ++i) average = average + verts[i];
		return average * (1.0f / count);
	}
	return c * (1.0f / area_sum) + p0;
}

//--------------------------------------------------------------------------------------------------
// Hull, normals, and inflation. These are CF-native: cf_norms/cf_inflate preserve
// cute_c2's exact behavior (including negative skin factors deflating polygons via the
// dual method), while cf_hull rides Box2D's hull builder.

int cf_hull(CF_V2* verts, int count)
{
	if (count <= 2) return 0;
	if (count > CF_POLY_MAX_VERTS) count = CF_POLY_MAX_VERTS;
	b2Hull hull = b2ComputeHull((const b2Vec2*)verts, count);
	for (int i = 0; i < hull.count; ++i) verts[i] = cf_v2(hull.points[i].x, hull.points[i].y);
	return hull.count;
}

void cf_norms(CF_V2* verts, CF_V2* norms, int count)
{
	// Outward normals for counter-clockwise winding: each edge rotated clockwise 90 degrees.
	for (int i = 0; i < count; ++i) {
		int a = i;
		int b = i + 1 < count ? i + 1 : 0;
		v2 e = verts[b] - verts[a];
		norms[i] = norm(V2(e.y, -e.x));
	}
}

void cf_make_poly(CF_Poly* p)
{
	p->count = cf_hull(p->verts, p->count);
	cf_norms(p->verts, p->norms, p->count);
}

// Inflating a polytope, idea by Dirk Gregorius ~ 2015 (as shipped in cute_c2). Each plane
// maps to a point by involution (dividing the plane normal by its offset); the dual of the
// dual, with planes pushed out by the skin factor in between, is the inflated polygon.
// Works for negative skin factors (deflation) too, which is why this stays hand-rolled:
// Box2D's rounded-polygon radius cannot go negative.
static CF_Poly s_dual(CF_Poly poly, float skin_factor)
{
	CF_Poly dual;
	dual.count = poly.count;

	// plane = a * x + b * y - d
	// dual = { a / d, b / d }
	for (int i = 0; i < poly.count; ++i) {
		v2 n = poly.norms[i];
		float d = dot(n, poly.verts[i]) + skin_factor;
		if (d == 0) dual.verts[i] = V2(0, 0);
		else dual.verts[i] = n / d;
	}

	// The vertices remain in proper CCW order, so only the normals must be recomputed.
	cf_norms(dual.verts, dual.norms, dual.count);

	return dual;
}

static CF_Poly s_inflate_poly(CF_Poly poly, float skin_factor)
{
	// Center the average onto the origin so every plane offset is positive.
	v2 average = poly.verts[0];
	for (int i = 1; i < poly.count; ++i) average += poly.verts[i];
	average = average / (float)poly.count;
	for (int i = 0; i < poly.count; ++i) poly.verts[i] -= average;

	CF_Poly dual = s_dual(poly, skin_factor);
	poly = s_dual(dual, 0);

	for (int i = 0; i < poly.count; ++i) poly.verts[i] += average;
	return poly;
}

void cf_inflate(void* shape, CF_ShapeType type, float skin_factor)
{
	switch (type) {
	case CF_SHAPE_TYPE_CIRCLE:
	{
		CF_Circle* circle = (CF_Circle*)shape;
		circle->r += skin_factor;
	}	break;

	case CF_SHAPE_TYPE_AABB:
	{
		CF_Aabb* bb = (CF_Aabb*)shape;
		v2 factor = V2(skin_factor, skin_factor);
		bb->min -= factor;
		bb->max += factor;
	}	break;

	case CF_SHAPE_TYPE_CAPSULE:
	{
		CF_Capsule* capsule = (CF_Capsule*)shape;
		capsule->r += skin_factor;
	}	break;

	case CF_SHAPE_TYPE_POLY:
	{
		CF_Poly* poly = (CF_Poly*)shape;
		*poly = s_inflate_poly(*poly, skin_factor);
	}	break;

	default: break;
	}
}

//--------------------------------------------------------------------------------------------------
// Box2D interop. CF_Circle/CF_Capsule/CF_Aabb are bit-compatible with their Box2D
// counterparts (asserted above); polygons, proxies, manifolds and cast outputs convert.

CF_INLINE b2Vec2 s_b2(CF_V2 v) { b2Vec2 out; out.x = v.x; out.y = v.y; return out; }
CF_INLINE CF_V2 s_cf(b2Vec2 v) { return cf_v2(v.x, v.y); }

static b2Polygon s_poly(const CF_Poly* p)
{
	b2Polygon out = { };
	out.count = p->count;
	for (int i = 0; i < p->count; ++i) {
		out.vertices[i] = s_b2(p->verts[i]);
		out.normals[i] = s_b2(p->norms[i]);
	}
	out.centroid = s_b2(cf_centroid(p->verts, p->count));
	out.radius = 0;
	return out;
}

static b2Polygon s_poly(CF_Aabb bb)
{
	v2 he = cf_half_extents(bb);
	return b2MakeOffsetBox(he.x, he.y, s_b2(cf_center(bb)), b2Rot_identity);
}

// Box2D's speculative margin reports nearby-but-separated contact points; a CF_Manifold
// only ever describes shapes that actually touch, so separated points are dropped and
// separation negates into penetration depth. `flip` restores the normal to point from A
// to B after a canonicalizing pair swap.
static CF_Manifold s_manifold(b2Manifold m, bool flip)
{
	CF_Manifold out = { };
	v2 n = s_cf(m.normal);
	out.n = flip ? -n : n;
	for (int i = 0; i < m.pointCount; ++i) {
		if (m.points[i].separation <= 0) {
			out.contact_points[out.count] = s_cf(m.points[i].point);
			out.depths[out.count] = -m.points[i].separation;
			++out.count;
		}
	}
	return out;
}

// A CF shape lifted into Box2D terms. AABBs ride as box polygons since Box2D has no
// standalone AABB shape.
struct CF_B2Shape
{
	int kind; // 0 circle, 1 capsule, 2 polygon -- ranked to match Box2D's pair order.
	b2Circle circle;
	b2Capsule capsule;
	b2Polygon poly;
};

static CF_B2Shape s_shape(const void* shape, CF_ShapeType type)
{
	CF_B2Shape s;
	s.kind = -1;
	switch (type) {
	case CF_SHAPE_TYPE_CIRCLE:  s.kind = 0; s.circle = *(const b2Circle*)shape; break;
	case CF_SHAPE_TYPE_CAPSULE: s.kind = 1; s.capsule = *(const b2Capsule*)shape; break;
	case CF_SHAPE_TYPE_AABB:    s.kind = 2; s.poly = s_poly(*(const CF_Aabb*)shape); break;
	case CF_SHAPE_TYPE_POLY:    s.kind = 2; s.poly = s_poly((const CF_Poly*)shape); break;
	default: break;
	}
	return s;
}

// Dispatches over Box2D's manifold pair matrix. Box2D orders each pair function with the
// "larger" shape kind first; when the caller's order is swapped to match, the manifold
// normal flips back so it always points from A to B, per CF_Manifold's contract.
static CF_Manifold s_collide(const CF_B2Shape* A, const CF_B2Shape* B)
{
	bool flip = A->kind < B->kind;
	if (flip) {
		const CF_B2Shape* t = A;
		A = B;
		B = t;
	}
	if (A->kind < 0 || B->kind < 0) {
		CF_Manifold none = { };
		return none;
	}
	b2Transform xf = b2Transform_identity;
	b2Manifold m = { };
	switch (A->kind * 3 + B->kind) {
	case 0 * 3 + 0: m = b2CollideCircles(&A->circle, xf, &B->circle, xf); break;
	case 1 * 3 + 0: m = b2CollideCapsuleAndCircle(&A->capsule, xf, &B->circle, xf); break;
	case 1 * 3 + 1: m = b2CollideCapsules(&A->capsule, xf, &B->capsule, xf); break;
	case 2 * 3 + 0: m = b2CollidePolygonAndCircle(&A->poly, xf, &B->circle, xf); break;
	case 2 * 3 + 1: m = b2CollidePolygonAndCapsule(&A->poly, xf, &B->capsule, xf); break;
	case 2 * 3 + 2: m = b2CollidePolygons(&A->poly, xf, &B->poly, xf); break;
	default: break;
	}
	return s_manifold(m, flip);
}

static CF_Manifold s_collide(const void* A, CF_ShapeType typeA, const void* B, CF_ShapeType typeB)
{
	CF_B2Shape a = s_shape(A, typeA);
	CF_B2Shape b = s_shape(B, typeB);
	return s_collide(&a, &b);
}

// Any CF shape as a GJK point cloud for distance/cast queries. `use_radius` false strips
// the radii, treating circles as points and capsules as segments (cute_c2's convention).
static b2ShapeProxy s_proxy(const void* shape, CF_ShapeType type, bool use_radius)
{
	b2ShapeProxy p = { };
	switch (type) {
	case CF_SHAPE_TYPE_CIRCLE:
	{
		const CF_Circle* circle = (const CF_Circle*)shape;
		p.points[0] = s_b2(circle->p);
		p.count = 1;
		p.radius = circle->r;
	}	break;

	case CF_SHAPE_TYPE_AABB:
	{
		const CF_Aabb* bb = (const CF_Aabb*)shape;
		p.points[0] = s_b2(bb->min);
		p.points[1] = s_b2(cf_v2(bb->max.x, bb->min.y));
		p.points[2] = s_b2(bb->max);
		p.points[3] = s_b2(cf_v2(bb->min.x, bb->max.y));
		p.count = 4;
	}	break;

	case CF_SHAPE_TYPE_CAPSULE:
	{
		const CF_Capsule* capsule = (const CF_Capsule*)shape;
		p.points[0] = s_b2(capsule->a);
		p.points[1] = s_b2(capsule->b);
		p.count = 2;
		p.radius = capsule->r;
	}	break;

	case CF_SHAPE_TYPE_POLY:
	{
		const CF_Poly* poly = (const CF_Poly*)shape;
		for (int i = 0; i < poly->count; ++i) p.points[i] = s_b2(poly->verts[i]);
		p.count = poly->count;
	}	break;

	default: break;
	}
	if (!use_radius) p.radius = 0;
	return p;
}

//--------------------------------------------------------------------------------------------------
// Boolean tests.

bool cf_circle_to_circle(CF_Circle A, CF_Circle B)
{
	v2 d = B.p - A.p;
	float r = A.r + B.r;
	return dot(d, d) < r * r;
}

bool cf_circle_to_aabb(CF_Circle A, CF_Aabb B)
{
	v2 L = cf_clamp_aabb_v2(B, A.p);
	v2 d = A.p - L;
	return dot(d, d) < A.r * A.r;
}

bool cf_circle_to_capsule(CF_Circle A, CF_Capsule B)
{
	return s_collide(&A, CF_SHAPE_TYPE_CIRCLE, &B, CF_SHAPE_TYPE_CAPSULE).count > 0;
}

bool cf_aabb_to_aabb(CF_Aabb A, CF_Aabb B)
{
	return cf_overlaps(A, B);
}

bool cf_aabb_to_capsule(CF_Aabb A, CF_Capsule B)
{
	return s_collide(&A, CF_SHAPE_TYPE_AABB, &B, CF_SHAPE_TYPE_CAPSULE).count > 0;
}

bool cf_capsule_to_capsule(CF_Capsule A, CF_Capsule B)
{
	return s_collide(&A, CF_SHAPE_TYPE_CAPSULE, &B, CF_SHAPE_TYPE_CAPSULE).count > 0;
}

bool cf_circle_to_poly(CF_Circle A, const CF_Poly* B)
{
	return s_collide(&A, CF_SHAPE_TYPE_CIRCLE, B, CF_SHAPE_TYPE_POLY).count > 0;
}

bool cf_aabb_to_poly(CF_Aabb A, const CF_Poly* B)
{
	return s_collide(&A, CF_SHAPE_TYPE_AABB, B, CF_SHAPE_TYPE_POLY).count > 0;
}

bool cf_capsule_to_poly(CF_Capsule A, const CF_Poly* B)
{
	return s_collide(&A, CF_SHAPE_TYPE_CAPSULE, B, CF_SHAPE_TYPE_POLY).count > 0;
}

bool cf_poly_to_poly(const CF_Poly* A, const CF_Poly* B)
{
	return s_collide(A, CF_SHAPE_TYPE_POLY, B, CF_SHAPE_TYPE_POLY).count > 0;
}

//--------------------------------------------------------------------------------------------------
// Raycasts. CF rays are origin + normalized direction + length; Box2D casts are origin +
// translation, so `t` converts through the ray length on the way in and out.

static CF_Raycast s_raycast(b2CastOutput out, float ray_t)
{
	CF_Raycast result = { };
	if (out.hit) {
		result.hit = true;
		result.t = out.fraction * ray_t;
		result.n = s_cf(out.normal);
	}
	return result;
}

static b2RayCastInput s_ray(CF_Ray ray)
{
	b2RayCastInput in;
	in.origin = s_b2(ray.p);
	in.translation = s_b2(ray.d * ray.t);
	in.maxFraction = 1.0f;
	return in;
}

CF_Raycast cf_ray_to_circle(CF_Ray A, CF_Circle B)
{
	b2RayCastInput in = s_ray(A);
	return s_raycast(b2RayCastCircle(&in, (const b2Circle*)&B), A.t);
}

CF_Raycast cf_ray_to_aabb(CF_Ray A, CF_Aabb B)
{
	b2RayCastInput in = s_ray(A);
	b2Polygon poly = s_poly(B);
	return s_raycast(b2RayCastPolygon(&in, &poly), A.t);
}

CF_Raycast cf_ray_to_capsule(CF_Ray A, CF_Capsule B)
{
	b2RayCastInput in = s_ray(A);
	return s_raycast(b2RayCastCapsule(&in, (const b2Capsule*)&B), A.t);
}

CF_Raycast cf_ray_to_poly(CF_Ray A, const CF_Poly* B)
{
	b2RayCastInput in = s_ray(A);
	b2Polygon poly = s_poly(B);
	return s_raycast(b2RayCastPolygon(&in, &poly), A.t);
}

//--------------------------------------------------------------------------------------------------
// Manifolds.

CF_Manifold cf_circle_to_circle_manifold(CF_Circle A, CF_Circle B)
{
	return s_collide(&A, CF_SHAPE_TYPE_CIRCLE, &B, CF_SHAPE_TYPE_CIRCLE);
}

CF_Manifold cf_circle_to_aabb_manifold(CF_Circle A, CF_Aabb B)
{
	return s_collide(&A, CF_SHAPE_TYPE_CIRCLE, &B, CF_SHAPE_TYPE_AABB);
}

CF_Manifold cf_circle_to_capsule_manifold(CF_Circle A, CF_Capsule B)
{
	return s_collide(&A, CF_SHAPE_TYPE_CIRCLE, &B, CF_SHAPE_TYPE_CAPSULE);
}

CF_Manifold cf_aabb_to_aabb_manifold(CF_Aabb A, CF_Aabb B)
{
	return s_collide(&A, CF_SHAPE_TYPE_AABB, &B, CF_SHAPE_TYPE_AABB);
}

CF_Manifold cf_aabb_to_capsule_manifold(CF_Aabb A, CF_Capsule B)
{
	return s_collide(&A, CF_SHAPE_TYPE_AABB, &B, CF_SHAPE_TYPE_CAPSULE);
}

CF_Manifold cf_capsule_to_capsule_manifold(CF_Capsule A, CF_Capsule B)
{
	return s_collide(&A, CF_SHAPE_TYPE_CAPSULE, &B, CF_SHAPE_TYPE_CAPSULE);
}

CF_Manifold cf_circle_to_poly_manifold(CF_Circle A, const CF_Poly* B)
{
	return s_collide(&A, CF_SHAPE_TYPE_CIRCLE, B, CF_SHAPE_TYPE_POLY);
}

CF_Manifold cf_aabb_to_poly_manifold(CF_Aabb A, const CF_Poly* B)
{
	return s_collide(&A, CF_SHAPE_TYPE_AABB, B, CF_SHAPE_TYPE_POLY);
}

CF_Manifold cf_capsule_to_poly_manifold(CF_Capsule A, const CF_Poly* B)
{
	return s_collide(&A, CF_SHAPE_TYPE_CAPSULE, B, CF_SHAPE_TYPE_POLY);
}

CF_Manifold cf_poly_to_poly_manifold(const CF_Poly* A, const CF_Poly* B)
{
	return s_collide(A, CF_SHAPE_TYPE_POLY, B, CF_SHAPE_TYPE_POLY);
}

//--------------------------------------------------------------------------------------------------
// Generic dispatch, distance, and sweeps.

float cf_gjk(const void* A, CF_ShapeType typeA, const void* B, CF_ShapeType typeB, CF_V2* outA, CF_V2* outB, bool use_radius, int* iterations, CF_GjkCache* cache)
{
	b2DistanceInput in = { };
	in.proxyA = s_proxy(A, typeA, use_radius);
	in.proxyB = s_proxy(B, typeB, use_radius);
	in.transformA = b2Transform_identity;
	in.transformB = b2Transform_identity;
	in.useRadii = use_radius;
	b2SimplexCache local = { };
	b2SimplexCache* c = cache ? (b2SimplexCache*)cache : &local;
	b2DistanceOutput out = b2ShapeDistance(&in, c, NULL, 0);
	if (outA) *outA = s_cf(out.pointA);
	if (outB) *outB = s_cf(out.pointB);
	if (iterations) *iterations = out.iterations;
	return out.distance;
}

CF_ToiResult cf_toi(const void* A, CF_ShapeType typeA, CF_V2 vA, const void* B, CF_ShapeType typeB, CF_V2 vB, int use_radius)
{
	// A linear shape cast of B against A under their relative motion. Initially-touching
	// shapes report a miss (with toi 1), unlike cute_c2's toi 0 -- sweep from a
	// non-overlapping configuration, e.g. after inflating by a small skin factor.
	b2ShapeCastPairInput in = { };
	in.proxyA = s_proxy(A, typeA, !!use_radius);
	in.proxyB = s_proxy(B, typeB, !!use_radius);
	in.transformA = b2Transform_identity;
	in.transformB = b2Transform_identity;
	in.translationB = s_b2(cf_v2(vB.x - vA.x, vB.y - vA.y));
	in.maxFraction = 1.0f;
	b2CastOutput out = b2ShapeCast(&in);
	CF_ToiResult result = { };
	result.hit = out.hit;
	result.toi = out.hit ? out.fraction : 1.0f;
	result.n = s_cf(out.normal);
	result.p = s_cf(out.point);
	result.iterations = out.iterations;
	return result;
}

int cf_collided(const void* A, CF_ShapeType typeA, const void* B, CF_ShapeType typeB)
{
	return s_collide(A, typeA, B, typeB).count > 0;
}

void cf_collide(const void* A, CF_ShapeType typeA, const void* B, CF_ShapeType typeB, CF_Manifold* m)
{
	*m = s_collide(A, typeA, B, typeB);
}

bool cf_cast_ray(CF_Ray A, const void* B, CF_ShapeType typeB, CF_Raycast* out)
{
	switch (typeB) {
	case CF_SHAPE_TYPE_CIRCLE:  *out = cf_ray_to_circle(A, *(const CF_Circle*)B); break;
	case CF_SHAPE_TYPE_AABB:    *out = cf_ray_to_aabb(A, *(const CF_Aabb*)B); break;
	case CF_SHAPE_TYPE_CAPSULE: *out = cf_ray_to_capsule(A, *(const CF_Capsule*)B); break;
	case CF_SHAPE_TYPE_POLY:    *out = cf_ray_to_poly(A, (const CF_Poly*)B); break;
	default: { CF_Raycast none = { }; *out = none; } break;
	}
	return out->hit;
}
