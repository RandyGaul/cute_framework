/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_PHYSICS_H
#define CF_PHYSICS_H

#include "cute_defines.h"
#include "cute_math.h"
#include "cute_math3d.h"
#include "cute_time.h"

#include <box2d/box2d.h>
#include <box3d/box3d.h>

//--------------------------------------------------------------------------------------------------
// C API
//
// CF's physics is Box2D v3, exposed directly -- the full b2* API is available from this
// one include (the library is pinned and fetched by CF's CMake, compiled into CF itself,
// and wired to CF's allocator at app startup). There is deliberately no CF_Body wrapper layer:
// Box2D v3 is already a C API of id-handles and `b2Default*Def()` initializers, the same
// idiom as CF's own `cf_*_defaults()`. Anything a wrapper could rename, Box2D's docs
// already teach better.
//
// What this header adds is the seam between the two worlds:
//
//     - Interop converters between CF and Box2D math/shape types. Most are bit-identical
//       and free; the ones that are NOT free are the ones that would corrupt silently if
//       cast (`CF_SinCos` stores {sin, cos} while `b2Rot` stores {cos, sin} -- never blind
//       cast a rotation or transform).
//     - A debug-draw bridge: `cf_physics_draw` renders a world through CF's 2d draw API
//       (which means anti-aliased SDF shapes, layers, and the current 2d camera).
//     - A stepping helper: `cf_physics_step` ties `b2World_Step` to CF's fixed-timestep
//       clock. Enable `cf_set_fixed_timestep` and call it from your `CF_OnUpdateFn`.
//
// The stateless collision queries in cute_math.h (`cf_circle_to_poly`, manifolds,
// raycasts, `cf_gjk`, `cf_toi`, ...) are ALSO powered by Box2D's geometry layer -- use
// those when you want immediate shape-vs-shape answers with no world involved, and this
// header when you want simulation: bodies, joints, contact events, and world queries.

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// Interop: CF <-> Box2D types.

/**
 * @function cf_v2_to_b2
 * @category physics
 * @brief    Converts a `CF_V2` to a `b2Vec2`.
 * @remarks  The two are bit-identical; this exists for call-site clarity.
 * @related  cf_b2_to_v2 cf_sincos_to_b2 cf_transform_to_b2
 */
CF_INLINE b2Vec2 cf_v2_to_b2(CF_V2 v) { b2Vec2 out; out.x = v.x; out.y = v.y; return out; }

/**
 * @function cf_b2_to_v2
 * @category physics
 * @brief    Converts a `b2Vec2` to a `CF_V2`.
 * @related  cf_v2_to_b2 cf_b2_to_sincos cf_b2_to_transform
 */
CF_INLINE CF_V2 cf_b2_to_v2(b2Vec2 v) { return cf_v2(v.x, v.y); }

/**
 * @function cf_sincos_to_b2
 * @category physics
 * @brief    Converts a `CF_SinCos` rotation to a `b2Rot`.
 * @remarks  These are NOT bit-identical: `CF_SinCos` stores {sin, cos} while `b2Rot`
 *           stores {cos, sin}. Casting one to the other transposes the rotation --
 *           always convert through this function.
 * @related  cf_b2_to_sincos cf_transform_to_b2 cf_v2_to_b2
 */
CF_INLINE b2Rot cf_sincos_to_b2(CF_SinCos r) { b2Rot out; out.c = r.c; out.s = r.s; return out; }

/**
 * @function cf_b2_to_sincos
 * @category physics
 * @brief    Converts a `b2Rot` to a `CF_SinCos` rotation.
 * @remarks  See `cf_sincos_to_b2` for why this must be a conversion and not a cast.
 * @related  cf_sincos_to_b2 cf_b2_to_transform cf_b2_to_v2
 */
CF_INLINE CF_SinCos cf_b2_to_sincos(b2Rot q) { CF_SinCos out; out.s = q.s; out.c = q.c; return out; }

/**
 * @function cf_transform_to_b2
 * @category physics
 * @brief    Converts a `CF_Transform` to a `b2Transform`.
 * @remarks  Both field order (rotation/position) and rotation layout differ -- always
 *           convert, never cast. The typical direction is the other way: pull a body's
 *           `b2Body_GetTransform` through `cf_b2_to_transform` for rendering.
 * @related  cf_b2_to_transform cf_sincos_to_b2 cf_v2_to_b2
 */
CF_INLINE b2Transform cf_transform_to_b2(CF_Transform tf) { b2Transform out; out.p = cf_v2_to_b2(tf.p); out.q = cf_sincos_to_b2(tf.r); return out; }

/**
 * @function cf_b2_to_transform
 * @category physics
 * @brief    Converts a `b2Transform` to a `CF_Transform`.
 * @related  cf_transform_to_b2 cf_b2_to_sincos cf_b2_to_v2
 */
CF_INLINE CF_Transform cf_b2_to_transform(b2Transform tf) { CF_Transform out; out.p = cf_b2_to_v2(tf.p); out.r = cf_b2_to_sincos(tf.q); return out; }

/**
 * @function cf_circle_to_b2
 * @category physics
 * @brief    Converts a `CF_Circle` to a `b2Circle` (bit-identical).
 * @related  cf_capsule_to_b2 cf_aabb_to_b2 cf_poly_to_b2
 */
CF_INLINE b2Circle cf_circle_to_b2(CF_Circle circle) { b2Circle out; out.center = cf_v2_to_b2(circle.p); out.radius = circle.r; return out; }

/**
 * @function cf_capsule_to_b2
 * @category physics
 * @brief    Converts a `CF_Capsule` to a `b2Capsule` (bit-identical).
 * @related  cf_circle_to_b2 cf_aabb_to_b2 cf_poly_to_b2
 */
CF_INLINE b2Capsule cf_capsule_to_b2(CF_Capsule capsule) { b2Capsule out; out.center1 = cf_v2_to_b2(capsule.a); out.center2 = cf_v2_to_b2(capsule.b); out.radius = capsule.r; return out; }

/**
 * @function cf_aabb_to_b2
 * @category physics
 * @brief    Converts a `CF_Aabb` to a `b2AABB` (bit-identical).
 * @remarks  Box2D has no AABB *shape*; for an AABB-shaped collider use
 *           `b2MakeBox`/`b2MakeOffsetBox`. This converts the bounds type used by queries
 *           like `b2World_OverlapAABB`.
 * @related  cf_b2_to_aabb cf_circle_to_b2 cf_poly_to_b2
 */
CF_INLINE b2AABB cf_aabb_to_b2(CF_Aabb bb) { b2AABB out; out.lowerBound = cf_v2_to_b2(bb.min); out.upperBound = cf_v2_to_b2(bb.max); return out; }

/**
 * @function cf_b2_to_aabb
 * @category physics
 * @brief    Converts a `b2AABB` to a `CF_Aabb`.
 * @related  cf_aabb_to_b2 cf_b2_to_v2
 */
CF_INLINE CF_Aabb cf_b2_to_aabb(b2AABB bb) { return cf_make_aabb(cf_b2_to_v2(bb.lowerBound), cf_b2_to_v2(bb.upperBound)); }

/**
 * @function cf_poly_to_b2
 * @category physics
 * @brief    Converts a `CF_Poly` to a `b2Polygon`.
 * @param    poly    The polygon, with valid verts, norms and count (see `cf_make_poly`).
 * @param    radius  An optional rounding radius for the resulting polygon, usually 0.
 * @remarks  A real conversion, not a cast: `b2Polygon` additionally carries a centroid
 *           and rounding radius, which are computed/assigned here.
 * @related  CF_Poly cf_make_poly cf_circle_to_b2 cf_capsule_to_b2
 */
CF_API b2Polygon CF_CALL cf_poly_to_b2(const CF_Poly* poly, float radius);

//--------------------------------------------------------------------------------------------------
// Debug drawing and stepping.

/**
 * @function cf_physics_debug_draw_defaults
 * @category physics
 * @brief    Returns a `b2DebugDraw` whose callbacks render through CF's 2d draw API.
 * @param    thickness  Stroke width for outlines/segments, in your 2d camera's world units.
 * @remarks  All draw callbacks are wired (polygons, circles, capsules, segments,
 *           transforms, points, strings) and `drawShapes` is enabled; toggle any other
 *           `b2DebugDraw` option before handing it to `b2World_Draw`. Shapes render into
 *           the current 2d camera, layer, and color state like any other `cf_draw_*`
 *           call. For the common case see `cf_physics_draw`.
 * @related  cf_physics_draw cf_physics_step
 */
CF_API b2DebugDraw CF_CALL cf_physics_debug_draw_defaults(float thickness);

/**
 * @function cf_physics_draw
 * @category physics
 * @brief    Draws a Box2D world's shapes and joints through CF's 2d draw API.
 * @param    world      The world to draw.
 * @param    thickness  Stroke width for outlines/segments, in your 2d camera's world units.
 * @remarks  Sugar over `cf_physics_debug_draw_defaults` + `b2World_Draw` with shapes and
 *           joints on. Call it anywhere you'd call other `cf_draw_*` functions.
 * @related  cf_physics_debug_draw_defaults cf_physics_step
 */
CF_API void CF_CALL cf_physics_draw(b2WorldId world, float thickness);

//--------------------------------------------------------------------------------------------------
// 3d: Box3D, exposed the same way Box2D is above. The b3* API mirrors b2's design (id
// handles, b3Default*Def() initializers, polled event buffers), and CF supplies the same
// seam: interop, a draw3d-powered debug draw, and stepping on CF's clock. Unlike the 2d
// rotation types, CF_V3/b3Vec3 and CF_Quat/b3Quat are bit-identical -- no swizzle traps.

/**
 * @function cf_v3_to_b3
 * @category physics
 * @brief    Converts a `CF_V3` to a `b3Vec3` (bit-identical; exists for call-site clarity).
 * @related  cf_b3_to_v3 cf_quat_to_b3 cf_m4_to_b3
 */
CF_INLINE b3Vec3 cf_v3_to_b3(CF_V3 v) { b3Vec3 out; out.x = v.x; out.y = v.y; out.z = v.z; return out; }

/**
 * @function cf_b3_to_v3
 * @category physics
 * @brief    Converts a `b3Vec3` to a `CF_V3`.
 * @related  cf_v3_to_b3 cf_b3_to_quat cf_b3_to_m4
 */
CF_INLINE CF_V3 cf_b3_to_v3(b3Vec3 v) { return cf_v3(v.x, v.y, v.z); }

/**
 * @function cf_quat_to_b3
 * @category physics
 * @brief    Converts a `CF_Quat` to a `b3Quat`.
 * @remarks  The layouts agree -- `b3Quat` stores the vector part first and calls the scalar
 *           `s` where CF calls it `w` -- so this is a plain repack.
 * @related  cf_b3_to_quat cf_v3_to_b3 cf_m4_to_b3
 */
CF_INLINE b3Quat cf_quat_to_b3(CF_Quat q) { b3Quat out; out.v.x = q.x; out.v.y = q.y; out.v.z = q.z; out.s = q.w; return out; }

/**
 * @function cf_b3_to_quat
 * @category physics
 * @brief    Converts a `b3Quat` to a `CF_Quat`.
 * @related  cf_quat_to_b3 cf_b3_to_v3 cf_b3_to_m4
 */
CF_INLINE CF_Quat cf_b3_to_quat(b3Quat q) { return cf_quat(q.v.x, q.v.y, q.v.z, q.s); }

/**
 * @function cf_m4_to_b3
 * @category physics
 * @brief    Converts a rigid `CF_M4x4` transform to a `b3Transform`.
 * @remarks  Physics transforms carry no scale: the matrix's upper 3x3 must be a rotation
 *           (scale is silently normalized away). The typical direction is the other way --
 *           see `cf_b3_to_m4`.
 * @related  cf_b3_to_m4 cf_quat_to_b3 cf_v3_to_b3
 */
CF_INLINE b3Transform cf_m4_to_b3(CF_M4x4 m)
{
	b3Transform out;
	out.p = cf_v3_to_b3(cf_v3(m.elements[12], m.elements[13], m.elements[14]));
	out.q = cf_quat_to_b3(cf_quat_from_m4(m));
	return out;
}

/**
 * @function cf_b3_to_m4
 * @category physics
 * @brief    Converts a `b3Transform` to a `CF_M4x4`, e.g. a body's transform for rendering.
 * @remarks  Pull a body's pose with `b3Body_GetTransform` and hand the result to
 *           `cf_draw3d_transform` (or compose scale on top with `cf_m4_from_trs`).
 * @related  cf_m4_to_b3 cf_b3_to_quat cf_b3_to_v3
 */
CF_INLINE CF_M4x4 cf_b3_to_m4(b3Transform tf) { return cf_m4_from_trs(cf_b3_to_v3(tf.p), cf_b3_to_quat(tf.q), cf_v3(1.0f, 1.0f, 1.0f)); }

/**
 * @function cf_capsule3_to_b3
 * @category physics
 * @brief    Converts a `CF_Capsule3` to a `b3Capsule` (bit-identical).
 * @related  cf_sphere_to_b3 cf_aabb3_to_b3
 */
CF_INLINE b3Capsule cf_capsule3_to_b3(CF_Capsule3 capsule) { b3Capsule out; out.center1 = cf_v3_to_b3(capsule.a); out.center2 = cf_v3_to_b3(capsule.b); out.radius = capsule.r; return out; }

/**
 * @function cf_sphere_to_b3
 * @category physics
 * @brief    Converts a `CF_Sphere` to a `b3Sphere` (bit-identical).
 * @related  cf_capsule3_to_b3 cf_aabb3_to_b3
 */
CF_INLINE b3Sphere cf_sphere_to_b3(CF_Sphere sphere) { b3Sphere out; out.center = cf_v3_to_b3(sphere.p); out.radius = sphere.r; return out; }

/**
 * @function cf_aabb3_to_b3
 * @category physics
 * @brief    Converts a `CF_Aabb3` to a `b3AABB` (bit-identical).
 * @remarks  Box3D has no AABB *shape*; box colliders come from `b3MakeBoxHull`. This
 *           converts the bounds type used by queries like `b3World_OverlapAABB`.
 * @related  cf_b3_to_aabb3 cf_sphere_to_b3 cf_capsule3_to_b3
 */
CF_INLINE b3AABB cf_aabb3_to_b3(CF_Aabb3 bb) { b3AABB out; out.lowerBound = cf_v3_to_b3(bb.min); out.upperBound = cf_v3_to_b3(bb.max); return out; }

/**
 * @function cf_b3_to_aabb3
 * @category physics
 * @brief    Converts a `b3AABB` to a `CF_Aabb3`.
 * @related  cf_aabb3_to_b3 cf_b3_to_v3
 */
CF_INLINE CF_Aabb3 cf_b3_to_aabb3(b3AABB bb) { return cf_make_aabb3(cf_b3_to_v3(bb.lowerBound), cf_b3_to_v3(bb.upperBound)); }

/**
 * @function cf_physics_world_def3
 * @category physics
 * @brief    Returns a `b3WorldDef` wired for CF's debug drawing.
 * @remarks  Same as `b3DefaultWorldDef` plus CF's debug-shape callbacks: Box3D bakes each
 *           shape into a drawable once via world-creation-time callbacks, so a world you
 *           want `cf_physics_draw3` to render shapes for must be created from this def.
 *           Everything else about the def is untouched -- set gravity and friends as usual.
 * @related  cf_physics_draw3 cf_physics_debug_draw3_defaults cf_physics_step3
 */
CF_API b3WorldDef CF_CALL cf_physics_world_def3(void);

/**
 * @function cf_physics_debug_draw3_defaults
 * @category physics
 * @brief    Returns a `b3DebugDraw` whose callbacks render through CF's 3d draw API.
 * @param    thickness  Stroke width for wireframes/segments, in world units.
 * @remarks  Spheres and capsules render as draw3d's built-in solids (hemisphere-lit, no
 *           shader required); hulls and meshes render as wireframes; height fields and
 *           compounds draw their bounds. `drawShapes` is enabled; toggle any other option
 *           before `b3World_Draw`. Create the world from `cf_physics_world_def3` so the
 *           shape bake callbacks are wired.
 * @related  cf_physics_draw3 cf_physics_world_def3 cf_physics_step3
 */
CF_API b3DebugDraw CF_CALL cf_physics_debug_draw3_defaults(float thickness);

/**
 * @function cf_physics_draw3
 * @category physics
 * @brief    Draws a Box3D world's shapes and joints through CF's 3d draw API.
 * @param    world      The world, created from `cf_physics_world_def3`.
 * @param    thickness  Stroke width for wireframes/segments, in world units.
 * @remarks  Sugar over `cf_physics_debug_draw3_defaults` + `b3World_Draw` with shapes and
 *           joints on. Call it under your 3d camera stacks like any `cf_draw3d_*` call.
 * @related  cf_physics_debug_draw3_defaults cf_physics_world_def3 cf_physics_step3
 */
CF_API void CF_CALL cf_physics_draw3(b3WorldId world, float thickness);

/**
 * @function cf_physics_step3
 * @category physics
 * @brief    Steps a Box3D world using CF's clock.
 * @param    world          The world to step.
 * @param    substep_count  Box3D solver sub-steps, typically 4.
 * @remarks  Identical wiring to the 2d `cf_physics_step`: call from your fixed-update
 *           callback with `cf_set_fixed_timestep` enabled; falls back to the variable
 *           frame dt otherwise.
 * @related  cf_physics_step cf_physics_draw3 cf_set_fixed_timestep
 */
CF_API void CF_CALL cf_physics_step3(b3WorldId world, int substep_count);

/**
 * @function cf_physics_step
 * @category physics
 * @brief    Steps a Box2D world using CF's clock.
 * @param    world          The world to step.
 * @param    substep_count  Box2D solver sub-steps, typically 4.
 * @remarks  Physics wants a fixed timestep. The intended wiring is:
 *
 *           ```c
 *           cf_set_fixed_timestep(60);
 *           // Each frame:
 *           cf_app_update(on_fixed_update); // CF calls it once per fixed tick.
 *           // Inside on_fixed_update:
 *           cf_physics_step(world, 4);
 *           ```
 *
 *           This steps by `CF_DELTA_TIME_FIXED`, so gameplay and physics share one clock
 *           (and Box2D v3's determinism actually holds). Without fixed timestep enabled
 *           it falls back to the variable `CF_DELTA_TIME`, which works but wobbles.
 *           When rendering between fixed ticks, interpolate transforms with
 *           `CF_DELTA_TIME_INTERPOLANT`.
 * @related  cf_physics_draw cf_set_fixed_timestep CF_DELTA_TIME_FIXED CF_DELTA_TIME_INTERPOLANT
 */
CF_API void CF_CALL cf_physics_step(b2WorldId world, int substep_count);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

CF_INLINE b2Vec2 to_b2(CF_V2 v) { return cf_v2_to_b2(v); }
CF_INLINE b2Rot to_b2(CF_SinCos r) { return cf_sincos_to_b2(r); }
CF_INLINE b2Transform to_b2(CF_Transform tf) { return cf_transform_to_b2(tf); }
CF_INLINE b2Circle to_b2(CF_Circle circle) { return cf_circle_to_b2(circle); }
CF_INLINE b2Capsule to_b2(CF_Capsule capsule) { return cf_capsule_to_b2(capsule); }
CF_INLINE b2AABB to_b2(CF_Aabb bb) { return cf_aabb_to_b2(bb); }
CF_INLINE b2Polygon to_b2(const CF_Poly& poly, float radius = 0) { return cf_poly_to_b2(&poly, radius); }
CF_INLINE CF_V2 from_b2(b2Vec2 v) { return cf_b2_to_v2(v); }
CF_INLINE CF_SinCos from_b2(b2Rot q) { return cf_b2_to_sincos(q); }
CF_INLINE CF_Transform from_b2(b2Transform tf) { return cf_b2_to_transform(tf); }
CF_INLINE CF_Aabb from_b2(b2AABB bb) { return cf_b2_to_aabb(bb); }

CF_INLINE b2DebugDraw physics_debug_draw_defaults(float thickness) { return cf_physics_debug_draw_defaults(thickness); }
CF_INLINE void physics_draw(b2WorldId world, float thickness) { cf_physics_draw(world, thickness); }
CF_INLINE void physics_step(b2WorldId world, int substep_count) { cf_physics_step(world, substep_count); }

CF_INLINE b3Vec3 to_b3(CF_V3 v) { return cf_v3_to_b3(v); }
CF_INLINE b3Quat to_b3(CF_Quat q) { return cf_quat_to_b3(q); }
CF_INLINE b3Transform to_b3(CF_M4x4 m) { return cf_m4_to_b3(m); }
CF_INLINE b3Sphere to_b3(CF_Sphere sphere) { return cf_sphere_to_b3(sphere); }
CF_INLINE b3Capsule to_b3(CF_Capsule3 capsule) { return cf_capsule3_to_b3(capsule); }
CF_INLINE b3AABB to_b3(CF_Aabb3 bb) { return cf_aabb3_to_b3(bb); }
CF_INLINE CF_V3 from_b3(b3Vec3 v) { return cf_b3_to_v3(v); }
CF_INLINE CF_Quat from_b3(b3Quat q) { return cf_b3_to_quat(q); }
CF_INLINE CF_M4x4 from_b3(b3Transform tf) { return cf_b3_to_m4(tf); }
CF_INLINE CF_Aabb3 from_b3(b3AABB bb) { return cf_b3_to_aabb3(bb); }

CF_INLINE b3WorldDef physics_world_def3() { return cf_physics_world_def3(); }
CF_INLINE b3DebugDraw physics_debug_draw3_defaults(float thickness) { return cf_physics_debug_draw3_defaults(thickness); }
CF_INLINE void physics_draw3(b3WorldId world, float thickness) { cf_physics_draw3(world, thickness); }
CF_INLINE void physics_step3(b3WorldId world, int substep_count) { cf_physics_step3(world, substep_count); }

}

#endif // CF_CPP

#endif // CF_PHYSICS_H
