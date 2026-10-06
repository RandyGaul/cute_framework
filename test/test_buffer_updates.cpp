/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Buffers updated several times in one frame with draws between: each draw must see the contents
// current when it was issued, whatever the backend does to avoid ending its render pass.

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>
#include <internal/cute_graphics_internal.h>
#include <internal/cute_draw_internal.h>

using namespace Cute;

#define W 64
#define H 64
#define COLUMNS 4

static const CF_Color s_colors[COLUMNS] = {
	{ 1, 0, 0, 1 },
	{ 0, 1, 0, 1 },
	{ 0, 0, 1, 1 },
	{ 1, 1, 0, 1 },
};

static const char* s_color_vs =
"layout (location = 0) in vec2 in_pos;\n"
"layout (location = 1) in vec4 in_col;\n"
"layout (location = 0) out vec4 v_col;\n"
"void main() { v_col = in_col; gl_Position = vec4(in_pos, 0, 1); }\n";

static const char* s_color_fs =
"layout (location = 0) in vec4 v_col;\n"
"layout (location = 0) out vec4 result;\n"
"void main() { result = v_col; }\n";

struct Vertex { float x, y; float c[4]; };

static CF_Mesh s_make_color_mesh(int vertex_capacity)
{
	CF_VertexAttribute attrs[2] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CF_OFFSET_OF(Vertex, x);
	attrs[1].name = "in_col";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT4;
	attrs[1].offset = CF_OFFSET_OF(Vertex, c);
	return cf_make_mesh(vertex_capacity * (int)sizeof(Vertex), attrs, 2, (int)sizeof(Vertex));
}

// Four corners of column i of n, full height.
static void s_column_quad(Vertex* v, int i, int n, CF_Color c)
{
	float x0 = -1.0f + 2.0f * (float)i / (float)n;
	float x1 = -1.0f + 2.0f * (float)(i + 1) / (float)n;
	float xs[4] = { x0, x1, x1, x0 };
	float ys[4] = { -1, -1, 1, 1 };
	for (int k = 0; k < 4; ++k) {
		v[k].x = xs[k];
		v[k].y = ys[k];
		v[k].c[0] = c.r; v[k].c[1] = c.g; v[k].c[2] = c.b; v[k].c[3] = c.a;
	}
}

static void s_read(CF_Canvas canvas, CF_Pixel* px)
{
	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
}

static bool s_near(int channel, float expected)
{
	int d = channel - (int)(expected * 255);
	return d > -8 && d < 8;
}

static bool s_column_is(const CF_Pixel* px, int i, int n, CF_Color c)
{
	int x = (int)(((float)i + 0.5f) * (float)W / (float)n);
	CF_Pixel p = px[(H / 2) * W + x];
	return s_near(p.colors.r, c.r) && s_near(p.colors.g, c.g) && s_near(p.colors.b, c.b);
}

static int s_pass_count()
{
#ifdef CF_WEBGPU
	if (cf_query_backend() == CF_BACKEND_TYPE_WEBGPU) return cf_webgpu_render_pass_count();
#endif
	return 0;
}

// Vertex and index data replaced before each of four draws in one pass. Each version's vertices
// hold the visible quad in one half and an offscreen quad in the other, and its indices pick the
// visible half, so a draw reading a stale version of either buffer misses its column. The last
// version is what the mesh holds in the next frame.
TEST_CASE(test_mesh_updated_between_draws)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Mesh mesh = s_make_color_mesh(8);
	cf_mesh_set_index_buffer(mesh, 6 * (int)sizeof(uint16_t), 16);
	CF_Shader shader = cf_make_shader_from_source(s_color_vs, s_color_fs);
	REQUIRE(shader.id);
	CF_Material material = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	int passes = s_pass_count();
	for (int i = 0; i < COLUMNS; ++i) {
		Vertex verts[8];
		int visible = (i & 1) * 4;
		s_column_quad(verts + visible, i, COLUMNS, s_colors[i]);
		s_column_quad(verts + (4 - visible), 0, COLUMNS, s_colors[i]);
		for (int k = 0; k < 4; ++k) verts[(4 - visible) + k].x += 4.0f;
		uint16_t base = (uint16_t)visible;
		uint16_t indices[6] = { base, (uint16_t)(base + 1), (uint16_t)(base + 2), base, (uint16_t)(base + 2), (uint16_t)(base + 3) };
		cf_mesh_update_vertex_data(mesh, verts, 8);
		cf_mesh_update_index_data(mesh, indices, 6);
		cf_apply_mesh(mesh);
		cf_apply_shader(shader, material);
		cf_draw_elements();
	}
	if (cf_query_backend() == CF_BACKEND_TYPE_WEBGPU) {
		REQUIRE(s_pass_count() - passes == 1);
	}
	cf_app_draw_onto_screen(false);
	s_read(canvas, px);
	for (int i = 0; i < COLUMNS; ++i) REQUIRE(s_column_is(px, i, COLUMNS, s_colors[i]));

	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);
	s_read(canvas, px);
	REQUIRE(s_column_is(px, COLUMNS - 1, COLUMNS, s_colors[COLUMNS - 1]));
	for (int i = 0; i < COLUMNS - 1; ++i) REQUIRE(!s_column_is(px, i, COLUMNS, s_colors[i]));

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(material);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	test_destroy_app();
	return true;
}

// The same for a storage buffer read by the vertex stage, the way CF's own shape draws feed
// their per-draw commands.
TEST_CASE(test_storage_updated_between_draws)
{
	const char* gles = getenv("CF_TEST_GLES");
	if (gles && *gles == '1') return true; // No SSBOs in GLES3 user shaders.
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	static const char* pull_vs =
	"layout (location = 0) in vec2 in_pos;\n"
	"layout (location = 0) out vec4 v_col;\n"
	"struct Inst { vec4 rect; vec4 col; };\n"
	"layout (std430, set = 0, binding = 0) readonly buffer inst_buffer { Inst u_inst[]; };\n"
	"void main() {\n"
	"    Inst inst = u_inst[gl_InstanceIndex];\n"
	"    v_col = inst.col;\n"
	"    gl_Position = vec4(mix(inst.rect.xy, inst.rect.zw, in_pos * 0.5 + 0.5), 0, 1);\n"
	"}\n";
	struct Inst { float rect[4]; float col[4]; };

	Vertex verts[6];
	CF_Mesh mesh = s_make_color_mesh(6);
	{
		float corners[6][2] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, -1 }, { 1, 1 }, { -1, 1 } };
		for (int k = 0; k < 6; ++k) { verts[k] = { }; verts[k].x = corners[k][0]; verts[k].y = corners[k][1]; }
		cf_mesh_update_vertex_data(mesh, verts, 6);
	}
	CF_StorageBufferParams sp = cf_storage_buffer_defaults((int)sizeof(Inst));
	sp.graphics_readable = true;
	CF_StorageBuffer sb = cf_make_storage_buffer(sp);
	CF_Shader shader = cf_make_shader_from_source(pull_vs, s_color_fs);
	REQUIRE(shader.id);
	CF_Material material = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	int passes = s_pass_count();
	for (int i = 0; i < COLUMNS; ++i) {
		CF_Color c = s_colors[i];
		Inst inst = { { -1.0f + 2.0f * i / COLUMNS, -1, -1.0f + 2.0f * (i + 1) / COLUMNS, 1 }, { c.r, c.g, c.b, c.a } };
		cf_update_storage_buffer(sb, &inst, (int)sizeof(inst));
		cf_apply_mesh(mesh);
		cf_apply_shader(shader, material);
		cf_apply_vs_storage_buffers(&sb, 1);
		cf_draw_elements_instanced(1);
	}
	if (cf_query_backend() == CF_BACKEND_TYPE_WEBGPU) {
		REQUIRE(s_pass_count() - passes == 1);
	}
	cf_app_draw_onto_screen(false);
	s_read(canvas, px);
	for (int i = 0; i < COLUMNS; ++i) REQUIRE(s_column_is(px, i, COLUMNS, s_colors[i]));

	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	cf_apply_vs_storage_buffers(&sb, 1);
	cf_draw_elements_instanced(1);
	cf_app_draw_onto_screen(false);
	s_read(canvas, px);
	REQUIRE(s_column_is(px, COLUMNS - 1, COLUMNS, s_colors[COLUMNS - 1]));
	for (int i = 0; i < COLUMNS - 1; ++i) REQUIRE(!s_column_is(px, i, COLUMNS, s_colors[i]));

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(material);
	cf_destroy_shader(shader);
	cf_destroy_storage_buffer(sb);
	cf_destroy_mesh(mesh);
	test_destroy_app();
	return true;
}

// Large updates made between passes: the first reaches a buffer nothing has read yet, the second
// one an earlier pass in the same submission already read, which must keep seeing the first.
TEST_CASE(test_large_mesh_updated_between_passes)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	const int count = 4096; // 96 KiB of vertices, past the backend's large-upload threshold.
	CF_Mesh mesh = s_make_color_mesh(count);
	CF_Shader shader = cf_make_shader_from_source(s_color_vs, s_color_fs);
	REQUIRE(shader.id);
	CF_Material material = cf_make_material();
	CF_Canvas canvases[2] = { cf_make_canvas(cf_canvas_defaults(W, H)), cf_make_canvas(cf_canvas_defaults(W, H)) };
	Vertex* verts = (Vertex*)cf_calloc(count * (int)sizeof(Vertex), 1); // Zeroed tail: degenerate triangles.
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	for (int i = 0; i < 2; ++i) {
		cf_apply_canvas(canvases[i], true);
		s_column_quad(verts, 0, 1, s_colors[i]);
		verts[4] = verts[0];
		verts[5] = verts[2];
		Vertex t = verts[3]; verts[3] = verts[4]; verts[4] = verts[5]; verts[5] = t;
		cf_mesh_update_vertex_data(mesh, verts, count);
		cf_apply_mesh(mesh);
		cf_apply_shader(shader, material);
		cf_draw_elements();
	}
	cf_app_draw_onto_screen(false);
	for (int i = 0; i < 2; ++i) {
		s_read(canvases[i], px);
		REQUIRE(s_column_is(px, 0, 1, s_colors[i]));
	}

	cf_free(px);
	cf_free(verts);
	cf_destroy_canvas(canvases[0]);
	cf_destroy_canvas(canvases[1]);
	cf_destroy_material(material);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	test_destroy_app();
	return true;
}

TEST_SUITE(test_buffer_updates)
{
	RUN_TEST_CASE(test_mesh_updated_between_draws);
	RUN_TEST_CASE(test_storage_updated_between_draws);
	RUN_TEST_CASE(test_large_mesh_updated_between_passes);
}
