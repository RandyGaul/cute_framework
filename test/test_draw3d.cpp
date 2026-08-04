/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// The draw3d layer end to end: mesh submissions through the shared 2d command stream. Covers
// the shader contract (reserved instance attributes + u_view_projection), automatic coalescing
// of consecutive submissions into one instanced draw, layer ordering against 2d drawing,
// per-submission uniform capture, and the user-owned-instancing escape hatch.

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>

using namespace Cute;

#define W 64
#define H 64

// Follows the shader contract in cute_draw3d.h: model rows + mesh attributes as color.
static const char* s_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 8) in vec4 in_model0;\n"
"layout (location = 9) in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 15) in vec4 in_mesh_attributes;\n"
"layout (location = 0) out vec4 v_color;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"    vec4 p = vec4(in_pos, 1.0);\n"
"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_color = in_mesh_attributes;\n"
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"}\n";

static const char* s_fs =
"layout (location = 0) in vec4 v_color;\n"
"layout (location = 0) out vec4 result;\n"
"void main() { result = v_color; }\n";

// Same contract, but modulated by a captured fragment uniform.
static const char* s_tint_fs =
"layout (location = 0) in vec4 v_color;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    vec4 u_tint;\n"
"};\n"
"void main() { result = v_color * u_tint; }\n";

// A unit quad in the xy plane (CCW winding, front-facing under cf_render_state_3d_defaults).
static CF_Mesh s_make_quad(float half)
{
	struct Vertex { float x, y, z; };
	Vertex verts[6] = {
		{ -half, -half, 0 }, { half, -half, 0 }, { half, half, 0 },
		{ -half, -half, 0 }, { half, half, 0 }, { -half, half, 0 },
	};
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);
	return mesh;
}

static CF_Pixel s_pixel(CF_Pixel* px, float fx, float fy) { return px[(int)(H * fy) * W + (int)(W * fx)]; }

// Three submissions of one quad, different transforms and mesh attributes: all coalesce into
// one instanced draw, and each instance lands where its captured transform says.
TEST_CASE(test_draw3d_transforms_and_coalescing)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Mesh mesh = s_make_quad(0.2f);
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
	cf_draw3d_push_shader(shader);

	struct { CF_V3 at; CF_V4 color; } spots[3] = {
		{ cf_v3(-0.5f, 0, 0), cf_v4(1, 0, 0, 1) },
		{ cf_v3( 0.5f, 0, 0), cf_v4(0, 1, 0, 1) },
		{ cf_v3( 0, 0.5f, 0), cf_v4(0, 0, 1, 1) },
	};
	for (int i = 0; i < 3; ++i) {
		cf_draw3d_push();
		cf_draw3d_translate(spots[i].at);
		cf_draw3d_push_mesh_attributes(spots[i].color);
		cf_draw3d_mesh(mesh);
		cf_draw3d_pop_mesh_attributes();
		cf_draw3d_pop();
	}
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	// NDC (-0.5, 0) is left-center; +y up in clip space maps blue to the upper half.
	CF_Pixel left = s_pixel(px, 0.25f, 0.5f);
	CF_Pixel right = s_pixel(px, 0.75f, 0.5f);
	CF_Pixel top = s_pixel(px, 0.5f, 0.25f);
	CF_Pixel center = s_pixel(px, 0.5f, 0.5f);
	REQUIRE(left.colors.r > 200 && left.colors.g < 60);
	REQUIRE(right.colors.g > 200 && right.colors.r < 60);
	REQUIRE(top.colors.b > 200 && top.colors.r < 60);
	REQUIRE(center.colors.r < 60 && center.colors.g < 60 && center.colors.b < 60);

	cf_draw3d_pop_shader();
	cf_draw3d_pop_projection();
	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	test_destroy_app();
	return true;
}

// Not an assertion test -- a benchmark, kept in the suite so flush-path changes get measured
// instead of guessed at (the geometry-arena lesson). Two scenarios over 200 frames each:
// interleaved (2000 objects cycling 6 meshes -- every submission breaks the batch, so the
// flush pays per-command instance uploads) and batched (2000 instances of 1 mesh -- one
// upload). Prints milliseconds; always passes.
TEST_CASE(test_draw3d_bench)
{
	// Costs tens of seconds by design; opt in when measuring flush-path changes.
	const char* bench = getenv("CF_BENCH");
	if (!bench || *bench != '1') return true;
	if (!test_make_app(256, 256)) return true; // Headless CI: no display/GPU.

	CF_Mesh meshes[6];
	for (int i = 0; i < 6; ++i) meshes[i] = s_make_quad(0.01f + 0.002f * i);
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_CanvasParams params = cf_canvas_defaults(256, 256);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);

	const int FRAMES = 200;
	const int OBJECTS = 2000;
	for (int scenario = 0; scenario < 2; ++scenario) {
		// Warm up buffers and pipelines outside the timed window.
		for (int warm = 0; warm < 5; ++warm) {
			cf_app_update(NULL);
			cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
			cf_draw3d_push_shader(shader);
			for (int i = 0; i < OBJECTS; ++i) {
				cf_draw3d_push();
				cf_draw3d_translate(cf_v3((i % 50) * 0.04f - 1.0f, (i / 50) * 0.045f - 1.0f, 0));
				cf_draw3d_mesh(meshes[scenario == 0 ? i % 6 : 0]);
				cf_draw3d_pop();
			}
			cf_render_to(canvas, true);
			cf_app_draw_onto_screen(false);
		}
		cf_gpu_sync();
		double t0 = cf_get_ticks() / (double)cf_get_tick_frequency();
		for (int f = 0; f < FRAMES; ++f) {
			cf_app_update(NULL);
			cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
			cf_draw3d_push_shader(shader);
			for (int i = 0; i < OBJECTS; ++i) {
				cf_draw3d_push();
				cf_draw3d_translate(cf_v3((i % 50) * 0.04f - 1.0f, (i / 50) * 0.045f - 1.0f, 0));
				cf_draw3d_mesh(meshes[scenario == 0 ? i % 6 : 0]);
				cf_draw3d_pop();
			}
			cf_render_to(canvas, true);
			cf_app_draw_onto_screen(false);
		}
		cf_gpu_sync();
		double ms = (cf_get_ticks() / (double)cf_get_tick_frequency() - t0) * 1000.0 / FRAMES;
		printf("[bench] draw3d %s: %.3f ms/frame (%d objects, %d frames)\n",
			scenario == 0 ? "interleaved-6-mesh" : "single-mesh-batched", ms, OBJECTS, FRAMES);
	}

	for (int i = 0; i < 6; ++i) cf_destroy_mesh(meshes[i]);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	test_destroy_app();
	return true;
}

// Meshes ride the shared command stream: cf_draw_push_layer orders a mesh against 2d drawing
// in a single cf_render_to, in both directions.
TEST_CASE(test_draw3d_layers_with_2d)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Mesh mesh = s_make_quad(0.5f);
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
	cf_draw3d_push_shader(shader);
	cf_draw3d_push_mesh_attributes(cf_v4(0, 1, 0, 1)); // Green mesh.

	// Round 1: 2d red box on layer 0, mesh above it on layer 1 -> center is green.
	// Round 2: mesh on layer 0, 2d red box above on layer 1 -> everything red.
	for (int round = 0; round < 2; ++round) {
		cf_app_update(NULL);
		cf_draw_push_layer(round == 0 ? 0 : 1);
		cf_draw_push_color(cf_color_red());
		cf_draw_box_fill(cf_make_aabb(cf_v2(-(float)W, -(float)H), cf_v2((float)W, (float)H)), 0);
		cf_draw_pop_color();
		cf_draw_pop_layer();

		cf_draw_push_layer(round == 0 ? 1 : 0);
		cf_draw3d_mesh(mesh);
		cf_draw_pop_layer();

		cf_render_to(canvas, true);
		cf_app_draw_onto_screen(false);

		CF_Readback rb = cf_canvas_readback(canvas);
		REQUIRE(rb.id);
		while (!cf_readback_ready(rb)) {}
		cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
		cf_destroy_readback(rb);

		CF_Pixel center = s_pixel(px, 0.5f, 0.5f);
		CF_Pixel corner = s_pixel(px, 0.05f, 0.05f);
		if (round == 0) {
			REQUIRE(center.colors.g > 200 && center.colors.r < 60);
		} else {
			REQUIRE(center.colors.r > 200 && center.colors.g < 60);
		}
		REQUIRE(corner.colors.r > 200 && corner.colors.g < 60);
	}

	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop_shader();
	cf_draw3d_pop_projection();
	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	test_destroy_app();
	return true;
}

// Uniforms are captured per submission: changing one between two submissions splits the
// coalescing group and each draw sees its own value.
TEST_CASE(test_draw3d_uniform_capture)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Mesh mesh = s_make_quad(0.2f);
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_tint_fs);
	REQUIRE(shader.id);
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
	cf_draw3d_push_shader(shader);
	cf_draw3d_push_mesh_attributes(cf_v4(1, 1, 1, 1)); // White base; tint decides.

	cf_draw3d_set_uniform_color("u_tint", cf_color_red());
	cf_draw3d_push();
	cf_draw3d_translate(cf_v3(-0.5f, 0, 0));
	cf_draw3d_mesh(mesh);
	cf_draw3d_pop();

	cf_draw3d_set_uniform_color("u_tint", cf_color_green());
	cf_draw3d_push();
	cf_draw3d_translate(cf_v3(0.5f, 0, 0));
	cf_draw3d_mesh(mesh);
	cf_draw3d_pop();

	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	CF_Pixel left = s_pixel(px, 0.25f, 0.5f);
	CF_Pixel right = s_pixel(px, 0.75f, 0.5f);
	REQUIRE(left.colors.r > 200 && left.colors.g < 60);
	REQUIRE(right.colors.g > 200 && right.colors.r < 60);

	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop_shader();
	cf_draw3d_pop_projection();
	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	test_destroy_app();
	return true;
}

// Escape hatch: a mesh with its own instance buffer is drawn as-is -- no reserved attributes,
// instancing entirely the user's.
static const char* s_escape_vs =
"layout (location = 0) in vec2 in_pos;\n"
"layout (location = 1) in vec4 in_inst;\n"
"layout (location = 2) in vec4 in_tint;\n"
"layout (location = 0) out vec4 v_tint;\n"
"void main() {\n"
"    v_tint = in_tint;\n"
"    gl_Position = vec4(in_pos * 0.4 + in_inst.xy, 0, 1);\n"
"}\n";

static const char* s_escape_fs =
"layout (location = 0) in vec4 v_tint;\n"
"layout (location = 0) out vec4 result;\n"
"void main() { result = v_tint; }\n";

TEST_CASE(test_draw3d_escape_hatch)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	struct Vertex { float x, y; };
	Vertex verts[6] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, -1 }, { 1, 1 }, { -1, 1 } };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);

	struct Instance { float off[4]; float tint[4]; };
	CF_VertexAttribute inst_attrs[2] = { };
	inst_attrs[0].name = "in_inst";
	inst_attrs[0].format = CF_VERTEX_FORMAT_FLOAT4;
	inst_attrs[0].offset = CF_OFFSET_OF(Instance, off);
	inst_attrs[0].per_instance = true;
	inst_attrs[1].name = "in_tint";
	inst_attrs[1].format = CF_VERTEX_FORMAT_FLOAT4;
	inst_attrs[1].offset = CF_OFFSET_OF(Instance, tint);
	inst_attrs[1].per_instance = true;
	cf_mesh_append_attributes(mesh, inst_attrs, 2);
	cf_mesh_set_instance_buffer(mesh, sizeof(Instance) * 2, sizeof(Instance));
	Instance instances[2] = {
		{ { -0.5f, 0, 0, 0 }, { 1.0f, 0, 0, 1.0f } },
		{ {  0.5f, 0, 0, 0 }, { 0, 1.0f, 0, 1.0f } },
	};
	cf_mesh_update_instance_data(mesh, instances, 2);

	CF_Shader shader = cf_make_shader_from_source(s_escape_vs, s_escape_fs);
	REQUIRE(shader.id);
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_draw3d_push_shader(shader);
	CF_RenderState rs = cf_render_state_3d_defaults();
	rs.depth_write_enabled = false;
	rs.depth_compare = CF_COMPARE_FUNCTION_ALWAYS;
	rs.cull_mode = CF_CULL_MODE_NONE;
	cf_draw3d_push_render_state(rs);
	cf_draw3d_mesh(mesh);
	cf_draw3d_pop_render_state();
	cf_draw3d_pop_shader();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	CF_Pixel left = s_pixel(px, 0.25f, 0.5f);
	CF_Pixel right = s_pixel(px, 0.75f, 0.5f);
	REQUIRE(left.colors.r > 200 && left.colors.g < 60);
	REQUIRE(right.colors.g > 200 && right.colors.r < 60);

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	test_destroy_app();
	return true;
}

// Draw lists: record interleaved submissions of two meshes, bake, and replay under two
// different live cameras -- the recorded level renders where the replay-time view says, and
// a transform pushed at replay time moves the whole list.
TEST_CASE(test_draw3d_draw_list)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Mesh mesh_a = s_make_quad(0.2f);
	CF_Mesh mesh_b = s_make_quad(0.1f);
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
	cf_draw3d_push_shader(shader);

	// Record A-B-A-B so the bake has something to group (A's fold together, B's fold together).
	CF_DrawList list = cf_make_draw_list();
	cf_draw_list_begin(list);
	struct { CF_Mesh mesh; CF_V3 at; CF_V4 color; } spots[4] = {
		{ mesh_a, cf_v3(-0.5f,  0.5f, 0), cf_v4(1, 0, 0, 1) },
		{ mesh_b, cf_v3( 0.5f,  0.5f, 0), cf_v4(0, 1, 0, 1) },
		{ mesh_a, cf_v3(-0.5f, -0.5f, 0), cf_v4(0, 0, 1, 1) },
		{ mesh_b, cf_v3( 0.5f, -0.5f, 0), cf_v4(1, 1, 0, 1) },
	};
	for (int i = 0; i < 4; ++i) {
		cf_draw3d_push();
		cf_draw3d_translate(spots[i].at);
		cf_draw3d_push_mesh_attributes(spots[i].color);
		cf_draw3d_mesh(spots[i].mesh);
		cf_draw3d_pop_mesh_attributes();
		cf_draw3d_pop();
	}
	cf_draw_list_end();

	// Replay 1: identity view -- everything where it was recorded. +y up puts red/green on top.
	cf_app_update(NULL);
	cf_draw_list(list);
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);
	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	CF_Pixel tl = s_pixel(px, 0.25f, 0.25f);
	CF_Pixel tr = s_pixel(px, 0.75f, 0.25f);
	CF_Pixel bl = s_pixel(px, 0.25f, 0.75f);
	CF_Pixel br = s_pixel(px, 0.75f, 0.75f);
	REQUIRE(tl.colors.r > 200 && tl.colors.g < 60 && tl.colors.b < 60);
	REQUIRE(tr.colors.g > 200 && tr.colors.r < 60);
	REQUIRE(bl.colors.b > 200 && bl.colors.r < 60);
	REQUIRE(br.colors.r > 200 && br.colors.g > 200 && br.colors.b < 60);

	// Replay 2: the camera is live -- a view shifting the world right by 1 pushes the left
	// column to the center. And the whole list moves for free under a replay-time transform.
	cf_app_update(NULL);
	cf_draw3d_push_view(cf_m4_translate(cf_v3(0.5f, 0, 0)));
	cf_draw3d_push();
	cf_draw3d_translate(cf_v3(0, -0.5f, 0));
	cf_draw_list(list);
	cf_draw3d_pop();
	cf_draw3d_pop_view();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);
	rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	// Recorded (-0.5, 0.5) + view shift (0.5) + list move (-0.5) = NDC (0, 0) = center.
	CF_Pixel center = s_pixel(px, 0.5f, 0.5f);
	REQUIRE(center.colors.r > 200 && center.colors.g < 60 && center.colors.b < 60);

	cf_destroy_draw_list(list);
	cf_draw3d_pop_shader();
	cf_draw3d_pop_projection();
	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh_a);
	cf_destroy_mesh(mesh_b);
	test_destroy_app();
	return true;
}

// Baked lists carry exact inverse-transpose normal matrices; the immediate path reuses the
// model rows. Render in_nmat0.x as color under scale(2,1,1): immediate sees 2.0 (saturates),
// baked sees the exact 0.5.
static const char* s_nmat_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 8) in vec4 in_model0;\n"
"layout (location = 9) in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 12) in vec4 in_nmat0;\n"
"layout (location = 0) out vec4 v_color;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"    vec4 p = vec4(in_pos, 1.0);\n"
"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_color = vec4(in_nmat0.x * 0.5, 0, 0, 1);\n"
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"}\n";

TEST_CASE(test_draw3d_baked_normal_matrices)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Mesh mesh = s_make_quad(0.3f);
	CF_Shader shader = cf_make_shader_from_source(s_nmat_vs, s_fs);
	REQUIRE(shader.id);
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
	cf_draw3d_push_shader(shader);

	for (int pass = 0; pass < 2; ++pass) {
		cf_app_update(NULL);
		CF_DrawList list = { 0 };
		if (pass == 1) {
			list = cf_make_draw_list();
			cf_draw_list_begin(list);
		}
		cf_draw3d_push();
		cf_draw3d_scale(cf_v3(2.0f, 1.0f, 1.0f));
		cf_draw3d_mesh(mesh);
		cf_draw3d_pop();
		if (pass == 1) {
			cf_draw_list_end();
			cf_draw_list(list);
		}
		cf_render_to(canvas, true);
		cf_app_draw_onto_screen(false);

		CF_Readback rb = cf_canvas_readback(canvas);
		REQUIRE(rb.id);
		while (!cf_readback_ready(rb)) {}
		cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
		cf_destroy_readback(rb);
		CF_Pixel center = s_pixel(px, 0.5f, 0.5f);
		if (pass == 0) {
			// Immediate: nmat row = model row = 2.0 -> 2.0 * 0.5 = 1.0 -> saturated red.
			REQUIRE(center.colors.r > 240);
		} else {
			// Baked: exact inverse-transpose gives 0.5 -> 0.5 * 0.5 = 0.25 -> ~64.
			REQUIRE(center.colors.r > 44 && center.colors.r < 84);
			cf_destroy_draw_list(list);
		}
	}

	cf_draw3d_pop_shader();
	cf_draw3d_pop_projection();
	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	test_destroy_app();
	return true;
}

// Sprite-textured meshes: two quads textured by two different easy sprites coalesce into one
// submission group, resolve through the texture atlas, and sample their own image via the
// in_uv_rect instance lane.
static const char* s_sprite_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 1) in vec2 in_uv;\n"
"layout (location = 8) in vec4 in_model0;\n"
"layout (location = 9) in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 11) in vec4 in_uv_rect;\n"
"layout (location = 0) out vec2 v_uv;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"    vec4 p = vec4(in_pos, 1.0);\n"
"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_uv = mix(in_uv_rect.xy, in_uv_rect.zw, in_uv);\n"
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"}\n";

static const char* s_sprite_fs =
"layout (location = 0) in vec2 v_uv;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_image;\n"
"void main() { result = texture(u_image, v_uv); }\n";

TEST_CASE(test_draw3d_sprite_textured)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	// A quad with uvs; uv (0, 0) samples the image's top-left, like 2d sprites.
	struct Vertex { float x, y, z, u, v; };
	Vertex verts[6] = {
		{ -0.2f, -0.2f, 0, 0, 1 }, { 0.2f, -0.2f, 0, 1, 1 }, { 0.2f, 0.2f, 0, 1, 0 },
		{ -0.2f, -0.2f, 0, 0, 1 }, { 0.2f, 0.2f, 0, 1, 0 }, { -0.2f, 0.2f, 0, 0, 0 },
	};
	CF_VertexAttribute attrs[2] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = CF_OFFSET_OF(Vertex, x);
	attrs[1].name = "in_uv";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CF_OFFSET_OF(Vertex, u);
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 2, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);

	// Two 8x8 solid-color sprites.
	CF_Pixel red[64], green[64];
	for (int i = 0; i < 64; ++i) {
		red[i] = cf_make_pixel_rgba(255, 0, 0, 255);
		green[i] = cf_make_pixel_rgba(0, 255, 0, 255);
	}
	CF_Sprite sprite_red = cf_make_easy_sprite_from_pixels(red, 8, 8);
	CF_Sprite sprite_green = cf_make_easy_sprite_from_pixels(green, 8, 8);

	CF_Shader shader = cf_make_shader_from_source(s_sprite_vs, s_sprite_fs);
	REQUIRE(shader.id);
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
	cf_draw3d_push_shader(shader);

	// Two frames: images may land on separate pages first flush; the second frame draws from
	// the settled atlas.
	for (int frame = 0; frame < 2; ++frame) {
		cf_app_update(NULL);
		cf_draw3d_push();
		cf_draw3d_translate(cf_v3(-0.5f, 0, 0));
		cf_draw3d_push_texture(&sprite_red);
		cf_draw3d_mesh(mesh);
		cf_draw3d_pop_texture();
		cf_draw3d_pop();

		cf_draw3d_push();
		cf_draw3d_translate(cf_v3(0.5f, 0, 0));
		cf_draw3d_push_texture(&sprite_green);
		cf_draw3d_mesh(mesh);
		cf_draw3d_pop_texture();
		cf_draw3d_pop();

		cf_render_to(canvas, true);
		cf_app_draw_onto_screen(false);
	}

	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	CF_Pixel left = s_pixel(px, 0.25f, 0.5f);
	CF_Pixel right = s_pixel(px, 0.75f, 0.5f);
	REQUIRE(left.colors.r > 200 && left.colors.g < 60);
	REQUIRE(right.colors.g > 200 && right.colors.r < 60);

	cf_draw3d_pop_shader();
	cf_draw3d_pop_projection();
	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	cf_easy_sprite_unload(&sprite_red);
	cf_easy_sprite_unload(&sprite_green);
	test_destroy_app();
	return true;
}


// Sprite texturing through a BAKED list: recorded submissions store image ids, the bake
// merges their image tables, and every replay re-resolves against the current atlas -- so
// two frames of replay must both sample the right images.
TEST_CASE(test_draw3d_sprite_textured_baked)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	struct Vertex { float x, y, z, u, v; };
	Vertex verts[6] = {
		{ -0.2f, -0.2f, 0, 0, 1 }, { 0.2f, -0.2f, 0, 1, 1 }, { 0.2f, 0.2f, 0, 1, 0 },
		{ -0.2f, -0.2f, 0, 0, 1 }, { 0.2f, 0.2f, 0, 1, 0 }, { -0.2f, 0.2f, 0, 0, 0 },
	};
	CF_VertexAttribute attrs[2] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = CF_OFFSET_OF(Vertex, x);
	attrs[1].name = "in_uv";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CF_OFFSET_OF(Vertex, u);
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 2, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);

	CF_Pixel red[64], green[64];
	for (int i = 0; i < 64; ++i) {
		red[i] = cf_make_pixel_rgba(255, 0, 0, 255);
		green[i] = cf_make_pixel_rgba(0, 255, 0, 255);
	}
	CF_Sprite sprite_red = cf_make_easy_sprite_from_pixels(red, 8, 8);
	CF_Sprite sprite_green = cf_make_easy_sprite_from_pixels(green, 8, 8);

	CF_Shader shader = cf_make_shader_from_source(s_sprite_vs, s_sprite_fs);
	REQUIRE(shader.id);
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
	cf_draw3d_push_shader(shader);

	// Record once: two sprite-textured submissions that bake into one instanced group.
	CF_DrawList list = cf_make_draw_list();
	cf_draw_list_begin(list);
	cf_draw3d_push();
	cf_draw3d_translate(cf_v3(-0.5f, 0, 0));
	cf_draw3d_push_texture(&sprite_red);
	cf_draw3d_mesh(mesh);
	cf_draw3d_pop_texture();
	cf_draw3d_pop();
	cf_draw3d_push();
	cf_draw3d_translate(cf_v3(0.5f, 0, 0));
	cf_draw3d_push_texture(&sprite_green);
	cf_draw3d_mesh(mesh);
	cf_draw3d_pop_texture();
	cf_draw3d_pop();
	cf_draw_list_end();

	// Replay across two frames; the second frame re-resolves against the settled atlas.
	for (int frame = 0; frame < 2; ++frame) {
		cf_app_update(NULL);
		cf_draw_list(list);
		cf_render_to(canvas, true);
		cf_app_draw_onto_screen(false);

		CF_Readback rb = cf_canvas_readback(canvas);
		REQUIRE(rb.id);
		while (!cf_readback_ready(rb)) {}
		cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
		cf_destroy_readback(rb);

		CF_Pixel left = s_pixel(px, 0.25f, 0.5f);
		CF_Pixel right = s_pixel(px, 0.75f, 0.5f);
		REQUIRE(left.colors.r > 200 && left.colors.g < 60);
		REQUIRE(right.colors.g > 200 && right.colors.r < 60);
	}

	cf_destroy_draw_list(list);
	cf_draw3d_pop_shader();
	cf_draw3d_pop_projection();
	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	cf_easy_sprite_unload(&sprite_red);
	cf_easy_sprite_unload(&sprite_green);
	test_destroy_app();
	return true;
}

// Mesh submissions into a multi-render-target canvas: the access layer's MRT composes with
// the draw3d layer -- one submission writes both targets.
static const char* s_mrt_fs =
"layout (location = 0) in vec4 v_color;\n"
"layout (location = 0) out vec4 out_a;\n"
"layout (location = 1) out vec4 out_b;\n"
"void main() {\n"
"    out_a = v_color;\n"
"    out_b = vec4(1.0 - v_color.rgb, 1.0);\n"
"}\n";

TEST_CASE(test_draw3d_mrt)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Mesh mesh = s_make_quad(0.5f);
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_mrt_fs);
	REQUIRE(shader.id);
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	params.target_count = 2;
	CF_Canvas canvas = cf_make_canvas(params);
	REQUIRE(canvas.id);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
	cf_draw3d_push_shader(shader);
	cf_draw3d_push_mesh_attributes(cf_v4(1, 0, 0, 1)); // Red -> target 0 red, target 1 cyan.
	cf_draw3d_mesh(mesh);
	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop_shader();
	cf_draw3d_pop_projection();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);

	for (int i = 0; i < 2; ++i) {
		CF_Readback rb = cf_canvas_readback2(canvas, i);
		REQUIRE(rb.id);
		while (!cf_readback_ready(rb)) {}
		cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
		cf_destroy_readback(rb);
		CF_Pixel c = s_pixel(px, 0.5f, 0.5f);
		if (i == 0) REQUIRE(c.colors.r > 200 && c.colors.g < 60 && c.colors.b < 60);
		else REQUIRE(c.colors.r < 60 && c.colors.g > 200 && c.colors.b > 200);
	}

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	test_destroy_app();
	return true;
}


// Projection helpers: project and unproject are inverses through both camera systems, for
// perspective and orthographic projections alike.
TEST_CASE(test_draw3d_project_unproject)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_M4x4 projections[2] = {
		cf_perspective(CF_PI / 3.0f, 1.0f, 0.5f, 100.0f),
		cf_ortho(-4, 4, -4, 4, 0.5f, 100.0f),
	};
	for (int p = 0; p < 2; ++p) {
		cf_draw3d_push_projection(projections[p]);
		cf_draw3d_push_view(cf_look_at(cf_v3(3, 4, 8), cf_v3(0, 1, 0), cf_v3(0, 1, 0)));

		CF_V3 world = cf_v3(0.7f, 1.3f, -0.4f);
		CF_V3 projected = cf_draw3d_project(world);
		REQUIRE(projected.z > 0 && projected.z < 1);

		// Unprojecting the same 2d point must produce a ray passing through the world point.
		CF_V3 origin, dir;
		cf_draw3d_unproject(cf_v2(projected.x, projected.y), &origin, &dir);
		CF_V3 to_p = cf_sub_v3(world, origin);
		float t = cf_dot_v3(to_p, dir);
		REQUIRE(t > 0); // In front of the near plane.
		CF_V3 closest = cf_add_v3(origin, cf_mul_v3_f(dir, t));
		CF_V3 err = cf_sub_v3(closest, world);
		REQUIRE(cf_len_v3(err) < 0.001f);

		// A point behind a perspective camera reports a negative depth.
		if (p == 0) {
			CF_V3 behind = cf_draw3d_project(cf_v3(6, 8, 16));
			REQUIRE(behind.z < 0);
		}

		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();
	}

	test_destroy_app();
	return true;
}

// cf_draw3d_sprite is a plain quad oriented by the transform stack; cf_draw3d_billboard aims
// itself at the camera. A side-on camera tells them apart: the billboard shows its face, the
// plain sprite is edge-on and invisible.
TEST_CASE(test_draw3d_sprite_and_billboard)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Pixel red[64], green[64];
	for (int i = 0; i < 64; ++i) {
		red[i] = cf_make_pixel_rgba(255, 0, 0, 255);
		green[i] = cf_make_pixel_rgba(0, 255, 0, 255);
	}
	CF_Sprite sprite_red = cf_make_easy_sprite_from_pixels(red, 8, 8);
	CF_Sprite sprite_green = cf_make_easy_sprite_from_pixels(green, 8, 8);
	// World size = pixels * scale: 8 * 0.05 = 0.4 units.
	sprite_red.scale = cf_v2(0.05f, 0.05f);
	sprite_green.scale = cf_v2(0.05f, 0.05f);

	CF_Shader shader = cf_make_shader_from_source(s_sprite_vs, s_sprite_fs);
	REQUIRE(shader.id);
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	CF_RenderState rs = cf_render_state_3d_defaults();
	rs.cull_mode = CF_CULL_MODE_NONE;

	cf_draw3d_push_shader(shader);
	cf_draw3d_push_render_state(rs);

	// Front-on camera: two plain sprites, side by side, coalescing like any mesh.
	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
	cf_draw3d_sprite(&sprite_red, cf_v3(-0.5f, 0, 0));
	cf_draw3d_sprite(&sprite_green, cf_v3(0.5f, 0, 0));
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);
	cf_draw3d_pop_projection();

	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	CF_Pixel left = s_pixel(px, 0.25f, 0.5f);
	CF_Pixel right = s_pixel(px, 0.75f, 0.5f);
	REQUIRE(left.colors.r > 200 && left.colors.g < 60);
	REQUIRE(right.colors.g > 200 && right.colors.r < 60);

	// Side-on camera at +x: a plain sprite quad is edge-on (invisible), a billboard faces us.
	for (int use_billboard = 0; use_billboard < 2; ++use_billboard) {
		cf_app_update(NULL);
		cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, 0.1f, 10.0f));
		cf_draw3d_push_view(cf_look_at(cf_v3(3, 0, 0), cf_v3(0, 0, 0), cf_v3(0, 1, 0)));
		if (use_billboard) cf_draw3d_billboard(&sprite_green, cf_v3(0, 0, 0));
		else cf_draw3d_sprite(&sprite_red, cf_v3(0, 0, 0));
		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();
		cf_render_to(canvas, true);
		cf_app_draw_onto_screen(false);

		rb = cf_canvas_readback(canvas);
		REQUIRE(rb.id);
		while (!cf_readback_ready(rb)) {}
		cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
		cf_destroy_readback(rb);
		CF_Pixel center = s_pixel(px, 0.5f, 0.5f);
		if (use_billboard) REQUIRE(center.colors.g > 200);
		else REQUIRE(center.colors.r < 60 && center.colors.g < 60); // Edge-on: nothing.
	}

	cf_draw3d_pop_render_state();
	cf_draw3d_pop_shader();
	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_easy_sprite_unload(&sprite_red);
	cf_easy_sprite_unload(&sprite_green);
	test_destroy_app();
	return true;
}

// Mesh submissions into an MSAA canvas: the multisampled pipeline paths compose with draw3d.
// The result reads back through a blit into a single-sample canvas (which is also where the
// resolve happens on backends that multisample; GLES clamps to one sample and still passes).
TEST_CASE(test_draw3d_msaa)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Mesh mesh = s_make_quad(0.5f);
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	params.sample_count = CF_SAMPLE_COUNT_4;
	CF_Canvas msaa = cf_make_canvas(params);
	REQUIRE(msaa.id);
	CF_Canvas plain = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
	cf_draw3d_push_shader(shader);
	cf_draw3d_push_mesh_attributes(cf_v4(0, 1, 0, 1));
	cf_draw3d_mesh(mesh);
	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop_shader();
	cf_draw3d_pop_projection();
	cf_render_to(msaa, true);

	// Blit the (resolved) msaa canvas into a plain one for readback.
	cf_draw_canvas(msaa, cf_v2(0, 0), cf_v2((float)W, (float)H));
	cf_render_to(plain, true);
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(plain);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	CF_Pixel center = s_pixel(px, 0.5f, 0.5f);
	REQUIRE(center.colors.g > 200 && center.colors.r < 60);

	cf_free(px);
	cf_destroy_canvas(msaa);
	cf_destroy_canvas(plain);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	test_destroy_app();
	return true;
}

// Built-in SDF shapes: strokes (line, filled disc) and solids (cube, sphere) hit their pixels
// under the built-in shaders, honoring the color stack, with no user shader pushed at all.
TEST_CASE(test_draw3d_shapes)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, 0.1f, 10.0f));
	cf_draw3d_push_view(cf_look_at(cf_v3(0, 0, 2), cf_v3(0, 0, 0), cf_v3(0, 1, 0)));

	// A green stroke through the center, a red disc left, a blue cube right, a yellow sphere top.
	cf_draw3d_push_color(cf_make_color_rgb_f(0, 1, 0));
	cf_draw3d_line(cf_v3(-0.2f, 0, 0), cf_v3(0.2f, 0, 0), 0.1f);
	cf_draw3d_pop_color();
	cf_draw3d_push_color(cf_make_color_rgb_f(1, 0, 0));
	cf_draw3d_circle_fill(cf_v3(-0.5f, 0, 0), cf_v3(0, 0, 1), 0.2f);
	cf_draw3d_pop_color();
	cf_draw3d_push_color(cf_make_color_rgb_f(0, 0, 1));
	cf_draw3d_cube(cf_v3(0.5f, 0, 0), cf_v3(0.2f, 0.2f, 0.2f));
	cf_draw3d_pop_color();
	cf_draw3d_push_color(cf_make_color_rgb_f(1, 1, 0));
	cf_draw3d_sphere(cf_v3(0, 0.5f, 0), 0.2f);
	cf_draw3d_pop_color();
	// The rest of the catalog as smoke coverage (correct pixels eyeballed in the shapes sample).
	cf_draw3d_circle(cf_v3(-0.5f, -0.5f, 0), cf_v3(0, 0, 1), 0.15f, 0.02f);
	cf_draw3d_arc(cf_v3(0.5f, -0.5f, 0), cf_v3(0, 0, 1), 0.15f, 0, 3.0f, 0.02f);
	cf_draw3d_box_wire(cf_v3(0, -0.5f, 0), cf_v3(0.1f, 0.1f, 0.1f), 0.02f);
	cf_draw3d_arrow(cf_v3(-0.8f, -0.8f, 0), cf_v3(-0.6f, -0.8f, 0), 0.02f, 0.05f);
	cf_draw3d_torus(cf_v3(0.8f, 0.8f, 0), cf_v3(0, 0, 1), 0.1f, 0.03f);
	// A magenta dashed line along the bottom: dash centers hit their pixels, gap centers don't.
	cf_draw3d_push_color(cf_make_color_rgb_f(1, 0, 1));
	cf_draw3d_push_dash(0.3f, 0.3f, 0);
	cf_draw3d_line(cf_v3(-0.6f, -0.75f, 0), cf_v3(0.6f, -0.75f, 0), 0.1f);
	cf_draw3d_pop_dash();
	cf_draw3d_pop_color();
	cf_draw3d_push();
	cf_draw3d_translate(cf_v3(-0.8f, 0.8f, 0));
	cf_draw3d_axes(0.1f, 0.02f);
	cf_draw3d_pop();

	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	CF_Pixel center = s_pixel(px, 0.5f, 0.5f);
	CF_Pixel left = s_pixel(px, 0.25f, 0.5f);
	CF_Pixel right = s_pixel(px, 0.75f, 0.5f);
	CF_Pixel top = s_pixel(px, 0.5f, 0.25f);
	REQUIRE(center.colors.g > 200 && center.colors.r < 60);
	REQUIRE(left.colors.r > 200 && left.colors.g < 60);
	// Solids are hemisphere-lit, so their channels dim but keep their hue.
	REQUIRE(right.colors.b > 80 && right.colors.r < 60);
	REQUIRE(top.colors.r > 80 && top.colors.g > 60 && top.colors.b < 60);
	CF_Pixel dash_on = s_pixel(px, 0.275f, 0.875f);
	CF_Pixel dash_off = s_pixel(px, 0.425f, 0.875f);
	REQUIRE(dash_on.colors.r > 200 && dash_on.colors.b > 200);
	REQUIRE(dash_off.colors.r < 60);

	cf_draw3d_pop_view();
	cf_draw3d_pop_projection();
	cf_free(px);
	cf_destroy_canvas(canvas);
	test_destroy_app();
	return true;
}

// Shape shader stubs (cf_make_draw3d_shape_shader): a user snippet post-processes the
// built-in stroke and solid results, with per-shape attributes flowing through ShapeParams.
TEST_CASE(test_draw3d_shape_shader)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	// Swaps every shape's color for its attributes lane, premultiplied-safe.
	CF_Shader shd = cf_make_draw3d_shape_shader_from_source(
		"vec4 shader(vec4 color, ShapeParams params)\n"
		"{\n"
		"    return vec4(params.attributes.rgb * color.a, color.a);\n"
		"}\n");
	REQUIRE(shd.id);

	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, 0.1f, 10.0f));
	cf_draw3d_push_view(cf_look_at(cf_v3(0, 0, 2), cf_v3(0, 0, 0), cf_v3(0, 1, 0)));

	cf_draw3d_push_shader(shd);
	cf_draw3d_push_mesh_attributes(cf_v4(1, 0, 1, 1)); // Magenta via the stub.
	cf_draw3d_push_color(cf_make_color_rgb_f(0, 1, 0)); // Green built-in result, replaced.
	cf_draw3d_line(cf_v3(-0.5f, 0, 0), cf_v3(0.5f, 0, 0), 0.2f); // Stroke pipeline.
	cf_draw3d_sphere(cf_v3(0, 0.5f, 0), 0.2f);                   // Solid pipeline.
	cf_draw3d_pop_color();
	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop_shader();

	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	CF_Pixel center = s_pixel(px, 0.5f, 0.5f); // Stroke, tinted by the stub.
	CF_Pixel top = s_pixel(px, 0.5f, 0.25f);   // Solid, tinted by the stub.
	REQUIRE(center.colors.r > 200 && center.colors.g < 60 && center.colors.b > 200);
	REQUIRE(top.colors.r > 100 && top.colors.g < 60 && top.colors.b > 100);

	cf_draw3d_pop_view();
	cf_draw3d_pop_projection();
	cf_destroy_shader(shd); // Also destroys the hidden solid-variant sibling.
	cf_free(px);
	cf_destroy_canvas(canvas);
	test_destroy_app();
	return true;
}

// Stroke sizing: thickness scales with the transform stack in world mode, and holds a constant
// on-screen width under perspective in pixel mode. Also covers the second attributes lane.
TEST_CASE(test_draw3d_stroke_sizing)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	// World-unit thickness under a 3x scale must draw 3x thicker.
	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, 0.1f, 10.0f));
	cf_draw3d_push_view(cf_look_at(cf_v3(0, 0, 2), cf_v3(0, 0, 0), cf_v3(0, 1, 0)));
	cf_draw3d_push_color(cf_make_color_rgb_f(0, 1, 0));
	cf_draw3d_line(cf_v3(-0.9f, 0.5f, 0), cf_v3(0.9f, 0.5f, 0), 0.04f);
	cf_draw3d_push();
	cf_draw3d_scale(cf_v3(3.0f));
	// Same thickness argument, but the stack scale must triple it. Positions are divided by
	// 3 so the scaled line lands where the unscaled one would.
	cf_draw3d_line(cf_v3(-0.3f, -0.2f, 0), cf_v3(0.3f, -0.2f, 0), 0.04f);
	cf_draw3d_pop();
	cf_draw3d_pop_color();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);
	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	// Both lines cross x = center; count them separately by scanning halves.
	int top = 0, bottom = 0;
	for (int y = 0; y < H / 2; ++y) if (px[y * W + W / 2].colors.a > 40) top++;
	for (int y = H / 2; y < H; ++y) if (px[y * W + W / 2].colors.a > 40) bottom++;
	REQUIRE(top > 0 && bottom > 0);
	// The scaled line is meaningfully thicker than the unscaled one (3x nominal, allowing
	// for AA fringe on a small canvas).
	int thin = top < bottom ? top : bottom;
	int thick = top < bottom ? bottom : top;
	REQUIRE(thick >= thin * 2);
	cf_draw3d_pop_view();
	cf_draw3d_pop_projection();

	// Pixel mode under perspective: two lines at very different depths must render the same
	// on-screen width, where world-unit thickness would halve with distance.
	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_perspective(CF_PI / 3.0f, 1.0f, 0.1f, 100.0f));
	cf_draw3d_push_view(cf_look_at(cf_v3(0, 0, 0), cf_v3(0, 0, -1), cf_v3(0, 1, 0)));
	cf_draw3d_push_color(cf_make_color_rgb_f(0, 1, 0));
	cf_draw3d_push_stroke_pixels(true);
	cf_draw3d_line(cf_v3(-5, 1.2f, -4), cf_v3(5, 1.2f, -4), 5.0f);   // Near.
	cf_draw3d_line(cf_v3(-5, -1.2f, -8), cf_v3(5, -1.2f, -8), 5.0f); // Twice as far.
	cf_draw3d_pop_stroke_pixels();
	cf_draw3d_pop_color();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);
	rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	int near_px = 0, far_px = 0;
	for (int y = 0; y < H / 2; ++y) if (px[y * W + W / 2].colors.a > 40) near_px++;
	for (int y = H / 2; y < H; ++y) if (px[y * W + W / 2].colors.a > 40) far_px++;
	REQUIRE(near_px > 0 && far_px > 0);
	// Equal within a pixel of AA slop -- world-unit sizing would differ by 2x.
	int diff = near_px - far_px;
	if (diff < 0) diff = -diff;
	REQUIRE(diff <= 1);
	cf_draw3d_pop_view();
	cf_draw3d_pop_projection();

	cf_free(px);
	cf_destroy_canvas(canvas);
	test_destroy_app();
	return true;
}

// The second per-instance lane rides in_uv_rect when no sprite texture is pushed.
TEST_CASE(test_draw3d_attributes2)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	const char* vs =
		"layout (location = 0) in vec3 in_pos;\n"
		"layout (location = 8)  in vec4 in_model0;\n"
		"layout (location = 9)  in vec4 in_model1;\n"
		"layout (location = 10) in vec4 in_model2;\n"
		"layout (location = 11) in vec4 in_uv_rect;\n"
		"layout (location = 0) out vec4 v_color;\n"
		"layout (set = 1, binding = 0) uniform uniform_block { mat4 u_view_projection; };\n"
		"void main() {\n"
		"    vec4 p = vec4(in_pos, 1.0);\n"
		"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
		"    v_color = in_uv_rect;\n"
		"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
		"}\n";
	const char* fs =
		"layout (location = 0) in vec4 v_color;\n"
		"layout (location = 0) out vec4 result;\n"
		"void main() { result = v_color; }\n";
	CF_Shader shader = cf_make_shader_from_source(vs, fs);
	REQUIRE(shader.id);

	CF_V3 tri[3] = { cf_v3(-0.8f, -0.8f, 0), cf_v3(0.8f, -0.8f, 0), cf_v3(0, 0.8f, 0) };
	CF_VertexAttribute attr = { };
	attr.name = "in_pos";
	attr.format = CF_VERTEX_FORMAT_FLOAT3;
	attr.offset = 0;
	CF_Mesh mesh = cf_make_mesh((int)sizeof(tri), &attr, 1, (int)sizeof(CF_V3));
	cf_mesh_update_vertex_data(mesh, tri, 3);

	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, 0.1f, 10.0f));
	cf_draw3d_push_view(cf_look_at(cf_v3(0, 0, 2), cf_v3(0, 0, 0), cf_v3(0, 1, 0)));
	cf_draw3d_push_shader(shader);
	cf_draw3d_push_mesh_attributes2(cf_v4(1, 0, 1, 1)); // Magenta through the second lane.
	cf_draw3d_mesh(mesh);
	cf_draw3d_pop_mesh_attributes2();
	cf_draw3d_pop_shader();

	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);
	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	CF_Pixel center = s_pixel(px, 0.5f, 0.5f);
	REQUIRE(center.colors.r > 200 && center.colors.g < 60 && center.colors.b > 200);
	// The default is the full uv rect, so untextured shaders sampling u_image still work.
	REQUIRE(cf_draw3d_peek_mesh_attributes2().x == 0 && cf_draw3d_peek_mesh_attributes2().z == 1);

	cf_draw3d_pop_view();
	cf_draw3d_pop_projection();
	cf_free(px);
	cf_destroy_mesh(mesh);
	cf_destroy_shader(shader);
	cf_destroy_canvas(canvas);
	test_destroy_app();
	return true;
}

// Diagnostics: submissions that coalesce report one command, and each state change that breaks
// a batch must be named correctly -- the whole point is that the reason is trustworthy.
TEST_CASE(test_draw3d_stats)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Mesh mesh = s_make_quad(0.4f);
	CF_Mesh other = s_make_quad(0.2f);
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	CF_Shader shader2 = cf_make_shader_from_source(s_vs, s_fs);

	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, 0.1f, 10.0f));
	cf_draw3d_push_view(cf_look_at(cf_v3(0, 0, 2), cf_v3(0, 0, 0), cf_v3(0, 1, 0)));
	cf_draw3d_push_shader(shader);
	cf_draw3d_stats(); // Clear anything from setup.

	// Five submissions of one mesh under identical state: one draw call, four coalesced.
	for (int i = 0; i < 5; ++i) cf_draw3d_mesh(mesh);
	CF_DrawStats3d stats = cf_draw3d_stats();
	REQUIRE(stats.instances == 5);
	REQUIRE(stats.commands == 1);
	REQUIRE(stats.splits[CF_DRAW_SPLIT_3D_FIRST] == 1);

	// A different mesh in the middle breaks the batch, twice, and the breaks are attributed to
	// the mesh. Reading stats resets the counters but not the command stream, so the first
	// submission here still joins the batch left over from above -- two new commands, not three.
	cf_draw3d_mesh(mesh);
	cf_draw3d_mesh(other);
	cf_draw3d_mesh(mesh);
	stats = cf_draw3d_stats();
	REQUIRE(stats.instances == 3);
	REQUIRE(stats.commands == 2);
	REQUIRE(stats.splits[CF_DRAW_SPLIT_3D_MESH] == 2);
	REQUIRE(cf_draw3d_worst_split(&stats) == CF_DRAW_SPLIT_3D_MESH);

	// A shader change is reported as a shader break, not something else.
	cf_draw3d_mesh(mesh);
	cf_draw3d_push_shader(shader2);
	cf_draw3d_mesh(mesh);
	cf_draw3d_pop_shader();
	stats = cf_draw3d_stats();
	REQUIRE(stats.splits[CF_DRAW_SPLIT_3D_SHADER] == 1);

	// Mesh attributes are the supported way to vary per-instance data: they must NOT split.
	for (int i = 0; i < 4; ++i) {
		cf_draw3d_push_mesh_attributes(cf_v4((float)i, 0, 0, 1));
		cf_draw3d_mesh(mesh);
		cf_draw3d_pop_mesh_attributes();
	}
	stats = cf_draw3d_stats();
	REQUIRE(stats.instances == 4);
	REQUIRE(stats.commands == 1);

	// Uniforms, by contrast, do split -- and say so.
	for (int i = 0; i < 3; ++i) {
		float v = (float)i;
		cf_draw3d_set_uniform("u_thing", &v, CF_UNIFORM_TYPE_FLOAT, 1);
		cf_draw3d_mesh(mesh);
	}
	stats = cf_draw3d_stats();
	REQUIRE(stats.instances == 3);
	REQUIRE(stats.splits[CF_DRAW_SPLIT_3D_UNIFORMS] >= 2);

	cf_draw3d_pop_shader();
	cf_draw3d_pop_view();
	cf_draw3d_pop_projection();
	cf_render_to(cf_app_get_canvas(), true);
	cf_app_draw_onto_screen(false);
	cf_destroy_mesh(mesh);
	cf_destroy_mesh(other);
	cf_destroy_shader(shader);
	cf_destroy_shader(shader2);
	test_destroy_app();
	return true;
}

// Within one layer, depth-writing mesh commands render before non-writing ones regardless of
// submission order -- the opaque-then-translucent default. A non-writer submitted FIRST but
// sitting NEARER the camera must survive a writer submitted after it: the writer lays down
// depth first, the non-writer then depth-tests nearer and draws on top. Without the reorder
// the writer simply paints over the non-writer. This pins the run scan across the empty
// spacer commands each mesh submission leaves on the stream.
TEST_CASE(test_draw3d_depth_writer_order)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Mesh mesh = s_make_quad(0.4f);
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
	cf_draw3d_push_shader(shader);

	// Red quad: nearer (cf_ortho looks down -z, so +z is toward the camera), does NOT write
	// depth, submitted first.
	CF_RenderState no_write = cf_render_state_3d_defaults();
	no_write.depth_write_enabled = false;
	cf_draw3d_push_render_state(no_write);
	cf_draw3d_push();
	cf_draw3d_translate(cf_v3(0, 0, 0.5f));
	cf_draw3d_push_mesh_attributes(cf_v4(1, 0, 0, 1));
	cf_draw3d_mesh(mesh);
	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop();
	cf_draw3d_pop_render_state();

	// Blue quad: farther, writes depth, submitted second.
	cf_draw3d_push();
	cf_draw3d_translate(cf_v3(0, 0, -0.5f));
	cf_draw3d_push_mesh_attributes(cf_v4(0, 0, 1, 1));
	cf_draw3d_mesh(mesh);
	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop();

	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	// The writer renders first, so the nearer non-writer depth-tests over it: red wins.
	CF_Pixel center = s_pixel(px, 0.5f, 0.5f);
	REQUIRE(center.colors.r > 200 && center.colors.b < 60);

	cf_draw3d_pop_shader();
	cf_draw3d_pop_projection();
	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	test_destroy_app();
	return true;
}

// Within a layer, non-depth-writing (translucent) mesh commands sort back-to-front by their
// submission anchors: a translucent quad submitted NEAR-first must still composite under the
// over operator as near-over-far. Different meshes force separate commands, which is the
// case the sort exists for (same-mesh translucents coalesce and keep submission order).
TEST_CASE(test_draw3d_translucent_sort)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Mesh near_mesh = s_make_quad(0.4f);
	CF_Mesh far_mesh = s_make_quad(0.4001f); // Distinct mesh, visually identical.
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
	cf_draw3d_push_shader(shader);

	// Premultiplied over blend, no depth writes: the translucency configuration.
	CF_RenderState rs = cf_render_state_3d_defaults();
	rs.depth_write_enabled = false;
	rs.blend.enabled = true;
	rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	rs.blend.rgb_op = CF_BLEND_OP_ADD;
	rs.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	rs.blend.alpha_op = CF_BLEND_OP_ADD;
	cf_draw3d_push_render_state(rs);

	// NEAR half-alpha red submitted FIRST (+z is toward the camera under cf_ortho),
	// FAR opaque blue submitted second. Correct back-to-front: blue renders first, red
	// blends over it -> center (0.5, 0, 0.5). Submission order would give pure blue.
	cf_draw3d_push();
	cf_draw3d_translate(cf_v3(0, 0, 0.5f));
	cf_draw3d_push_mesh_attributes(cf_v4(0.5f, 0, 0, 0.5f)); // Premultiplied half red.
	cf_draw3d_mesh(near_mesh);
	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop();

	cf_draw3d_push();
	cf_draw3d_translate(cf_v3(0, 0, -0.5f));
	cf_draw3d_push_mesh_attributes(cf_v4(0, 0, 1.0f, 1.0f)); // Opaque blue.
	cf_draw3d_mesh(far_mesh);
	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop();

	cf_draw3d_pop_render_state();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	CF_Pixel center = s_pixel(px, 0.5f, 0.5f);
	REQUIRE(center.colors.r > 100 && center.colors.r < 160); // Half red over...
	REQUIRE(center.colors.b > 100 && center.colors.b < 160); // ...half-visible blue.

	cf_draw3d_pop_shader();
	cf_draw3d_pop_projection();
	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_mesh(near_mesh);
	cf_destroy_mesh(far_mesh);
	test_destroy_app();
	return true;
}

TEST_SUITE(test_draw3d)
{
	RUN_TEST_CASE(test_draw3d_transforms_and_coalescing);
	RUN_TEST_CASE(test_draw3d_layers_with_2d);
	RUN_TEST_CASE(test_draw3d_uniform_capture);
	RUN_TEST_CASE(test_draw3d_escape_hatch);
	RUN_TEST_CASE(test_draw3d_draw_list);
	RUN_TEST_CASE(test_draw3d_baked_normal_matrices);
	RUN_TEST_CASE(test_draw3d_sprite_textured);
	RUN_TEST_CASE(test_draw3d_sprite_textured_baked);
	RUN_TEST_CASE(test_draw3d_mrt);
	RUN_TEST_CASE(test_draw3d_project_unproject);
	RUN_TEST_CASE(test_draw3d_sprite_and_billboard);
	RUN_TEST_CASE(test_draw3d_msaa);
	RUN_TEST_CASE(test_draw3d_shapes);
	RUN_TEST_CASE(test_draw3d_shape_shader);
	RUN_TEST_CASE(test_draw3d_stroke_sizing);
	RUN_TEST_CASE(test_draw3d_attributes2);
	RUN_TEST_CASE(test_draw3d_stats);
	RUN_TEST_CASE(test_draw3d_depth_writer_order);
	RUN_TEST_CASE(test_draw3d_translucent_sort);
	RUN_TEST_CASE(test_draw3d_bench);
}
