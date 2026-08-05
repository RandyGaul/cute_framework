/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_physics.h>
#include <cute_draw.h>
#include <cute_draw3d.h>
#include <cute_color.h>
#include <cute_alloc.h>
#include <cute_time.h>
#include <cute_c_runtime.h>

#include <box3d/collision.h>
#include <box3d/math_functions.h>

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

// Box3D spells the same allocator contract with int32 sizes.
static void* s_b3_alloc(int32_t size, int32_t alignment)
{
	return cf_aligned_alloc((size_t)size, alignment);
}

static int s_b3_assert(const char* condition, const char* file_name, int line_number)
{
	fprintf(stderr, "BOX3D ASSERT: %s, %s, line %d\n", condition, file_name, line_number);
	return 1; // Break in the debugger.
}

void cf_physics_wire_allocator()
{
	b2SetAllocator(s_b2_alloc, s_b2_free);
	b2SetAssertFcn(s_b2_assert);
	b3SetAllocator(s_b3_alloc, s_b2_free);
	b3SetAssertFcn(s_b3_assert);
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

void cf_physics_step3(b3WorldId world, int substep_count)
{
	float dt = CF_DELTA_TIME_FIXED > 0 ? CF_DELTA_TIME_FIXED : CF_DELTA_TIME;
	b3World_Step(world, dt, substep_count);
}

//--------------------------------------------------------------------------------------------------
// 3d debug draw: b3DebugDraw callbacks routed into the draw3d API. Box3D bakes each shape
// into a user drawable once (via world-creation callbacks), so spheres and capsules store
// their primitive data and everything else bakes into a wireframe segment list.

static float s_thickness3 = 0.02f;

static CF_Color s_color3(b3HexColor hex, float scale, float alpha)
{
	float r = (float)((hex >> 16) & 0xFF) * (1.0f / 255.0f);
	float g = (float)((hex >> 8) & 0xFF) * (1.0f / 255.0f);
	float b = (float)(hex & 0xFF) * (1.0f / 255.0f);
	return cf_make_color_rgba_f(r * scale, g * scale, b * scale, alpha);
}

struct CF_B3DebugShape
{
	b3ShapeType type;
	b3Sphere sphere;
	b3Capsule capsule;
	CF_V3* segments; // Wireframe segment pairs for hulls/meshes/bounds; NULL for primitives.
	int segment_vert_count;
};

static void s_add_segment(CF_V3* segments, int* count, CF_V3 a, CF_V3 b)
{
	segments[(*count)++] = a;
	segments[(*count)++] = b;
}

static CF_V3* s_wire_box(b3AABB bb, int* out_count)
{
	CF_V3* segments = (CF_V3*)cf_alloc(sizeof(CF_V3) * 24);
	CF_V3 lo = cf_b3_to_v3(bb.lowerBound);
	CF_V3 hi = cf_b3_to_v3(bb.upperBound);
	CF_V3 c[8] = {
		cf_v3(lo.x, lo.y, lo.z), cf_v3(hi.x, lo.y, lo.z), cf_v3(hi.x, hi.y, lo.z), cf_v3(lo.x, hi.y, lo.z),
		cf_v3(lo.x, lo.y, hi.z), cf_v3(hi.x, lo.y, hi.z), cf_v3(hi.x, hi.y, hi.z), cf_v3(lo.x, hi.y, hi.z),
	};
	int n = 0;
	for (int i = 0; i < 4; ++i) {
		int j = (i + 1) & 3;
		s_add_segment(segments, &n, c[i], c[j]);         // Bottom ring.
		s_add_segment(segments, &n, c[i + 4], c[j + 4]); // Top ring.
		s_add_segment(segments, &n, c[i], c[i + 4]);     // Verticals.
	}
	*out_count = n;
	return segments;
}

static void* s_create_debug_shape(const b3DebugShape* debug_shape, void* udata)
{
	CF_UNUSED(udata);
	CF_B3DebugShape* shape = (CF_B3DebugShape*)cf_calloc(sizeof(CF_B3DebugShape), 1);
	shape->type = debug_shape->type;
	switch (debug_shape->type) {
	case b3_sphereShape:
		shape->sphere = *debug_shape->sphere;
		break;

	case b3_capsuleShape:
		shape->capsule = *debug_shape->capsule;
		break;

	case b3_hullShape:
	{
		// Emit each hull edge once: a half-edge and its twin describe the same segment,
		// so keep only the half of each pair with the smaller index.
		const b3HullData* hull = debug_shape->hull;
		const b3Vec3* points = b3GetHullPoints(hull);
		const b3HullHalfEdge* edges = b3GetHullEdges(hull);
		shape->segments = (CF_V3*)cf_alloc(sizeof(CF_V3) * (size_t)hull->edgeCount);
		int n = 0;
		for (int i = 0; i < hull->edgeCount; ++i) {
			if (i < edges[i].twin) {
				s_add_segment(shape->segments, &n, cf_b3_to_v3(points[edges[i].origin]), cf_b3_to_v3(points[edges[edges[i].twin].origin]));
			}
		}
		shape->segment_vert_count = n;
	}	break;

	case b3_meshShape:
	{
		const b3Mesh* mesh = debug_shape->mesh;
		const b3Vec3* verts = b3GetMeshVertices(mesh->data);
		const b3MeshTriangle* tris = b3GetMeshTriangles(mesh->data);
		CF_V3 scale = cf_b3_to_v3(mesh->scale);
		shape->segments = (CF_V3*)cf_alloc(sizeof(CF_V3) * 6 * (size_t)mesh->data->triangleCount);
		int n = 0;
		for (int i = 0; i < mesh->data->triangleCount; ++i) {
			CF_V3 a = cf_mul_v3(cf_b3_to_v3(verts[tris[i].index1]), scale);
			CF_V3 b = cf_mul_v3(cf_b3_to_v3(verts[tris[i].index2]), scale);
			CF_V3 c = cf_mul_v3(cf_b3_to_v3(verts[tris[i].index3]), scale);
			s_add_segment(shape->segments, &n, a, b);
			s_add_segment(shape->segments, &n, b, c);
			s_add_segment(shape->segments, &n, c, a);
		}
		shape->segment_vert_count = n;
	}	break;

	case b3_heightShape:
		shape->segments = s_wire_box(b3ComputeHeightFieldAABB(debug_shape->heightField, b3Transform_identity), &shape->segment_vert_count);
		break;

	case b3_compoundShape:
		shape->segments = s_wire_box(b3ComputeCompoundAABB(debug_shape->compound, b3Transform_identity), &shape->segment_vert_count);
		break;

	default: break;
	}
	return shape;
}

static void s_destroy_debug_shape(void* user_shape, void* udata)
{
	CF_UNUSED(udata);
	CF_B3DebugShape* shape = (CF_B3DebugShape*)user_shape;
	if (shape) {
		cf_free(shape->segments);
		cf_free(shape);
	}
}

static bool s_draw3_shape(void* user_shape, b3WorldTransform transform, b3HexColor color, void* context)
{
	CF_UNUSED(context);
	CF_B3DebugShape* shape = (CF_B3DebugShape*)user_shape;
	if (!shape) return true;
	cf_draw3d_push();
	cf_draw3d_translate(cf_b3_to_v3(transform.p));
	cf_draw3d_rotate(cf_b3_to_quat(transform.q));
	cf_draw3d_push_color(s_color3(color, 1.0f, 1.0f));
	switch (shape->type) {
	case b3_sphereShape:
		cf_draw3d_sphere(cf_b3_to_v3(shape->sphere.center), shape->sphere.radius);
		break;

	case b3_capsuleShape:
		cf_draw3d_capsule(cf_b3_to_v3(shape->capsule.center1), cf_b3_to_v3(shape->capsule.center2), shape->capsule.radius);
		break;

	default:
		for (int i = 0; i < shape->segment_vert_count; i += 2) {
			cf_draw3d_line(shape->segments[i], shape->segments[i + 1], s_thickness3);
		}
		break;
	}
	cf_draw3d_pop_color();
	cf_draw3d_pop();
	return true;
}

static void s_draw3_segment(b3Pos p1, b3Pos p2, b3HexColor color, void* context)
{
	CF_UNUSED(context);
	cf_draw3d_push_color(s_color3(color, 1.0f, 1.0f));
	cf_draw3d_line(cf_b3_to_v3(p1), cf_b3_to_v3(p2), s_thickness3);
	cf_draw3d_pop_color();
}

static void s_draw3_transform(b3WorldTransform transform, void* context)
{
	CF_UNUSED(context);
	cf_draw3d_push();
	cf_draw3d_translate(cf_b3_to_v3(transform.p));
	cf_draw3d_rotate(cf_b3_to_quat(transform.q));
	cf_draw3d_axes(s_thickness3 * 8.0f, s_thickness3);
	cf_draw3d_pop();
}

static void s_draw3_point(b3Pos p, float size, b3HexColor color, void* context)
{
	CF_UNUSED(context);
	cf_draw3d_push_color(s_color3(color, 1.0f, 1.0f));
	cf_draw3d_sphere(cf_b3_to_v3(p), size * s_thickness3 * 0.5f);
	cf_draw3d_pop_color();
}

static void s_draw3_sphere(b3Pos p, float radius, b3HexColor color, float alpha, void* context)
{
	CF_UNUSED(context);
	cf_draw3d_push_color(s_color3(color, 1.0f, alpha));
	cf_draw3d_sphere(cf_b3_to_v3(p), radius);
	cf_draw3d_pop_color();
}

static void s_draw3_capsule(b3Pos p1, b3Pos p2, float radius, b3HexColor color, float alpha, void* context)
{
	CF_UNUSED(context);
	cf_draw3d_push_color(s_color3(color, 1.0f, alpha));
	cf_draw3d_capsule(cf_b3_to_v3(p1), cf_b3_to_v3(p2), radius);
	cf_draw3d_pop_color();
}

static void s_draw3_bounds(b3AABB aabb, b3HexColor color, void* context)
{
	CF_UNUSED(context);
	CF_Aabb3 bb = cf_b3_to_aabb3(aabb);
	cf_draw3d_push_color(s_color3(color, 1.0f, 1.0f));
	cf_draw3d_box_wire(cf_center_aabb3(bb), cf_half_extents_aabb3(bb), s_thickness3);
	cf_draw3d_pop_color();
}

static void s_draw3_box(b3Vec3 extents, b3WorldTransform transform, b3HexColor color, void* context)
{
	CF_UNUSED(context);
	cf_draw3d_push();
	cf_draw3d_translate(cf_b3_to_v3(transform.p));
	cf_draw3d_rotate(cf_b3_to_quat(transform.q));
	cf_draw3d_push_color(s_color3(color, 1.0f, 1.0f));
	cf_draw3d_cube(cf_v3(0.0f), cf_b3_to_v3(extents));
	cf_draw3d_pop_color();
	cf_draw3d_pop();
}

static void s_draw3_string(b3Pos p, const char* s, b3HexColor color, void* context)
{
	CF_UNUSED(context);
	// Project into the 2d draw API's world space under the current 3d camera stacks.
	CF_V3 projected = cf_draw3d_project(cf_b3_to_v3(p));
	if (projected.z < 0) return;
	cf_draw_push_color(s_color3(color, 1.0f, 1.0f));
	cf_draw_text(s, cf_v2(projected.x, projected.y), -1);
	cf_draw_pop_color();
}

b3WorldDef cf_physics_world_def3(void)
{
	b3WorldDef def = b3DefaultWorldDef();
	def.createDebugShape = s_create_debug_shape;
	def.destroyDebugShape = s_destroy_debug_shape;
	return def;
}

b3DebugDraw cf_physics_debug_draw3_defaults(float thickness)
{
	s_thickness3 = thickness;
	b3DebugDraw draw = b3DefaultDebugDraw();
	draw.DrawShapeFcn = s_draw3_shape;
	draw.DrawSegmentFcn = s_draw3_segment;
	draw.DrawTransformFcn = s_draw3_transform;
	draw.DrawPointFcn = s_draw3_point;
	draw.DrawSphereFcn = s_draw3_sphere;
	draw.DrawCapsuleFcn = s_draw3_capsule;
	draw.DrawBoundsFcn = s_draw3_bounds;
	draw.DrawBoxFcn = s_draw3_box;
	draw.DrawStringFcn = s_draw3_string;
	draw.drawShapes = true;
	return draw;
}

void cf_physics_draw3(b3WorldId world, float thickness)
{
	b3DebugDraw draw = cf_physics_debug_draw3_defaults(thickness);
	draw.drawJoints = true;
	b3World_Draw(world, &draw, UINT64_MAX); // All category bits: draw everything.
}
