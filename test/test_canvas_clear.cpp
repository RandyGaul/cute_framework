/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
#include <stdlib.h>

using namespace Cute;

#define W 32
#define H 32

static int s_app_options()
{
	int options = CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT;
	const char* gles = getenv("CF_TEST_GLES");
	if (gles && *gles == '1') options |= CF_APP_OPTIONS_GFX_OPENGL_BIT | CF_APP_OPTIONS_GFX_DEBUG_BIT;
	return options;
}

static bool s_near(int a, int b) { int d = a - b; return (d < 0 ? -d : d) < 24; }

static CF_Pixel s_clear_and_read(CF_Canvas canvas, CF_Pixel* px)
{
	cf_app_update(NULL);
	cf_clear_canvas(canvas);
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	return px[(H / 2) * W + (W / 2)];
}

// cf_clear_color is a single global that every clear reads, so a multi-pass renderer wanting
// different clears per pass has to set it before each pass and restore it afterwards. Two
// canvases must be able to hold their own clear colors at the same time.
TEST_CASE(test_per_canvas_clear_color)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

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
	cf_destroy_app();
	return true;
}

// cf_clear_canvas hardcoded clear_depth to 1.0 and clear_stencil to 0 on the SDL_GPU backend,
// ignoring cf_clear_depth_stencil entirely -- while the cf_apply_canvas path honored it, so the
// two disagreed. Clearing depth to 0 with a LESS_THAN test must reject everything drawn after.
TEST_CASE(test_canvas_clear_depth_is_honored)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, s_app_options(), NULL))) return true; // Headless CI: no display/GPU.

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
	cf_destroy_app();
	return true;
}

TEST_SUITE(test_canvas_clear)
{
	RUN_TEST_CASE(test_per_canvas_clear_color);
	RUN_TEST_CASE(test_canvas_clear_depth_is_honored);
}
