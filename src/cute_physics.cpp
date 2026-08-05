/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_physics.h>
#include <cute_draw.h>
#include <cute_color.h>
#include <cute_alloc.h>
#include <cute_time.h>
#include <cute_c_runtime.h>

#include <stdio.h>

//--------------------------------------------------------------------------------------------------
// Allocator + assert wiring, called once from cf_make_app so every Box2D allocation flows
// through CF's allocator (Box2D asks for aligned memory for its SIMD solver data).

static void* s_b2_alloc(unsigned int size, int alignment)
{
	return cf_aligned_alloc((size_t)size, alignment);
}

static void s_b2_free(void* mem)
{
	cf_aligned_free(mem);
}

static int s_b2_assert(const char* condition, const char* file_name, int line_number)
{
	fprintf(stderr, "BOX2D ASSERT: %s, %s, line %d\n", condition, file_name, line_number);
	return 1; // Break in the debugger.
}

void cf_physics_wire_allocator()
{
	b2SetAllocator(s_b2_alloc, s_b2_free);
	b2SetAssertFcn(s_b2_assert);
}

//--------------------------------------------------------------------------------------------------
// Interop.

b2Polygon cf_poly_to_b2(const CF_Poly* poly, float radius)
{
	b2Polygon out = { };
	out.count = poly->count;
	for (int i = 0; i < poly->count; ++i) {
		out.vertices[i] = cf_v2_to_b2(poly->verts[i]);
		out.normals[i] = cf_v2_to_b2(poly->norms[i]);
	}
	out.centroid = cf_v2_to_b2(cf_centroid(poly->verts, poly->count));
	out.radius = radius;
	return out;
}

//--------------------------------------------------------------------------------------------------
// Debug draw: b2DebugDraw callbacks routed into the 2d draw API. Everything lands in the
// current 2d camera, layer and blend state like any other cf_draw_* call.

static float s_thickness = 0.1f;

static CF_Color s_color(b2HexColor hex, float scale)
{
	float r = (float)((hex >> 16) & 0xFF) * (1.0f / 255.0f);
	float g = (float)((hex >> 8) & 0xFF) * (1.0f / 255.0f);
	float b = (float)(hex & 0xFF) * (1.0f / 255.0f);
	return cf_make_color_rgb_f(r * scale, g * scale, b * scale);
}

static void s_draw_polygon(const b2Vec2* vertices, int count, b2HexColor color, void* context)
{
	CF_UNUSED(context);
	cf_draw_push_color(s_color(color, 1.0f));
	cf_draw_polyline((const CF_V2*)vertices, count, s_thickness, true);
	cf_draw_pop_color();
}

static void s_draw_solid_polygon(b2Transform xf, const b2Vec2* vertices, int count, float radius, b2HexColor color, void* context)
{
	CF_UNUSED(context);
	CF_V2 points[B2_MAX_POLYGON_VERTICES];
	for (int i = 0; i < count; ++i) points[i] = cf_b2_to_v2(b2TransformPoint(xf, vertices[i]));
	cf_draw_push_color(s_color(color, 1.0f));
	// Box2D's rounding radius maps straight onto the draw API's chubbiness.
	cf_draw_polygon_fill(points, count, radius);
	cf_draw_pop_color();
}

static void s_draw_circle(b2Vec2 center, float radius, b2HexColor color, void* context)
{
	CF_UNUSED(context);
	cf_draw_push_color(s_color(color, 1.0f));
	cf_draw_circle2(cf_b2_to_v2(center), radius, s_thickness);
	cf_draw_pop_color();
}

static void s_draw_solid_circle(b2Transform xf, float radius, b2HexColor color, void* context)
{
	CF_UNUSED(context);
	CF_V2 center = cf_b2_to_v2(xf.p);
	cf_draw_push_color(s_color(color, 1.0f));
	cf_draw_circle_fill2(center, radius);
	cf_draw_pop_color();
	// A radius line so rotation reads on screen.
	b2Vec2 axis_tip;
	axis_tip.x = radius;
	axis_tip.y = 0;
	cf_draw_push_color(s_color(color, 0.6f));
	cf_draw_line(center, cf_b2_to_v2(b2TransformPoint(xf, axis_tip)), s_thickness);
	cf_draw_pop_color();
}

static void s_draw_solid_capsule(b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void* context)
{
	CF_UNUSED(context);
	cf_draw_push_color(s_color(color, 1.0f));
	cf_draw_capsule_fill2(cf_b2_to_v2(p1), cf_b2_to_v2(p2), radius);
	cf_draw_pop_color();
}

static void s_draw_segment(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context)
{
	CF_UNUSED(context);
	cf_draw_push_color(s_color(color, 1.0f));
	cf_draw_line(cf_b2_to_v2(p1), cf_b2_to_v2(p2), s_thickness);
	cf_draw_pop_color();
}

static void s_draw_transform(b2Transform xf, void* context)
{
	CF_UNUSED(context);
	float scale = s_thickness * 8.0f;
	CF_V2 p = cf_b2_to_v2(xf.p);
	b2Vec2 x_axis; x_axis.x = scale; x_axis.y = 0;
	b2Vec2 y_axis; y_axis.x = 0; y_axis.y = scale;
	cf_draw_push_color(cf_make_color_rgb_f(0.9f, 0.2f, 0.2f));
	cf_draw_line(p, cf_b2_to_v2(b2TransformPoint(xf, x_axis)), s_thickness);
	cf_draw_pop_color();
	cf_draw_push_color(cf_make_color_rgb_f(0.2f, 0.9f, 0.2f));
	cf_draw_line(p, cf_b2_to_v2(b2TransformPoint(xf, y_axis)), s_thickness);
	cf_draw_pop_color();
}

static void s_draw_point(b2Vec2 p, float size, b2HexColor color, void* context)
{
	CF_UNUSED(context);
	cf_draw_push_color(s_color(color, 1.0f));
	cf_draw_circle_fill2(cf_b2_to_v2(p), size * s_thickness * 0.5f);
	cf_draw_pop_color();
}

static void s_draw_string(b2Vec2 p, const char* s, b2HexColor color, void* context)
{
	CF_UNUSED(context);
	cf_draw_push_color(s_color(color, 1.0f));
	cf_draw_text(s, cf_b2_to_v2(p), -1);
	cf_draw_pop_color();
}

b2DebugDraw cf_physics_debug_draw_defaults(float thickness)
{
	s_thickness = thickness;
	b2DebugDraw draw = b2DefaultDebugDraw();
	draw.DrawPolygonFcn = s_draw_polygon;
	draw.DrawSolidPolygonFcn = s_draw_solid_polygon;
	draw.DrawCircleFcn = s_draw_circle;
	draw.DrawSolidCircleFcn = s_draw_solid_circle;
	draw.DrawSolidCapsuleFcn = s_draw_solid_capsule;
	draw.DrawSegmentFcn = s_draw_segment;
	draw.DrawTransformFcn = s_draw_transform;
	draw.DrawPointFcn = s_draw_point;
	draw.DrawStringFcn = s_draw_string;
	draw.drawShapes = true;
	return draw;
}

void cf_physics_draw(b2WorldId world, float thickness)
{
	b2DebugDraw draw = cf_physics_debug_draw_defaults(thickness);
	draw.drawJoints = true;
	b2World_Draw(world, &draw);
}

//--------------------------------------------------------------------------------------------------
// Stepping.

void cf_physics_step(b2WorldId world, int substep_count)
{
	// One clock for gameplay and physics: the fixed timestep when enabled (which is what
	// physics wants -- see the header remarks), the variable frame dt otherwise.
	float dt = CF_DELTA_TIME_FIXED > 0 ? CF_DELTA_TIME_FIXED : CF_DELTA_TIME;
	b2World_Step(world, dt, substep_count);
}
