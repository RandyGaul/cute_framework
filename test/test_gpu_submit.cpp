/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include "test_app_shared.h"
#include "test_leak.h"

#include <cute.h>
#include <internal/cute_graphics_internal.h>

using namespace Cute;

#define W 32
#define H 32

struct AppDestroyGuard
{
	~AppDestroyGuard() { test_destroy_app(); }
};

static bool s_near(int a, int b) { int d = a - b; return (d < 0 ? -d : d) < 24; }

static bool s_is(CF_Pixel p, int r, int g, int b)
{
	return s_near(p.colors.r, r) && s_near(p.colors.g, g) && s_near(p.colors.b, b);
}

static void s_read(CF_Canvas canvas, CF_Pixel* px)
{
	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
}

static CF_Pixel s_at(CF_Pixel* px, float u, float v) { return px[(int)(v * H) * W + (int)(u * W)]; }

static void s_fill(CF_Aabb box, CF_Color color)
{
	cf_draw_push_color(color);
	cf_draw_box_fill(box, 0);
	cf_draw_pop_color();
}

static const float BIG = 1000.0f;

// Draw, submit, draw: the second render_to resumes the canvas with LOAD, and the first
// submit really sent its work -- a mid-frame readback after it sees the red half.
TEST_CASE(test_gpu_submit_between_render_to)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	AppDestroyGuard app_guard;

	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_clear_color(0, 0, 0, 1.0f);

	cf_app_update(NULL);
	s_fill(cf_make_aabb(cf_v2(-BIG, -BIG), cf_v2(0, BIG)), cf_make_color_rgb_f(1.0f, 0, 0));
	cf_render_to(canvas, true);
	cf_gpu_submit();
	s_read(canvas, px);
	CF_Pixel mid_left = s_at(px, 0.25f, 0.5f);
	CF_Pixel mid_right = s_at(px, 0.75f, 0.5f);

	s_fill(cf_make_aabb(cf_v2(0, -BIG), cf_v2(BIG, BIG)), cf_make_color_rgb_f(0, 0, 1.0f));
	cf_render_to(canvas, false);
	cf_app_draw_onto_screen(false);
	s_read(canvas, px);
	CF_Pixel left = s_at(px, 0.25f, 0.5f);
	CF_Pixel right = s_at(px, 0.75f, 0.5f);

	cf_free(px);
	cf_destroy_canvas(canvas);

	REQUIRE(s_is(mid_left, 255, 0, 0));
	REQUIRE(s_is(mid_right, 0, 0, 0));
	REQUIRE(s_is(left, 255, 0, 0));
	REQUIRE(s_is(right, 0, 0, 255));
	return true;
}

// Two submits in one frame, inside an open debug label (re-opened in each new command buffer).
TEST_CASE(test_gpu_submit_twice_in_one_frame)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	AppDestroyGuard app_guard;

	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_clear_color(0, 0, 0, 1.0f);

	cf_app_update(NULL);
	cf_push_gpu_label("gpu_submit_test");
	s_fill(cf_make_aabb(cf_v2(-BIG, -BIG), cf_v2(0, 0)), cf_make_color_rgb_f(1.0f, 0, 0));
	cf_render_to(canvas, true);
	cf_gpu_submit();
	s_fill(cf_make_aabb(cf_v2(0, -BIG), cf_v2(BIG, 0)), cf_make_color_rgb_f(0, 1.0f, 0));
	cf_render_to(canvas, false);
	cf_gpu_submit();
	s_fill(cf_make_aabb(cf_v2(-BIG, 0), cf_v2(0, BIG)), cf_make_color_rgb_f(0, 0, 1.0f));
	cf_render_to(canvas, false);
	cf_pop_gpu_label();
	cf_app_draw_onto_screen(false);
	s_read(canvas, px);

	// Quadrant orientation differs per backend; each color must own exactly one quadrant, and
	// the undrawn one keeps the clear color.
	CF_Pixel q[4] = { s_at(px, 0.25f, 0.25f), s_at(px, 0.75f, 0.25f), s_at(px, 0.25f, 0.75f), s_at(px, 0.75f, 0.75f) };
	int reds = 0, greens = 0, blues = 0, blacks = 0;
	for (int i = 0; i < 4; ++i) {
		reds += s_is(q[i], 255, 0, 0);
		greens += s_is(q[i], 0, 255, 0);
		blues += s_is(q[i], 0, 0, 255);
		blacks += s_is(q[i], 0, 0, 0);
	}

	cf_free(px);
	cf_destroy_canvas(canvas);

	REQUIRE(reds == 1 && greens == 1 && blues == 1 && blacks == 1);
	return true;
}

// Submit between two low-level draws on the same canvas: the pass reopens with LOAD, so the
// draw recorded before the submit survives.
TEST_CASE(test_gpu_submit_mid_pass)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	AppDestroyGuard app_guard;

	const char* vs =
		"layout (location = 0) in vec2 in_pos;\n"
		"layout (set = 1, binding = 0) uniform uniform_block {\n"
		"	vec4 u_offset;\n"
		"};\n"
		"void main() { gl_Position = vec4(in_pos.x * 0.5 + u_offset.x, in_pos.y, 0, 1); }\n";
	const char* fs =
		"layout(location = 0) out vec4 result;\n"
		"layout (set = 3, binding = 0) uniform uniform_block {\n"
		"	vec4 u_color;\n"
		"};\n"
		"void main() { result = u_color; }\n";

	struct Vertex { float x, y; };
	Vertex verts[6] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, -1 }, { 1, 1 }, { -1, 1 } };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);

	CF_Shader shader = cf_make_shader_from_source(vs, fs);
	REQUIRE(shader.id);

	CF_Material left = cf_make_material();
	CF_V4 left_offset = cf_v4(-0.5f, 0, 0, 0);
	CF_Color red = cf_make_color_rgb_f(1.0f, 0, 0);
	cf_material_set_uniform_vs(left, "u_offset", &left_offset, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_fs(left, "u_color", &red, CF_UNIFORM_TYPE_FLOAT4, 1);
	CF_Material right = cf_make_material();
	CF_V4 right_offset = cf_v4(0.5f, 0, 0, 0);
	CF_Color blue = cf_make_color_rgb_f(0, 0, 1.0f);
	cf_material_set_uniform_vs(right, "u_offset", &right_offset, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_fs(right, "u_color", &blue, CF_UNIFORM_TYPE_FLOAT4, 1);

	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_clear_color(0, 0, 0, 1.0f);

	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, left);
	cf_draw_elements();
	cf_gpu_submit();
	cf_apply_shader(shader, right);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);
	s_read(canvas, px);
	CF_Pixel l = s_at(px, 0.25f, 0.5f);
	CF_Pixel r = s_at(px, 0.75f, 0.5f);

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(left);
	cf_destroy_material(right);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);

	REQUIRE(s_is(l, 255, 0, 0));
	REQUIRE(s_is(r, 0, 0, 255));
	return true;
}

static int s_backend_error_count()
{
#ifdef CF_WEBGPU
	if (cf_query_backend() == CF_BACKEND_TYPE_WEBGPU) return cf_webgpu_error_count();
#endif
	return 0;
}

// Labels opened and closed on both sides of a pass boundary and a submit. WebGPU debug groups
// must balance per encoder and pass, and its command encoder takes none while a pass is open;
// an unbalanced group fails validation at the pass end or the submit.
TEST_CASE(test_gpu_labels_across_passes_and_submits)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	AppDestroyGuard app_guard;

	const char* vs =
		"layout (location = 0) in vec2 in_pos;\n"
		"layout (set = 1, binding = 0) uniform uniform_block {\n"
		"	vec4 u_offset;\n"
		"};\n"
		"void main() { gl_Position = vec4(in_pos.x * 0.5 + u_offset.x, in_pos.y, 0, 1); }\n";
	const char* fs =
		"layout(location = 0) out vec4 result;\n"
		"layout (set = 3, binding = 0) uniform uniform_block {\n"
		"	vec4 u_color;\n"
		"};\n"
		"void main() { result = u_color; }\n";
	struct Vertex { float x, y; };
	Vertex verts[6] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, -1 }, { 1, 1 }, { -1, 1 } };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);
	CF_Shader shader = cf_make_shader_from_source(vs, fs);
	REQUIRE(shader.id);
	CF_Material left = cf_make_material();
	CF_V4 left_offset = cf_v4(-0.5f, 0, 0, 0);
	CF_Color red = cf_make_color_rgb_f(1.0f, 0, 0);
	cf_material_set_uniform_vs(left, "u_offset", &left_offset, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_fs(left, "u_color", &red, CF_UNIFORM_TYPE_FLOAT4, 1);
	CF_Material right = cf_make_material();
	CF_V4 right_offset = cf_v4(0.5f, 0, 0, 0);
	CF_Color blue = cf_make_color_rgb_f(0, 0, 1.0f);
	cf_material_set_uniform_vs(right, "u_offset", &right_offset, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_fs(right, "u_color", &blue, CF_UNIFORM_TYPE_FLOAT4, 1);
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_clear_color(0, 0, 0, 1.0f);
	int errors = s_backend_error_count();

	cf_app_update(NULL);
	cf_push_gpu_label("outer");
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, left);
	cf_push_gpu_label("inner");
	cf_draw_elements();
	cf_pop_gpu_label();
	cf_pop_gpu_label(); // Opened before the pass, closed inside it.
	cf_push_gpu_label("late"); // Opened inside the pass, still open across the submit.
	cf_gpu_submit();
	cf_apply_shader(shader, right);
	cf_draw_elements();
	cf_pop_gpu_label();
	cf_push_gpu_label("open at frame end");
	cf_app_draw_onto_screen(false);
	s_read(canvas, px);
	CF_Pixel l = s_at(px, 0.25f, 0.5f);
	CF_Pixel r = s_at(px, 0.75f, 0.5f);
	int new_errors = s_backend_error_count() - errors;

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(left);
	cf_destroy_material(right);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);

	REQUIRE(new_errors == 0);
	REQUIRE(s_is(l, 255, 0, 0));
	REQUIRE(s_is(r, 0, 0, 255));
	return true;
}

// Labels open across a submit are remembered so they can be reopened, but only while open:
// distinct dynamic names (frame or entity IDs) must not pile up for the life of the app.
TEST_CASE(test_gpu_label_names_not_retained)
{
	if (!test_leak_check_enabled()) return true;
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	AppDestroyGuard app_guard;

	cf_app_update(NULL);
	cf_push_gpu_label("warm up");
	cf_gpu_submit();
	cf_pop_gpu_label();
	cf_app_draw_onto_screen(false);

	TestLeakCheck leak;
	test_leak_begin(&leak);
	for (int frame = 0; frame < 3; ++frame) {
		cf_app_update(NULL);
		for (int i = 0; i < 200; ++i) {
			cf_push_gpu_label(String::fmt("label %d/%d", frame, i).c_str());
			cf_pop_gpu_label();
		}
		cf_push_gpu_label(String::fmt("open at frame end %d", frame).c_str());
		cf_gpu_submit();
		cf_app_draw_onto_screen(false);
	}
	REQUIRE(test_leak_blocks(&leak) < 100);
	return true;
}

TEST_SUITE(test_gpu_submit)
{
	RUN_TEST_CASE(test_gpu_submit_between_render_to);
	RUN_TEST_CASE(test_gpu_submit_twice_in_one_frame);
	RUN_TEST_CASE(test_gpu_submit_mid_pass);
	RUN_TEST_CASE(test_gpu_labels_across_passes_and_submits);
	RUN_TEST_CASE(test_gpu_label_names_not_retained);
}
