# Physics

CF's physics is [Box2D v3](https://box2d.org) in 2d and [Box3D](https://github.com/erincatto/box3d) in 3d, exposed directly. The libraries (Box2D v3.1.1, Box3D v0.1.0, both MIT) are pinned and fetched by CF's CMake, compiled into `cute.lib`, and wired to CF's allocator at app startup -- one `#include <cute.h>` and the entire `b2*` API is yours: worlds, bodies, shapes, joints, contact events, and world queries. There is deliberately no `CF_Body` wrapper layer. Box2D v3 is already a C API of id-handles and `b2Default*Def()` initializers -- the same idiom as CF's own `cf_*_defaults()` -- and [Box2D's documentation](https://box2d.org/documentation/) teaches it better than any renamed veneer could.

What CF adds is the seam, in [cute_physics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_physics.h):

- **Interop converters** between CF and Box2D math/shape types.
- **A debug-draw bridge** rendering a world through CF's 2d draw API.
- **A stepping helper** tying `b2World_Step` to CF's fixed-timestep clock.

This is distinct from the [Collision](collision.md) topic: the stateless queries there (`cf_circle_to_poly`, manifolds, raycasts, `cf_gjk`, `cf_toi`) answer immediate shape-vs-shape questions with no world involved -- they also run on Box2D's geometry layer internally. This page is about simulation.

## Quick Start

```c
#include <cute.h>

static b2WorldId world;

void fixed_update(void* udata)
{
	cf_physics_step(world, 4); // Steps by CF_DELTA_TIME_FIXED, 4 solver sub-steps.
}

int main(int argc, char* argv[])
{
	cf_make_app("Physics", 0, 0, 0, 1280, 720, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	cf_set_fixed_timestep(60);

	b2WorldDef world_def = b2DefaultWorldDef();
	world_def.gravity.y = -10.0f;
	world = b2CreateWorld(&world_def);

	b2BodyDef body_def = b2DefaultBodyDef();
	body_def.type = b2_dynamicBody;
	body_def.position.y = 5.0f;
	b2BodyId body = b2CreateBody(world, &body_def);
	b2ShapeDef shape_def = b2DefaultShapeDef();
	b2Polygon box = b2MakeBox(0.5f, 0.5f);
	b2CreatePolygonShape(body, &shape_def, &box);

	while (cf_app_is_running()) {
		cf_app_update(fixed_update);
		cf_draw_push();
		cf_draw_scale(32.0f, 32.0f);  // 32 pixels per meter.
		cf_physics_draw(world, 0.04f); // Anti-aliased debug draw via cf_draw_*.
		cf_draw_pop();
		cf_app_draw_onto_screen(true);
	}
	b2DestroyWorld(world);
	cf_destroy_app();
	return 0;
}
```

The [physics sample](https://github.com/RandyGaul/cute_framework/blob/master/samples/physics.c) is this pattern with a box pyramid, explosions (`b2World_Explode`), and click-to-spawn.

## Stepping and Time

Physics wants a fixed timestep -- Box2D v3 is deterministic, but only if you feed it identical dt values. The wiring is `cf_set_fixed_timestep` plus a `CF_OnUpdateFn` handed to `cf_app_update`, with `cf_physics_step` inside; CF then calls it exactly once per fixed tick regardless of render framerate. When rendering between ticks, interpolate body transforms with `CF_DELTA_TIME_INTERPOLANT` (pull them via `b2Body_GetTransform` through `cf_b2_to_transform`, or batch with `b2World_GetBodyEvents`).

Work in meters, not pixels. Box2D's solver is tuned for objects in roughly the 0.1 to 50 range -- scale your *camera* (e.g. `cf_draw_scale(32.0f, 32.0f)` for 32 px/m), never your physics.

## Interop and the One Trap

`CF_V2` and `b2Vec2` are bit-identical, as are `CF_Aabb`/`b2AABB`, `CF_Circle`/`b2Circle`, and `CF_Capsule`/`b2Capsule` -- the converters (`cf_v2_to_b2` and friends) exist for call-site clarity. Two types are **not** cast-safe:

- `CF_SinCos` stores `{sin, cos}`; `b2Rot` stores `{cos, sin}`. A blind cast transposes the rotation. Always go through `cf_sincos_to_b2` / `cf_b2_to_sincos`.
- `CF_Transform` and `b2Transform` differ in both field order and rotation layout -- `cf_transform_to_b2` / `cf_b2_to_transform`.

Polygons are a real conversion (`cf_poly_to_b2` computes the centroid Box2D wants), and Box2D has no AABB *shape* -- use `b2MakeBox` for box colliders.

## Events, Not Callbacks

Box2D v3 replaced collision listeners with per-step event buffers -- poll after stepping:

```c
b2ContactEvents events = b2World_GetContactEvents(world);
for (int i = 0; i < events.beginCount; ++i) {
	b2ContactBeginTouchEvent* e = events.beginEvents + i;
	// e->shapeIdA, e->shapeIdB...
}
```

Note contact events are opt-in per shape (`b2ShapeDef.enableContactEvents`), as are sensor and hit events. See Box2D's [world](https://box2d.org/documentation/group__world.html) and [events documentation](https://box2d.org/documentation/md_simulation.html) for the full menu: sensors, hit events with approach speeds, joint break thresholds, `b2World_CastRay`, overlap queries, and the character mover (`b2World_CastMover` / `b2SolvePlanes`).

## 3D: Box3D

Everything above has a 3d twin. CF pins [Box3D](https://github.com/erincatto/box3d) (v0.1.0, MIT) -- Erin Catto's 3d engine, which deliberately mirrors Box2D v3's design: `b3*` id handles, `b3Default*Def()` initializers, polled event buffers, joints, and world queries. The same include exposes it all, and the seam repeats:

- `cf_physics_step3(world, substeps)` -- the same fixed-clock wiring as 2d.
- `cf_physics_draw3(world, thickness)` -- debug draw through draw3d's shader-free built-ins: spheres, capsules, and box-shaped hulls as hemisphere-lit solids, general hulls and meshes as anti-aliased wireframes. One requirement: Box3D bakes shapes into drawables via world-creation callbacks, so **create your world from `cf_physics_world_def3()`** for shape drawing to work.
- Interop is friendlier than 2d: `CF_V3`/`b3Vec3` and `CF_Quat`/`b3Quat` are bit-identical (no swizzle trap), and `cf_b3_to_m4` turns a body's `b3Body_GetTransform` straight into a `cf_draw3d_transform`-ready matrix.

The [physics3d sample](https://github.com/RandyGaul/cute_framework/blob/master/samples/physics3d.c) is the whole pattern: a box tower on a ground slab, orbit camera, camera-ray spawning, explosions.

Box3D also powers the new **3d stateless collision kit** in cute_math3d.h -- `CF_Capsule3`, `CF_Triangle3`, pairwise manifolds (including shapes-vs-triangle for colliding against your own level geometry), `cf_ray3_to_capsule3`/`cf_ray3_to_triangle3`, and the generic `cf_gjk3`/`cf_toi3`/`cf_collide3` family mirroring the 2d kit. And the freestanding character-mover solver (`b3SolvePlanes`, `b3ClipVector`, `b3World_CastMover`) is available directly for move-and-slide controllers.

One caveat worth knowing: Box3D is younger than Box2D (its author calls v0.1 alpha), so expect API movement upstream; CF pins the fetched release and will bump deliberately.

## Build Notes

- Box2D compiles into CF itself -- nothing extra to link, and `b2*` symbols export from shared builds. CMake fetches the pinned release with a shallow clone on first configure.
- SIMD: SSE2/NEON everywhere, wasm SIMD on web; AVX2 stays off for compatibility.
- Allocations route through `cf_aligned_alloc` (wired at `cf_make_app`).
- Pinned at v3.1.1 (fetched shallow at configure time); the upstream `main` branch has breaking API changes queued for 3.2, so consult v3.1 documentation.
