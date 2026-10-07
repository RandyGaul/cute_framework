/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>
#include <SDL3/SDL.h>
#include <stdlib.h>

using namespace Cute;

#define W 32
#define H 32

static bool s_near(int a, int b) { int d = a - b; return (d < 0 ? -d : d) < 24; }

// A failed REQUIRE below returns out of the test case early, so destroying the app must
// happen via RAII rather than a final statement -- otherwise the leaked app (and its
// file system init) breaks cf_make_app in whichever test case runs next.
struct AppDestroyGuard
{
	~AppDestroyGuard() { test_destroy_app(); }
};

static CF_Pixel s_clear_and_read(CF_Canvas canvas, CF_Pixel* px)
{
	cf_app_update(NULL);
	cf_clear_canvas(canvas);
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	if (getenv("CF_TEST_DUMP")) {
		// Debug aid: a stride/pitch bug shows up as row 0 reading correctly while row 1
		// (and beyond) start mid-garbage; a wrong-clear bug reads uniformly wrong instead.
		printf(
			"backend=%s row0=%08x %08x row1=%08x %08x center=%08x\n",
			cf_backend_type_to_string(cf_query_backend()),
			px[0].val, px[1].val, px[W].val, px[W + 1].val, px[(H / 2) * W + (W / 2)].val
		);
	}

	return px[(H / 2) * W + (W / 2)];
}

// cf_clear_color is a single global that every clear reads, so a multi-pass renderer wanting
// different clears per pass has to set it before each pass and restore it afterwards. Two
// canvases must be able to hold their own clear colors at the same time.
TEST_CASE(test_per_canvas_clear_color)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	AppDestroyGuard app_guard;

	CF_Canvas red_canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Canvas blue_canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Canvas inherits = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	// The global stays green the whole way through; neither override touches it.
	cf_clear_color(0, 1.0f, 0, 1.0f);
	cf_canvas_set_clear_color(red_canvas, cf_make_color_rgb_f(1.0f, 0, 0));
	cf_canvas_set_clear_color(blue_canvas, cf_make_color_rgb_f(0, 0, 1.0f));

	CF_Pixel r = s_clear_and_read(red_canvas, px);
	REQUIRE(s_near(r.colors.r, 255) && s_near(r.colors.b, 0));

	CF_Pixel b = s_clear_and_read(blue_canvas, px);
	REQUIRE(s_near(b.colors.b, 255) && s_near(b.colors.r, 0));

	// A canvas with no override still follows the global, so existing code is unaffected.
	CF_Pixel g = s_clear_and_read(inherits, px);
	REQUIRE(s_near(g.colors.g, 255) && s_near(g.colors.r, 0) && s_near(g.colors.b, 0));

	// And setting overrides did not disturb the global for anyone else.
	CF_Pixel g2 = s_clear_and_read(inherits, px);
	REQUIRE(s_near(g2.colors.g, 255));

	cf_free(px);
	cf_destroy_canvas(red_canvas);
	cf_destroy_canvas(blue_canvas);
	cf_destroy_canvas(inherits);
	return true;
}

// cf_clear_canvas hardcoded clear_depth to 1.0 and clear_stencil to 0 on the SDL_GPU backend,
// ignoring cf_clear_depth_stencil entirely -- while the cf_apply_canvas path honored it, so the
// two disagreed. Clearing depth to 0 with a LESS_THAN test must reject everything drawn after.
TEST_CASE(test_canvas_clear_depth_is_honored)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	AppDestroyGuard app_guard;

	const char* vs =
		"layout (location = 0) in vec2 in_pos;\n"
		"void main() { gl_Position = vec4(in_pos, 0.5, 1); }\n"; // Fixed mid-range depth.
	const char* fs =
		"layout(location = 0) out vec4 result;\n"
		"void main() { result = vec4(1, 0, 0, 1); }\n";

	struct Vertex { float x, y; };
	Vertex verts[4] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
	uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 4);
	cf_mesh_set_index_buffer(mesh, sizeof(indices), 16);
	cf_mesh_update_index_data(mesh, indices, 6);

	CF_Shader shader = cf_make_shader_from_source(vs, fs);
	REQUIRE(shader.id);

	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);

	CF_Material material = cf_make_material();
	CF_RenderState rs = cf_render_state_defaults();
	rs.depth_write_enabled = true;
	rs.depth_compare = CF_COMPARE_FUNCTION_LESS_THAN;
	cf_material_set_render_state(material, rs);

	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_clear_color(0, 0, 0, 1.0f);

	// Depth cleared to 0: the quad at 0.5 fails LESS_THAN and must not appear.
	cf_canvas_set_clear_depth_stencil(canvas, 0.0f, 0);
	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);
	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	REQUIRE(s_near(px[(H / 2) * W + (W / 2)].colors.r, 0)); // Rejected.

	// Depth cleared to 1: the same quad now passes.
	cf_canvas_set_clear_depth_stencil(canvas, 1.0f, 0);
	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);
	rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	REQUIRE(s_near(px[(H / 2) * W + (W / 2)].colors.r, 255)); // Drawn.

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(material);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	return true;
}

struct MsaaRestoreGuard
{
	~MsaaRestoreGuard() { cf_app_set_msaa(1); }
};

// cf_app_set_msaa says yes only to sample counts the device really renders: a canvas made at an
// accepted count shows partially covered pixels along a triangle edge, never just 0 or 255.
TEST_CASE(test_msaa_query_is_honest)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	AppDestroyGuard app_guard;
	MsaaRestoreGuard msaa_guard;

	const char* vs =
		"layout (location = 0) in vec2 in_pos;\n"
		"void main() { gl_Position = vec4(in_pos, 0, 1); }\n";
	const char* fs =
		"layout(location = 0) out vec4 result;\n"
		"void main() { result = vec4(1, 0, 0, 1); }\n";
	struct Vertex { float x, y; };
	Vertex verts[3] = { { -1, -1 }, { 1, -1 }, { 1, 0.7f } }; // A shallow edge crossing many pixels.
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 3);
	CF_Shader shader = cf_make_shader_from_source(vs, fs);
	REQUIRE(shader.id);
	CF_Material material = cf_make_material();
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_clear_color(0, 0, 0, 1.0f);

	int counts[3] = { 2, 4, 8 };
	CF_SampleCount sample_counts[3] = { CF_SAMPLE_COUNT_2, CF_SAMPLE_COUNT_4, CF_SAMPLE_COUNT_8 };
	for (int i = 0; i < 3; ++i) {
		if (!cf_app_set_msaa(counts[i])) continue;
		CF_CanvasParams params = cf_canvas_defaults(W, H);
		params.sample_count = sample_counts[i];
		params.depth_stencil_enable = true;
		CF_Canvas canvas = cf_make_canvas(params);
		REQUIRE(canvas.id);
		cf_app_update(NULL);
		cf_apply_canvas(canvas, true);
		cf_apply_mesh(mesh);
		cf_apply_shader(shader, material);
		cf_draw_elements();
		cf_app_draw_onto_screen(false);
		CF_Readback rb = cf_canvas_readback(canvas);
		REQUIRE(rb.id);
		while (!cf_readback_ready(rb)) {}
		cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
		cf_destroy_readback(rb);
		int partial = 0;
		for (int p = 0; p < W * H; ++p) {
			if (px[p].colors.r > 30 && px[p].colors.r < 225) ++partial;
		}
		REQUIRE(partial > 0);
		cf_destroy_canvas(canvas);
	}

	cf_free(px);
	cf_destroy_material(material);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	return true;
}

// A quad rasterized at depth 0.9 whose shader writes gl_FragDepth = 0.3 must store 0.3. Two probe
// quads bracket the stored value: the left half at 0.31 must be rejected, the right half at 0.29
// must pass.
TEST_CASE(test_frag_depth_is_stored)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	AppDestroyGuard app_guard;

	const char* writer_vs =
		"layout (location = 0) in vec2 in_pos;\n"
		"void main() { gl_Position = vec4(in_pos, 0.9, 1); }\n";
	const char* writer_fs =
		"layout(location = 0) out vec4 result;\n"
		"void write_depth() { gl_FragDepth = 0.3; }\n"
		"void main() { write_depth(); result = vec4(1, 0, 0, 1); }\n";
	const char* probe_vs =
		"layout (location = 0) in vec2 in_pos;\n"
		"layout (set = 1, binding = 0) uniform uniform_block {\n"
		"	vec4 u_place;\n" // x: horizontal offset, y: depth.
		"};\n"
		"void main() { gl_Position = vec4(in_pos.x * 0.5 + u_place.x, in_pos.y, u_place.y, 1); }\n";
	const char* probe_fs =
		"layout(location = 0) out vec4 result;\n"
		"layout (set = 3, binding = 0) uniform uniform_block {\n"
		"	vec4 u_color;\n"
		"};\n"
		"void main() { result = u_color; }\n";

	struct Vertex { float x, y; };
	Vertex verts[4] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
	uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 4);
	cf_mesh_set_index_buffer(mesh, sizeof(indices), 16);
	cf_mesh_update_index_data(mesh, indices, 6);

	CF_Shader writer = cf_make_shader_from_source(writer_vs, writer_fs);
	REQUIRE(writer.id);
	CF_Shader probe = cf_make_shader_from_source(probe_vs, probe_fs);
	REQUIRE(probe.id);

	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);

	CF_RenderState rs = cf_render_state_defaults();
	rs.depth_write_enabled = true;
	rs.depth_compare = CF_COMPARE_FUNCTION_LESS_THAN;
	CF_Material writer_material = cf_make_material();
	cf_material_set_render_state(writer_material, rs);
	CF_Material behind = cf_make_material();
	cf_material_set_render_state(behind, rs);
	CF_V4 behind_place = cf_v4(-0.5f, 0.31f, 0, 0);
	CF_Color green = cf_make_color_rgb_f(0, 1.0f, 0);
	cf_material_set_uniform_vs(behind, "u_place", &behind_place, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_fs(behind, "u_color", &green, CF_UNIFORM_TYPE_FLOAT4, 1);
	CF_Material in_front = cf_make_material();
	cf_material_set_render_state(in_front, rs);
	CF_V4 in_front_place = cf_v4(0.5f, 0.29f, 0, 0);
	CF_Color blue = cf_make_color_rgb_f(0, 0, 1.0f);
	cf_material_set_uniform_vs(in_front, "u_place", &in_front_place, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_fs(in_front, "u_color", &blue, CF_UNIFORM_TYPE_FLOAT4, 1);

	cf_clear_color(0, 0, 0, 1.0f);
	cf_canvas_set_clear_depth_stencil(canvas, 1.0f, 0);
	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(writer, writer_material);
	cf_draw_elements();
	cf_apply_shader(probe, behind);
	cf_draw_elements();
	cf_apply_shader(probe, in_front);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);

	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	CF_Pixel left = px[(H / 2) * W + (W / 4)];
	CF_Pixel right = px[(H / 2) * W + (3 * W / 4)];

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(writer_material);
	cf_destroy_material(behind);
	cf_destroy_material(in_front);
	cf_destroy_shader(writer);
	cf_destroy_shader(probe);
	cf_destroy_mesh(mesh);

	REQUIRE(s_near(left.colors.r, 255) && s_near(left.colors.g, 0));
	REQUIRE(s_near(right.colors.b, 255) && s_near(right.colors.r, 0));
	return true;
}

// A window with no area (a collapsed browser canvas, a layout transition) used to rebuild the
// app canvas at 0x0, which asserts on the next frame on every backend.
TEST_CASE(test_zero_size_window_keeps_app_canvas)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	AppDestroyGuard app_guard;
	int canvas_w = cf_app_get_canvas_width();
	int canvas_h = cf_app_get_canvas_height();

	cf_app_set_size(0, 0);
	REQUIRE(cf_app_get_canvas_width() == canvas_w);
	REQUIRE(cf_app_get_canvas_height() == canvas_h);
	cf_app_update(NULL);
	cf_draw_box(cf_make_aabb(cf_v2(-8, -8), cf_v2(8, 8)), 1.0f, 0);
	cf_app_draw_onto_screen(true);

	// The same collapse arriving as a window event.
	SDL_Event e = { };
	e.type = SDL_EVENT_WINDOW_RESIZED;
	e.window.windowID = SDL_GetWindowID(cf_app_get_window());
	REQUIRE(SDL_PushEvent(&e));
	cf_app_update(NULL);
	REQUIRE(cf_app_get_canvas_width() == canvas_w);
	REQUIRE(cf_app_get_canvas_height() == canvas_h);
	cf_app_draw_onto_screen(true);

	cf_app_set_size(W, H);
	float scale = cf_app_get_pixel_scale();
	REQUIRE(cf_app_get_canvas_width() == (int)CF_ROUNDF(W * scale));
	REQUIRE(cf_app_get_canvas_height() == (int)CF_ROUNDF(H * scale));
	cf_app_update(NULL);
	cf_draw_box(cf_make_aabb(cf_v2(-8, -8), cf_v2(8, 8)), 1.0f, 0);
	cf_app_draw_onto_screen(true);
	return true;
}

TEST_SUITE(test_canvas_clear)
{
	RUN_TEST_CASE(test_per_canvas_clear_color);
	RUN_TEST_CASE(test_canvas_clear_depth_is_honored);
	RUN_TEST_CASE(test_msaa_query_is_honest);
	RUN_TEST_CASE(test_frag_depth_is_stored);
	RUN_TEST_CASE(test_zero_size_window_keeps_app_canvas);
}
