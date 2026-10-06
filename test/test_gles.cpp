/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Regressions from the GLES backend's state caching, buffer streaming, and readback. The draw
// tests run on every backend; the ones that need a GLES-internal hook skip elsewhere.

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>

using namespace Cute;

#define W 32
#define H 32

struct Vertex { float x, y; float r, g, b, a; };

static const char* s_vs =
"layout (location = 0) in vec2 in_pos;\n"
"layout (location = 1) in vec4 in_col;\n"
"layout (location = 0) out vec4 v_col;\n"
"void main() { v_col = in_col; gl_Position = vec4(in_pos, 0, 1); }\n";

static const char* s_fs =
"layout (location = 0) in vec4 v_col;\n"
"layout (location = 0) out vec4 result;\n"
"void main() { result = v_col; }\n";

static CF_Mesh s_make_color_mesh(int vertex_capacity)
{
	CF_VertexAttribute attrs[2] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CF_OFFSET_OF(Vertex, x);
	attrs[1].name = "in_col";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT4;
	attrs[1].offset = CF_OFFSET_OF(Vertex, r);
	return cf_make_mesh(vertex_capacity * (int)sizeof(Vertex), attrs, 2, sizeof(Vertex));
}

// Two triangles covering NDC x in [x0, x1], full height.
static void s_fill_strip(Vertex* v, float x0, float x1, CF_Color c)
{
	float xs[6] = { x0, x1, x1, x0, x1, x0 };
	float ys[6] = { -1, -1, 1, -1, 1, 1 };
	for (int i = 0; i < 6; ++i) v[i] = { xs[i], ys[i], c.r, c.g, c.b, c.a };
}

// Scissor applies after the shader: SDL_GPU backends open the render pass there.
static void s_draw_fill(CF_Mesh mesh, CF_Shader shader, CF_Material material, CF_Color c, int scissor_w = 0, int scissor_h = 0)
{
	Vertex v[6];
	s_fill_strip(v, -1, 1, c);
	cf_mesh_update_vertex_data(mesh, v, 6);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	if (scissor_w) cf_apply_scissor(0, 0, scissor_w, scissor_h);
	cf_draw_elements();
}

static void s_read(CF_Canvas canvas, CF_Pixel* px)
{
	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
}

static bool s_is(CF_Pixel p, int r, int g, int b)
{
	auto near = [](int a, int b) { int d = a - b; return (d < 0 ? -d : d) < 24; };
	return near(p.colors.r, r) && near(p.colors.g, g) && near(p.colors.b, b);
}

// GL keeps its scissor box while the test is disabled. Caching the box of an unscissored
// draw as if it were applied let a later scissor equal to that box skip glScissor and
// stay clipped to an older one.
TEST_CASE(test_scissor_box_survives_unscissored_pass)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Canvas a = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Canvas b = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_Material material = cf_make_material();
	CF_Mesh mesh = s_make_color_mesh(6);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_canvas_set_clear_color(a, cf_color_black());
	cf_canvas_set_clear_color(b, cf_color_black());
	cf_app_update(NULL);

	cf_apply_canvas(a, true);
	s_draw_fill(mesh, shader, material, cf_color_red(), 8, 8);

	cf_apply_canvas(b, true);
	s_draw_fill(mesh, shader, material, cf_color_blue());

	cf_apply_canvas(a, false);
	s_draw_fill(mesh, shader, material, cf_color_green(), W, H);

	cf_app_draw_onto_screen(false);
	s_read(a, px);
	REQUIRE(s_is(px[(H / 2) * W + (W / 2)], 0, 255, 0));

	cf_free(px);
	cf_destroy_mesh(mesh);
	cf_destroy_material(material);
	cf_destroy_shader(shader);
	cf_destroy_canvas(a);
	cf_destroy_canvas(b);
	test_destroy_app();
	return true;
}

TEST_SUITE(test_gles)
{
	RUN_TEST_CASE(test_scissor_box_survives_unscissored_pass);
}
