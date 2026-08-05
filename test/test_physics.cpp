/*
    Cute Framework
    Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>

// Smoke tests for the vendored physics engines: worlds create, bodies fall under gravity,
// and the interop converters round-trip. Pure CPU -- no app or GPU (the allocators wire
// lazily here since cf_make_app may not have run).

TEST_CASE(test_physics_world_2d)
{
	b2WorldDef def = b2DefaultWorldDef();
	def.gravity.y = -10.0f;
	b2WorldId world = b2CreateWorld(&def);

	// Static ground + a falling dynamic box.
	b2BodyDef ground_def = b2DefaultBodyDef();
	ground_def.position.y = -5.0f;
	b2BodyId ground = b2CreateBody(world, &ground_def);
	b2ShapeDef ground_shape = b2DefaultShapeDef();
	b2Polygon ground_box = b2MakeBox(10.0f, 0.5f);
	b2CreatePolygonShape(ground, &ground_shape, &ground_box);

	b2BodyDef body_def = b2DefaultBodyDef();
	body_def.type = b2_dynamicBody;
	body_def.position.y = 5.0f;
	b2BodyId body = b2CreateBody(world, &body_def);
	b2ShapeDef shape_def = b2DefaultShapeDef();
	b2Polygon box = b2MakeBox(0.5f, 0.5f);
	b2CreatePolygonShape(body, &shape_def, &box);

	// Fall for two simulated seconds; the box must come to rest on the ground
	// (top of ground at y = -4.5, so the box center settles near y = -4).
	for (int i = 0; i < 120; ++i) b2World_Step(world, 1.0f / 60.0f, 4);
	b2Vec2 p = b2Body_GetPosition(body);
	REQUIRE(cf_abs(p.y + 4.0f) < 0.1f);

	b2DestroyWorld(world);
	return true;
}

TEST_CASE(test_physics_world_3d)
{
	// cf_physics_world_def3 wires CF's debug-shape bake callbacks; creating shapes and
	// stepping must work without any drawing ever happening.
	b3WorldDef def = cf_physics_world_def3();
	def.gravity.y = -10.0f;
	b3WorldId world = b3CreateWorld(&def);

	b3BodyDef ground_def = b3DefaultBodyDef();
	ground_def.position.y = -5.0f;
	b3BodyId ground = b3CreateBody(world, &ground_def);
	b3ShapeDef ground_shape = b3DefaultShapeDef();
	b3BoxHull ground_box = b3MakeBoxHull(10.0f, 0.5f, 10.0f);
	b3CreateHullShape(ground, &ground_shape, &ground_box.base);

	b3BodyDef body_def = b3DefaultBodyDef();
	body_def.type = b3_dynamicBody;
	body_def.position.y = 5.0f;
	b3BodyId body = b3CreateBody(world, &body_def);
	b3ShapeDef shape_def = b3DefaultShapeDef();
	b3Sphere sphere = { { 0, 0, 0 }, 0.5f };
	b3CreateSphereShape(body, &shape_def, &sphere);

	for (int i = 0; i < 120; ++i) b3World_Step(world, 1.0f / 60.0f, 4);
	b3Pos p = b3Body_GetPosition(body);
	REQUIRE(cf_abs(p.y + 4.0f) < 0.1f);

	b3DestroyWorld(world);
	return true;
}

TEST_CASE(test_physics_interop)
{
	// 2d: the rotation swizzle must transpose fields, not copy them.
	CF_SinCos sc = cf_sincos(0.5f);
	b2Rot rot = cf_sincos_to_b2(sc);
	REQUIRE(rot.c == sc.c && rot.s == sc.s);
	CF_SinCos back = cf_b2_to_sincos(rot);
	REQUIRE(back.s == sc.s && back.c == sc.c);

	// 3d: quaternions are bit-identical repacks, and transforms round-trip through m4.
	CF_Quat q = cf_quat_from_axis_angle(cf_norm_v3(cf_v3(1, 2, 3)), 0.7f);
	b3Quat bq = cf_quat_to_b3(q);
	REQUIRE(bq.v.x == q.x && bq.v.y == q.y && bq.v.z == q.z && bq.s == q.w);
	CF_Quat qb = cf_b3_to_quat(bq);
	REQUIRE(qb.x == q.x && qb.w == q.w);

	b3Transform tf;
	tf.p = cf_v3_to_b3(cf_v3(3, -4, 5));
	tf.q = bq;
	CF_M4x4 m = cf_b3_to_m4(tf);
	b3Transform tf2 = cf_m4_to_b3(m);
	REQUIRE(cf_abs(tf2.p.x - 3.0f) < 1e-4f && cf_abs(tf2.p.y + 4.0f) < 1e-4f);
	// Quaternions double-cover; compare action on a vector instead of components.
	CF_V3 v = cf_v3(0.3f, -0.7f, 1.1f);
	CF_V3 via_q = cf_mul_q_v3(q, v);
	CF_V3 via_q2 = cf_mul_q_v3(cf_b3_to_quat(tf2.q), v);
	REQUIRE(cf_abs(via_q.x - via_q2.x) < 1e-4f);
	REQUIRE(cf_abs(via_q.y - via_q2.y) < 1e-4f);
	REQUIRE(cf_abs(via_q.z - via_q2.z) < 1e-4f);
	return true;
}

TEST_SUITE(test_physics)
{
	RUN_TEST_CASE(test_physics_world_2d);
	RUN_TEST_CASE(test_physics_world_3d);
	RUN_TEST_CASE(test_physics_interop);
}
