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

#define CUTE_MATH2D_NAMESPACE cute
#include <cute/cute_math2d.h>
#include <cute/cute_c2.h>

#include <cute_defines.h>

#define CUTE_MAX_POLYGON_VERTS C2_MAX_POLYGON_VERTS

namespace cute
{

// The majority of math types are in <cute/cute_math2d.h>, see this file for things like
// the vector, matrix, and transformation functions.

struct poly_t
{
	int count;
	v2 verts[CUTE_MAX_POLYGON_VERTS];
	v2 norms[CUTE_MAX_POLYGON_VERTS];
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
extern CUTE_API bool CUTE_CALL circle_to_circle(circle_t A, circle_t B);
extern CUTE_API bool CUTE_CALL circle_to_aabb(circle_t A, aabb_t B);
extern CUTE_API bool CUTE_CALL circle_to_capsule(circle_t A, capsule_t B);
extern CUTE_API bool CUTE_CALL aabb_to_aabb(aabb_t A, aabb_t B);
extern CUTE_API bool CUTE_CALL aabb_to_capsule(aabb_t A, capsule_t B);
extern CUTE_API bool CUTE_CALL capsule_to_capsule(capsule_t A, capsule_t B);
extern CUTE_API bool CUTE_CALL circle_to_poly(circle_t A, const poly_t* B, const transform_t* bx);
extern CUTE_API bool CUTE_CALL aabb_to_poly(aabb_t A, const poly_t* B, const transform_t* bx);
extern CUTE_API bool CUTE_CALL capsule_to_poly(capsule_t A, const poly_t* B, const transform_t* bx);
extern CUTE_API bool CUTE_CALL poly_to_poly(const poly_t* A, const transform_t* ax, const poly_t* B, const transform_t* bx);

// ray operations
// output is placed into the raycast_t struct, which represents the hit location
// of the ray. the out param contains no meaningful information if these funcs
// return 0
extern CUTE_API bool CUTE_CALL ray_to_circle(ray_t A, circle_t B, raycast_t* out);
extern CUTE_API bool CUTE_CALL ray_to_aabb(ray_t A, aabb_t B, raycast_t* out);
extern CUTE_API bool CUTE_CALL ray_to_capsule(ray_t A, capsule_t B, raycast_t* out);
extern CUTE_API bool CUTE_CALL ray_to_poly(ray_t A, const poly_t* B, const transform_t* bx_ptr, raycast_t* out);

// manifold generation
// these functions are (generally) slower than the boolean versions, but will compute one
// or two points that represent the plane of contact. This information is
// is usually needed to resolve and prevent shapes from colliding. If no coll
// ision occured the count member of the manifold struct is set to 0.
extern CUTE_API void CUTE_CALL circle_to_circle_manifold(circle_t A, circle_t B, manifold_t* m);
extern CUTE_API void CUTE_CALL circle_to_aabb_manifold(circle_t A, aabb_t B, manifold_t* m);
extern CUTE_API void CUTE_CALL circle_to_capsule_manifold(circle_t A, capsule_t B, manifold_t* m);
extern CUTE_API void CUTE_CALL aabb_to_aabb_manifold(aabb_t A, aabb_t B, manifold_t* m);
extern CUTE_API void CUTE_CALL aabb_to_capsule_manifold(aabb_t A, capsule_t B, manifold_t* m);
extern CUTE_API void CUTE_CALL capsule_to_capsule_manifold(capsule_t A, capsule_t B, manifold_t* m);
extern CUTE_API void CUTE_CALL circle_to_poly_manifold(circle_t A, const poly_t* B, const transform_t* bx, manifold_t* m);
extern CUTE_API void CUTE_CALL aabb_to_poly_manifold(aabb_t A, const poly_t* B, const transform_t* bx, manifold_t* m);
extern CUTE_API void CUTE_CALL capsule_to_poly_manifold(capsule_t A, const poly_t* B, const transform_t* bx, manifold_t* m);
extern CUTE_API void CUTE_CALL poly_to_poly_manifold(const poly_t* A, const transform_t* ax, const poly_t* B, const transform_t* bx, manifold_t* m);

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
extern CUTE_API float CUTE_CALL gjk(const void* A, shape_type_t typeA, const transform_t* ax_ptr, const void* B, shape_type_t typeB, const transform_t* bx_ptr, v2* outA, v2* outB, int use_radius, int* iterations, gjk_cache_t* cache);

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
extern CUTE_API float CUTE_CALL toi(const void* A, shape_type_t typeA, const transform_t* ax_ptr, v2 vA, const void* B, shape_type_t typeB, const transform_t* bx_ptr, v2 vB, int use_radius, int* iterations);

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
extern CUTE_API void CUTE_CALL inflate(void* shape, shape_type_t type, float skin_factor);

// Computes 2D convex hull. Will not do anything if less than two verts supplied. If
// more than C2_MAX_POLYGON_VERTS are supplied extras are ignored.
extern CUTE_API int CUTE_CALL hull(v2* verts, int count);
extern CUTE_API void CUTE_CALL norms(v2* verts, v2* norms, int count);

// runs c2Hull and c2Norms, assumes p->verts and p->count are both set to valid values
extern CUTE_API void CUTE_CALL make_poly(poly_t* p);

// Generic collision detection routines, useful for games that want to use some poly-
// morphism to write more generic-styled code. Internally calls various above functions.
// For AABBs/Circles/Capsules ax and bx are ignored. For polys ax and bx can define
// model to world transformations (for polys only), or be NULL for identity transforms.
extern CUTE_API int CUTE_CALL collided(const void* A, const transform_t* ax, shape_type_t typeA, const void* B, const transform_t* bx, shape_type_t typeB);
extern CUTE_API void CUTE_CALL collide(const void* A, const transform_t* ax, shape_type_t typeA, const void* B, const transform_t* bx, shape_type_t typeB, manifold_t* m);
extern CUTE_API bool CUTE_CALL cast_ray(ray_t A, const void* B, const transform_t* bx, shape_type_t typeB, raycast_t* out);

}
