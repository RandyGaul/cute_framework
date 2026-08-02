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

static int s_options()
{
	int options = CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT;
	const char* gles = getenv("CF_TEST_GLES");
	if (gles && *gles == '1') options |= CF_APP_OPTIONS_GFX_OPENGL_BIT | CF_APP_OPTIONS_GFX_DEBUG_BIT;
	return options;
}

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
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, s_options(), NULL))) return true; // Headless CI: no display/GPU.

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
	cf_destroy_app();
	return true;
}

// Meshes ride the shared command stream: cf_draw_push_layer orders a mesh against 2d drawing
// in a single cf_render_to, in both directions.
TEST_CASE(test_draw3d_layers_with_2d)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, s_options(), NULL))) return true; // Headless CI: no display/GPU.

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
	cf_destroy_app();
	return true;
}

// Uniforms are captured per submission: changing one between two submissions splits the
// coalescing group and each draw sees its own value.
TEST_CASE(test_draw3d_uniform_capture)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, s_options(), NULL))) return true; // Headless CI: no display/GPU.

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
	cf_destroy_app();
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
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, s_options(), NULL))) return true; // Headless CI: no display/GPU.

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
	cf_destroy_app();
	return true;
}

// Draw lists: record interleaved submissions of two meshes, bake, and replay under two
// different live cameras -- the recorded level renders where the replay-time view says, and
// a transform pushed at replay time moves the whole list.
TEST_CASE(test_draw3d_draw_list)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, s_options(), NULL))) return true; // Headless CI: no display/GPU.

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
	cf_destroy_app();
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
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, s_options(), NULL))) return true; // Headless CI: no display/GPU.

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
	cf_destroy_app();
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
}
