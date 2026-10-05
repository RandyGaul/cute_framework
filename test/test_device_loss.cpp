/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>
#include <internal/cute_graphics_internal.h>

using namespace Cute;

// A lost device stays lost and every frame after it is a no-op. The loss kills the app's
// device for good, so this runs on a private app.

#ifdef CF_WEBGPU

#define W 64
#define H 64

static const char* s_vs =
"layout (location = 0) in vec2 in_pos;\n"
"void main() { gl_Position = vec4(in_pos, 0, 1); }\n";

static const char* s_fs =
"layout (location = 0) out vec4 result;\n"
"void main() { result = vec4(1, 0, 0, 1); }\n";

// The loss hook is WebGPU's; other backends skip without making an app. False also means
// headless CI (no display/GPU).
static bool s_make_webgpu_app()
{
	if (!(test_app_options() & CF_APP_OPTIONS_GFX_WEBGPU_BIT)) return false;
	return test_make_private_app(W, H);
}

struct Vertex { float x, y; };

static CF_Mesh s_make_quad()
{
	Vertex verts[6] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, -1 }, { 1, 1 }, { -1, 1 } };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);
	return mesh;
}

// Spins at most a bounded number of polls: a readback that never turns ready must fail the
// test, not hang the suite.
static bool s_readback_settles(CF_Canvas canvas, CF_Pixel* px)
{
	CF_Readback rb = cf_canvas_readback(canvas);
	bool ready = false;
	for (int i = 0; i < 100000 && !ready; ++i) ready = cf_readback_ready(rb);
	if (ready) cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	return ready;
}

static bool s_frame(CF_Canvas canvas, CF_Shader shader, CF_Material material, CF_Mesh mesh, CF_Pixel* px, int frame)
{
	cf_app_update(NULL);

	CF_Pixel pixels[8 * 8] = { };
	CF_Texture tex = cf_make_texture(cf_texture_defaults(8, 8));
	cf_texture_update(tex, pixels, sizeof(pixels));
	CF_Mesh scratch = s_make_quad();

	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	cf_draw_elements();

	cf_draw_circle_fill2(cf_v2(0, 0), 10.0f);
	cf_draw_text("lost", cf_v2(-20, 0), -1);
	cf_draw_box(cf_make_aabb(cf_v2(-5, -5), cf_v2(5, 5)), 1, 0);
	cf_render_to(canvas, frame & 1);
	cf_clear_canvas(canvas);
	bool settled = s_readback_settles(canvas, px);

	cf_app_draw_onto_screen(true);

	cf_destroy_mesh(scratch);
	cf_destroy_texture(tex);
	return settled;
}

TEST_CASE(test_device_loss_frames_are_no_ops)
{
	if (!s_make_webgpu_app()) return true;
	TestPrivateAppGuard app_guard;

	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_Material material = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Mesh mesh = s_make_quad();
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	// A healthy frame first, so the loss lands with uploads and staging in flight.
	REQUIRE(s_frame(canvas, shader, material, mesh, px, 0));
	REQUIRE(!cf_webgpu_device_is_lost());

	// The loss surfaces from whichever call next touches the device, as after a driver reset.
	cf_webgpu_lose_device();
	for (int frame = 1; frame <= 4; ++frame) {
		REQUIRE(s_frame(canvas, shader, material, mesh, px, frame));
		REQUIRE(cf_webgpu_device_is_lost());
	}

	// Resources made after the loss still come back as usable handles.
	CF_Shader late_shader = cf_make_shader_from_source(s_vs, s_fs);
	CF_Canvas late_canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	REQUIRE(late_canvas.id);
	REQUIRE(s_frame(late_canvas, late_shader.id ? late_shader : shader, material, mesh, px, 5));
	cf_gpu_sync();
	REQUIRE(cf_webgpu_device_is_lost());

	if (late_shader.id) cf_destroy_shader(late_shader);
	cf_destroy_canvas(late_canvas);
	cf_free(px);
	cf_destroy_mesh(mesh);
	cf_destroy_canvas(canvas);
	cf_destroy_material(material);
	cf_destroy_shader(shader);
	return true;
}

// The loss lands after work is recorded, so the submission is the first thing to touch the lost
// device. wgpu-native takes a bare pass's submission silently; the next frame reports the loss.
TEST_CASE(test_device_loss_before_submit_bare_pass)
{
	if (!s_make_webgpu_app()) return true;
	TestPrivateAppGuard app_guard;

	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	cf_app_update(NULL);
	cf_apply_canvas(canvas, true);
	cf_webgpu_lose_device();
	cf_gpu_sync();
	cf_app_draw_onto_screen(true);
	cf_app_update(NULL);
	cf_draw_box(cf_make_aabb(cf_v2(-5, -5), cf_v2(5, 5)), 1, 0);
	cf_app_draw_onto_screen(true);
	REQUIRE(cf_webgpu_device_is_lost());
	cf_destroy_canvas(canvas);
	return true;
}

TEST_CASE(test_device_loss_before_submit_uploads)
{
	if (!s_make_webgpu_app()) return true;
	TestPrivateAppGuard app_guard;

	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel pixels[8 * 8] = { };
	CF_Texture tex = cf_make_texture(cf_texture_defaults(8, 8));
	cf_app_update(NULL);
	cf_texture_update(tex, pixels, sizeof(pixels));
	cf_draw_box(cf_make_aabb(cf_v2(-5, -5), cf_v2(5, 5)), 1, 0);
	cf_render_to(canvas, true);
	cf_webgpu_lose_device();
	cf_gpu_sync();
	REQUIRE(cf_webgpu_device_is_lost());
	cf_app_draw_onto_screen(true);
	cf_destroy_texture(tex);
	cf_destroy_canvas(canvas);
	return true;
}

// An upload bigger than any recycled staging chunk makes a fresh one, so creating and mapping
// it is the first thing to touch the lost device.
TEST_CASE(test_device_loss_first_call_is_staging)
{
	if (!s_make_webgpu_app()) return true;
	TestPrivateAppGuard app_guard;

	int size = 1200;
	CF_Texture tex = cf_make_texture(cf_texture_defaults(size, size));
	CF_Pixel* pixels = (CF_Pixel*)cf_calloc(size * size * (int)sizeof(CF_Pixel), 1);
	cf_app_update(NULL);
	cf_webgpu_lose_device();
	cf_texture_update(tex, pixels, size * size * (int)sizeof(CF_Pixel));
	cf_app_draw_onto_screen(true);
	REQUIRE(cf_webgpu_device_is_lost());
	cf_free(pixels);
	cf_destroy_texture(tex);
	return true;
}

// Shader creation checks itself inside an error scope, which keeps the loss away from the
// uncaptured-error callback.
TEST_CASE(test_device_loss_first_call_is_shader)
{
	if (!s_make_webgpu_app()) return true;
	TestPrivateAppGuard app_guard;

	cf_app_update(NULL);
	cf_webgpu_lose_device();
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(cf_webgpu_device_is_lost());
	cf_app_draw_onto_screen(true);
	if (shader.id) cf_destroy_shader(shader);
	return true;
}

#endif // CF_WEBGPU

TEST_SUITE(test_device_loss)
{
#ifdef CF_WEBGPU
	RUN_TEST_CASE(test_device_loss_frames_are_no_ops);
	RUN_TEST_CASE(test_device_loss_before_submit_bare_pass);
	RUN_TEST_CASE(test_device_loss_before_submit_uploads);
	RUN_TEST_CASE(test_device_loss_first_call_is_staging);
	RUN_TEST_CASE(test_device_loss_first_call_is_shader);
#endif
}
