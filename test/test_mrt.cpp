/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Multiple render targets: a canvas with more than one color attachment, written by a fragment
// shader through layout(location = N) outputs. Covers per-target clear colors and per-target
// readback, since a g-buffer is useless if you cannot verify what landed in attachment 1.

#include "test_harness.h"

#include <cute.h>

using namespace Cute;

#define W 64
#define H 64

static const char* s_vs =
"layout (location = 0) in vec2 in_pos;\n"
"void main() { gl_Position = vec4(in_pos, 0, 1); }\n";

// Writes a different color to each attachment -- the whole point of MRT.
static const char* s_fs =
"layout (location = 0) out vec4 out_a;\n"
"layout (location = 1) out vec4 out_b;\n"
"void main() {\n"
"    out_a = vec4(1.0, 0.0, 0.0, 1.0);\n"
"    out_b = vec4(0.0, 1.0, 0.0, 1.0);\n"
"}\n";

static CF_Mesh s_make_fullscreen_quad()
{
	struct Vertex { float x, y; };
	Vertex verts[6] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, -1 }, { 1, 1 }, { -1, 1 } };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);
	return mesh;
}

static CF_Pixel s_readback_center(CF_Canvas canvas, int index, CF_Pixel* px)
{
	CF_Readback rb = cf_canvas_readback2(canvas, index);
	if (!rb.id) return cf_make_pixel_rgba(0, 0, 0, 0); // Callers' color checks then fail loudly.
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	return px[(H / 2) * W + (W / 2)];
}

TEST_CASE(test_mrt_draw_and_clear)
{
	int options = CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT;
	const char* gles = getenv("CF_TEST_GLES");
	if (gles && *gles == '1') options |= CF_APP_OPTIONS_GFX_OPENGL_BIT | CF_APP_OPTIONS_GFX_DEBUG_BIT;
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, options, NULL))) return true; // Headless CI: no display/GPU.

	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.target_count = 2;
	CF_Canvas canvas = cf_make_canvas(params);
	REQUIRE(canvas.id);

	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	// Per-target clear colors, no draw: attachment 0 blue, attachment 1 white.
	cf_canvas_set_clear_color2(canvas, 0, cf_make_color_rgb_f(0, 0, 1.0f));
	cf_canvas_set_clear_color2(canvas, 1, cf_make_color_rgb_f(1.0f, 1.0f, 1.0f));
	cf_app_update(NULL);
	cf_clear_canvas(canvas);
	cf_app_draw_onto_screen(false);
	CF_Pixel c0 = s_readback_center(canvas, 0, px);
	CF_Pixel c1 = s_readback_center(canvas, 1, px);
	REQUIRE(c0.colors.b > 200 && c0.colors.r < 60);
	REQUIRE(c1.colors.r > 200 && c1.colors.g > 200 && c1.colors.b > 200);

	// Draw through a two-output fragment shader: red to 0, green to 1.
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_Mesh mesh = s_make_fullscreen_quad();
	CF_Material material = cf_make_material();

	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);

	CF_Pixel a = s_readback_center(canvas, 0, px);
	CF_Pixel b = s_readback_center(canvas, 1, px);
	REQUIRE(a.colors.r > 200 && a.colors.g < 60);
	REQUIRE(b.colors.g > 200 && b.colors.r < 60);

	// Out-of-range accessors fail cleanly rather than aliasing target 0.
	REQUIRE(cf_canvas_get_target2(canvas, 2).id == 0);
	REQUIRE(cf_canvas_readback2(canvas, 2).id == 0);
	// And index 0 matches the legacy accessor.
	REQUIRE(cf_canvas_get_target2(canvas, 0).id == cf_canvas_get_target(canvas).id);

	cf_free(px);
	cf_destroy_material(material);
	cf_destroy_mesh(mesh);
	cf_destroy_shader(shader);
	cf_destroy_canvas(canvas);
	cf_destroy_app();
	return true;
}

// A single-target canvas made with zero-initialized target_count must behave exactly as before
// this member existed -- the compatibility contract for every existing caller.
TEST_CASE(test_mrt_single_target_compat)
{
	int options = CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT;
	const char* gles = getenv("CF_TEST_GLES");
	if (gles && *gles == '1') options |= CF_APP_OPTIONS_GFX_OPENGL_BIT | CF_APP_OPTIONS_GFX_DEBUG_BIT;
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, options, NULL))) return true; // Headless CI: no display/GPU.

	CF_CanvasParams params = cf_canvas_defaults(W, H);
	params.target_count = 0; // Zero means one.
	CF_Canvas canvas = cf_make_canvas(params);
	REQUIRE(canvas.id);

	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_canvas_set_clear_color(canvas, cf_make_color_rgb_f(1.0f, 0, 1.0f));
	cf_app_update(NULL);
	cf_clear_canvas(canvas);
	cf_app_draw_onto_screen(false);
	CF_Pixel c = s_readback_center(canvas, 0, px);
	REQUIRE(c.colors.r > 200 && c.colors.b > 200 && c.colors.g < 60);
	REQUIRE(cf_canvas_readback2(canvas, 1).id == 0);

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_app();
	return true;
}

TEST_SUITE(test_mrt)
{
	RUN_TEST_CASE(test_mrt_draw_and_clear);
	RUN_TEST_CASE(test_mrt_single_target_compat);
}
