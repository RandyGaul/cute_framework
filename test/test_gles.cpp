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
#include <internal/cute_graphics_internal.h>

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

static bool s_is(CF_Pixel p, int r, int g, int b, int tolerance = 24)
{
	auto near = [=](int a, int b) { int d = a - b; return (d < 0 ? -d : d) <= tolerance; };
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

static CF_Color s_strip_color(int i)
{
	return cf_make_color_rgb((uint8_t)(i * 37), (uint8_t)(i * 101), (uint8_t)(i * 13));
}

#define STRIPS 64

// Many update+draw pairs on one mesh in one frame overrun the 3-slot ring. Each upload must
// still land where only its own draw reads it, without the CPU waiting on the GPU. The small
// mesh orphans its storage on every upload; the large one appends into the active slot.
TEST_CASE(test_mesh_streaming_within_one_frame)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(STRIPS, H));
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_Material material = cf_make_material();
	CF_Pixel* px = (CF_Pixel*)cf_alloc(STRIPS * H * (int)sizeof(CF_Pixel));
	cf_canvas_set_clear_color(canvas, cf_color_black());

	int capacities[2] = { 6, 6 * STRIPS };
	for (int c = 0; c < 2; ++c) {
		CF_Mesh mesh = s_make_color_mesh(capacities[c]);
		int waits = cf_gles_fence_wait_count();

		cf_app_update(NULL);
		cf_apply_canvas(canvas, true);
		for (int i = 0; i < STRIPS; ++i) {
			Vertex v[6];
			float x0 = -1.0f + 2.0f * i / STRIPS;
			s_fill_strip(v, x0, x0 + 2.0f / STRIPS, s_strip_color(i));
			cf_mesh_update_vertex_data(mesh, v, 6);
			cf_apply_mesh(mesh);
			cf_apply_shader(shader, material);
			cf_draw_elements();
		}
		cf_app_draw_onto_screen(false);
		REQUIRE(cf_gles_fence_wait_count() == waits);

		CF_Readback rb = cf_canvas_readback(canvas);
		while (!cf_readback_ready(rb)) {}
		cf_readback_data(rb, px, STRIPS * H * (int)sizeof(CF_Pixel));
		cf_destroy_readback(rb);
		for (int i = 0; i < STRIPS; ++i) {
			CF_Color e = s_strip_color(i);
			REQUIRE(s_is(px[(H / 2) * STRIPS + i], (int)(e.r * 255 + 0.5f), (int)(e.g * 255 + 0.5f), (int)(e.b * 255 + 0.5f), 2));
		}
		cf_destroy_mesh(mesh);
	}

	cf_free(px);
	cf_destroy_material(material);
	cf_destroy_shader(shader);
	cf_destroy_canvas(canvas);
	test_destroy_app();
	return true;
}

static const char* s_textured_fs =
"layout (location = 0) in vec4 v_col;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_tex;\n"
"void main() { result = v_col * texture(u_tex, vec2(0.5)); }\n";

// On the web a full texture ring reuses its busy slot instead of waiting on a fence, which
// would yield to the browser mid-frame. This forces that path on desktop: each upload must
// still reach only the draws issued after it.
TEST_CASE(test_texture_streaming_reuses_busy_slots)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	if (cf_query_backend() != CF_BACKEND_TYPE_GLES3) return true;

	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(STRIPS, H));
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_textured_fs);
	REQUIRE(shader.id);
	CF_TextureParams tp = cf_texture_defaults(1, 1);
	tp.stream = true;
	CF_Texture texture = cf_make_texture(tp);
	CF_Material material = cf_make_material();
	cf_material_set_texture_fs(material, "u_tex", texture);
	CF_Mesh mesh = s_make_color_mesh(6 * STRIPS);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(STRIPS * H * (int)sizeof(CF_Pixel));
	cf_canvas_set_clear_color(canvas, cf_color_black());

	cf_gles_reuse_busy_ring_slots(true);
	int waits = cf_gles_fence_wait_count();
	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	for (int i = 0; i < STRIPS; ++i) {
		CF_Color c = s_strip_color(i);
		CF_Pixel p = cf_make_pixel_rgb((uint8_t)(c.r * 255 + 0.5f), (uint8_t)(c.g * 255 + 0.5f), (uint8_t)(c.b * 255 + 0.5f));
		cf_texture_update(texture, &p, sizeof(p));
		Vertex v[6];
		float x0 = -1.0f + 2.0f * i / STRIPS;
		s_fill_strip(v, x0, x0 + 2.0f / STRIPS, cf_color_white());
		cf_mesh_update_vertex_data(mesh, v, 6);
		cf_apply_mesh(mesh);
		cf_apply_shader(shader, material);
		cf_draw_elements();
	}
	cf_app_draw_onto_screen(false);
	int new_waits = cf_gles_fence_wait_count() - waits;
	cf_gles_reuse_busy_ring_slots(false);
	REQUIRE(new_waits == 0);

	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, STRIPS * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	for (int i = 0; i < STRIPS; ++i) {
		CF_Color e = s_strip_color(i);
		REQUIRE(s_is(px[(H / 2) * STRIPS + i], (int)(e.r * 255 + 0.5f), (int)(e.g * 255 + 0.5f), (int)(e.b * 255 + 0.5f), 2));
	}

	cf_free(px);
	cf_destroy_mesh(mesh);
	cf_destroy_material(material);
	cf_destroy_texture(texture);
	cf_destroy_shader(shader);
	cf_destroy_canvas(canvas);
	test_destroy_app();
	return true;
}

enum ReadbackKind { READBACK_UNORM8, READBACK_HALF, READBACK_FLOAT };

struct ReadbackCase
{
	CF_PixelFormat format;
	int channels;
	ReadbackKind kind;
};

static const ReadbackCase s_readback_cases[] = {
	{ CF_PIXEL_FORMAT_R8_UNORM, 1, READBACK_UNORM8 },
	{ CF_PIXEL_FORMAT_R8G8_UNORM, 2, READBACK_UNORM8 },
	{ CF_PIXEL_FORMAT_R16_FLOAT, 1, READBACK_HALF },
	{ CF_PIXEL_FORMAT_R16G16_FLOAT, 2, READBACK_HALF },
	{ CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, 4, READBACK_HALF },
	{ CF_PIXEL_FORMAT_R32_FLOAT, 1, READBACK_FLOAT },
	{ CF_PIXEL_FORMAT_R32G32_FLOAT, 2, READBACK_FLOAT },
};

// Normal numbers and zero only, which is all the test values need.
static float s_float_from_half(uint16_t h)
{
	uint32_t sign = (uint32_t)(h & 0x8000) << 16;
	uint32_t exponent = (h >> 10) & 0x1f;
	uint32_t mantissa = h & 0x3ff;
	uint32_t x = exponent ? sign | ((exponent + 112) << 23) | (mantissa << 13) : sign;
	float f;
	CF_MEMCPY(&f, &x, sizeof f);
	return f;
}

#define RB_W 8
#define RB_H 8

// Clears each format to known values and reads it back in the target's own layout.
static bool s_check_readback_formats()
{
	const float float_values[4] = { 0.25f, 1.5f, -2.0f, 4.0f };
	const float unorm_values[4] = { 0.25f, 0.5f, 0.75f, 1.0f };
	const int unorm_bytes[4] = { 64, 128, 191, 255 };
	for (int i = 0; i < (int)CF_ARRAY_SIZE(s_readback_cases); ++i) {
		ReadbackCase rc = s_readback_cases[i];
		if (!cf_query_pixel_format(rc.format, CF_PIXELFORMAT_OP_RENDER_TARGET)) continue;
		CF_CanvasParams params = cf_canvas_defaults(RB_W, RB_H);
		params.target.pixel_format = rc.format;
		CF_Canvas canvas = cf_make_canvas(params);
		REQUIRE(canvas.id);
		const float* v = rc.kind == READBACK_UNORM8 ? unorm_values : float_values;
		CF_Color clear = { v[0], v[1], v[2], v[3] };
		cf_canvas_set_clear_color(canvas, clear);
		cf_app_update(NULL);
		cf_clear_canvas(canvas);
		cf_app_draw_onto_screen(false);

		int component_size = rc.kind == READBACK_UNORM8 ? 1 : rc.kind == READBACK_HALF ? 2 : 4;
		int size = RB_W * RB_H * rc.channels * component_size;
		CF_Readback rb = cf_canvas_readback(canvas);
		REQUIRE(rb.id);
		while (!cf_readback_ready(rb)) {}
		REQUIRE(cf_readback_size(rb) == size);
		uint8_t* data = (uint8_t*)cf_alloc(size);
		cf_readback_data(rb, data, size);
		cf_destroy_readback(rb);

		int last = RB_W * RB_H - 1;
		bool ok = true;
		for (int texel = 0; texel <= last; texel += last) {
			for (int c = 0; c < rc.channels; ++c) {
				int k = texel * rc.channels + c;
				if (rc.kind == READBACK_UNORM8) ok = ok && cf_abs(data[k] - unorm_bytes[c]) <= 1;
				else if (rc.kind == READBACK_HALF) ok = ok && s_float_from_half(((uint16_t*)data)[k]) == v[c];
				else ok = ok && ((float*)data)[k] == v[c];
			}
		}
		if (!ok) printf("readback mismatch: %s\n", cf_pixel_format_to_string(rc.format));
		cf_free(data);
		cf_destroy_canvas(canvas);
		REQUIRE(ok);
	}
	return true;
}

TEST_CASE(test_readback_formats)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	REQUIRE(s_check_readback_formats());
	test_destroy_app();
	return true;
}

// A driver may accept only the guaranteed pair of each buffer class for glReadPixels,
// rejecting a target's own layout such as RED/HALF_FLOAT.
TEST_CASE(test_readback_formats_guaranteed_pairs_only)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	if (cf_query_backend() != CF_BACKEND_TYPE_GLES3) return true;
	cf_gles_only_guaranteed_read_pairs(true);
	bool ok = s_check_readback_formats();
	cf_gles_only_guaranteed_read_pairs(false);
	REQUIRE(ok);
	test_destroy_app();
	return true;
}

TEST_SUITE(test_gles)
{
	RUN_TEST_CASE(test_scissor_box_survives_unscissored_pass);
	RUN_TEST_CASE(test_mesh_streaming_within_one_frame);
	RUN_TEST_CASE(test_texture_streaming_reuses_busy_slots);
	RUN_TEST_CASE(test_readback_formats);
	RUN_TEST_CASE(test_readback_formats_guaranteed_pairs_only);
}
