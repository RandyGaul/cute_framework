/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_PHYSICS_H
#define CF_PHYSICS_H

#include "cute_defines.h"
#include "cute_math.h"
#include "cute_time.h"

#include <box2d/box2d.h>

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

}

#endif // CF_CPP

#endif // CF_PHYSICS_H
