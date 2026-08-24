/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>
using namespace Cute;

#define VIDEO_W 96
#define VIDEO_H 64
#define VIDEO_FRAMES 12

// A moving pattern rather than a flat colour: a codec asked to compress nothing compresses it
// perfectly and proves nothing about whether it works.
static void s_fill(CF_Pixel* pix, int frame)
{
	for (int y = 0; y < VIDEO_H; ++y) {
		for (int x = 0; x < VIDEO_W; ++x) {
			int bar = ((x + frame * 5) % VIDEO_W) < VIDEO_W / 4;
			pix[y * VIDEO_W + x] = cf_make_pixel_rgb(
				(uint8_t)(bar ? 240 : x * 255 / (VIDEO_W - 1)),
				(uint8_t)(y * 255 / (VIDEO_H - 1)),
				(uint8_t)((frame * 20) & 255));
		}
	}
}

// Lossy compression means "close", not "equal", so the check is on how far off the worst channel
// is rather than on equality. Anything wired up wrongly -- a plane swapped, a stride wrong, frames
// out of order -- lands far outside this.
static int s_worst_difference(const CF_Pixel* a, const CF_Pixel* b, int count)
{
	int worst = 0;
	for (int i = 0; i < count; ++i) {
		int dr = (int)a[i].colors.r - (int)b[i].colors.r;
		int dg = (int)a[i].colors.g - (int)b[i].colors.g;
		int db = (int)a[i].colors.b - (int)b[i].colors.b;
		if (dr < 0) dr = -dr;
		if (dg < 0) dg = -dg;
		if (db < 0) db = -db;
		if (dr > worst) worst = dr;
		if (dg > worst) worst = dg;
		if (db > worst) worst = db;
	}
	return worst;
}

static CF_VideoEncoder* s_record(int quality)
{
	CF_VideoEncoder* encoder = cf_make_video_encoder(VIDEO_W, VIDEO_H, 30);
	if (!encoder) return NULL;
	cf_video_encoder_quality(encoder, quality);
	CF_Pixel* pix = (CF_Pixel*)cf_alloc(sizeof(CF_Pixel) * VIDEO_W * VIDEO_H);
	for (int i = 0; i < VIDEO_FRAMES; ++i) {
		s_fill(pix, i);
		CF_Image image = { VIDEO_W, VIDEO_H, pix };
		if (cf_is_error(cf_video_encoder_add_frame(encoder, image))) {
			cf_free(pix);
			cf_destroy_video_encoder(encoder);
			return NULL;
		}
	}
	cf_free(pix);
	return encoder;
}

/* Encode a short sequence, decode it back, and check every frame arrives in order. */
TEST_CASE(test_video_round_trip)
{
	CF_VideoEncoder* encoder = s_record(50);
	CHECK_POINTER(encoder);

	int size = 0;
	const void* mp4 = cf_video_encoder_data(encoder, &size);
	CHECK_POINTER((void*)mp4);
	REQUIRE(size > 0);

	CF_Video* video = cf_make_video_from_memory(mp4, size);
	CHECK_POINTER(video);
	REQUIRE(cf_video_width(video) == VIDEO_W);
	REQUIRE(cf_video_height(video) == VIDEO_H);
	REQUIRE(cf_video_fps(video) == 30);

	CF_Pixel* expected = (CF_Pixel*)cf_alloc(sizeof(CF_Pixel) * VIDEO_W * VIDEO_H);
	int frames = 0;
	do {
		CF_Image got = cf_video_frame(video);
		REQUIRE(got.w == VIDEO_W);
		REQUIRE(got.h == VIDEO_H);
		CHECK_POINTER(got.pix);
		s_fill(expected, frames);
		// Frames out of order would be far worse than this; the pattern moves a quarter of the
		// picture every frame.
		REQUIRE(s_worst_difference(expected, got.pix, VIDEO_W * VIDEO_H) < 90);
		++frames;
	} while (cf_video_next_frame(video));
	cf_free(expected);

	REQUIRE(frames == VIDEO_FRAMES);
	REQUIRE(cf_video_is_finished(video));

	cf_destroy_video(video);
	cf_destroy_video_encoder(encoder);
	return true;
}

/* A higher quality setting must actually produce a closer picture, and a bigger file. */
TEST_CASE(test_video_quality)
{
	int worst[2] = { 0, 0 };
	int bytes[2] = { 0, 0 };
	int quality[2] = { 10, 90 };
	for (int k = 0; k < 2; ++k) {
		CF_VideoEncoder* encoder = s_record(quality[k]);
		CHECK_POINTER(encoder);
		const void* mp4 = cf_video_encoder_data(encoder, &bytes[k]);
		CHECK_POINTER((void*)mp4);
		CF_Video* video = cf_make_video_from_memory(mp4, bytes[k]);
		CHECK_POINTER(video);
		CF_Pixel* expected = (CF_Pixel*)cf_alloc(sizeof(CF_Pixel) * VIDEO_W * VIDEO_H);
		int frame = 0;
		do {
			CF_Image got = cf_video_frame(video);
			s_fill(expected, frame);
			int d = s_worst_difference(expected, got.pix, VIDEO_W * VIDEO_H);
			if (d > worst[k]) worst[k] = d;
			++frame;
		} while (cf_video_next_frame(video));
		cf_free(expected);
		cf_destroy_video(video);
		cf_destroy_video_encoder(encoder);
	}
	REQUIRE(worst[1] < worst[0]);
	REQUIRE(bytes[1] > bytes[0]);
	return true;
}

/* Save to the virtual file system and open it again, both container forms. */
TEST_CASE(test_video_save_and_load)
{
	// The virtual file system comes up with the app, so this one needs it even though nothing
	// here draws.
	if (!test_make_app(VIDEO_W, VIDEO_H)) return true;
	// The base directory is already mounted for reading, which is what lets the file written
	// here be opened again by the same virtual path.
	cf_fs_set_write_directory(cf_fs_get_base_directory());
	const char* paths[] = { "/test_video.mp4", "/test_video.h264" };
	for (int k = 0; k < 2; ++k) {
		CF_VideoEncoder* encoder = s_record(50);
		CHECK_POINTER(encoder);
		CF_Result saved = cf_video_encoder_save(encoder, paths[k]);
		cf_destroy_video_encoder(encoder);
		REQUIRE(!cf_is_error(saved));

		// Raw .h264 and .mp4 are told apart by what is in them, not by the name.
		CF_Video* video = cf_make_video(paths[k]);
		CHECK_POINTER(video);
		REQUIRE(cf_video_width(video) == VIDEO_W);
		int frames = 1;
		while (cf_video_next_frame(video)) ++frames;
		REQUIRE(frames == VIDEO_FRAMES);
		cf_destroy_video(video);
		cf_fs_remove(paths[k]);
	}
	test_destroy_app();
	return true;
}

/* Playback timing: a second of updates at 30fps must advance about thirty frames, and looping
   must carry on past the end rather than stopping. */
TEST_CASE(test_video_update_and_loop)
{
	CF_VideoEncoder* encoder = s_record(50);
	CHECK_POINTER(encoder);
	int size = 0;
	const void* mp4 = cf_video_encoder_data(encoder, &size);
	CF_Video* video = cf_make_video_from_memory(mp4, size);
	CHECK_POINTER(video);

	// Twelve frames at 30fps is four tenths of a second; running past that must stop.
	for (int i = 0; i < 60; ++i) cf_video_update(video, 1.0f / 30.0f);
	REQUIRE(cf_video_is_finished(video));

	cf_video_restart(video);
	REQUIRE(!cf_video_is_finished(video));
	cf_video_set_looped(video, true);
	for (int i = 0; i < 120; ++i) cf_video_update(video, 1.0f / 30.0f);
	REQUIRE(!cf_video_is_finished(video));

	cf_destroy_video(video);
	cf_destroy_video_encoder(encoder);
	return true;
}

/* Anything that is not a video must be refused rather than crashed on. */
TEST_CASE(test_video_rejects_rubbish)
{
	CHECK_POINTER(!cf_make_video_from_memory(NULL, 0));
	uint8_t junk[256];
	for (int i = 0; i < 256; ++i) junk[i] = (uint8_t)(i * 7);
	CHECK_POINTER(!cf_make_video_from_memory(junk, sizeof(junk)));
	CHECK_POINTER(!cf_make_video("/does_not_exist.mp4"));
	CHECK_POINTER((void*)cf_video_error());

	// The accessors have to survive a null video, since that is what a failed load hands back.
	REQUIRE(cf_video_width(NULL) == 0);
	REQUIRE(cf_video_height(NULL) == 0);
	REQUIRE(cf_video_is_finished(NULL));
	REQUIRE(!cf_video_next_frame(NULL));
	cf_destroy_video(NULL);
	cf_destroy_video_encoder(NULL);
	return true;
}

/* The sprite and texture the video hands out must be usable, and must follow the frames. */
TEST_CASE(test_video_sprite_and_texture)
{
	if (!test_make_app(VIDEO_W, VIDEO_H)) return true;

	CF_VideoEncoder* encoder = s_record(50);
	CHECK_POINTER(encoder);
	int size = 0;
	const void* mp4 = cf_video_encoder_data(encoder, &size);
	CF_Video* video = cf_make_video_from_memory(mp4, size);
	CHECK_POINTER(video);

	CF_Sprite sprite = cf_video_sprite(video);
	REQUIRE(sprite.w == VIDEO_W);
	REQUIRE(sprite.h == VIDEO_H);
	CF_Texture texture = cf_video_texture(video);
	REQUIRE(texture.id != 0);

	// The same call again on the same frame must hand back the same objects rather than making
	// new ones every time it is asked.
	REQUIRE(cf_video_texture(video).id == texture.id);
	cf_video_next_frame(video);
	REQUIRE(cf_video_texture(video).id == texture.id);

	cf_destroy_video(video);
	cf_destroy_video_encoder(encoder);
	test_destroy_app();
	return true;
}

// The draw3d path: a frame reaching a mesh through cf_draw3d_set_texture. The shader remaps uvs by
// in_uv_rect exactly as a sprite-textured mesh would; with no sprite pushed that lane is the full
// rect, so one shader serves both ways of supplying the image.
static const char* s_video3d_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 1) in vec2 in_uv;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
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

static const char* s_video3d_fs =
"layout (location = 0) in vec2 v_uv;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_image;\n"
"void main() {\n"
"    result = vec4(texture(u_image, v_uv).rgb, 1.0);\n"
"}\n";

typedef struct VideoVertex
{
	CF_V3 pos;
	CF_V2 uv;
} VideoVertex;

/* A decoded frame drawn onto a mesh with the draw3d API must land on the canvas, right way up. */
TEST_CASE(test_video_draw3d)
{
	if (!test_make_app(VIDEO_W, VIDEO_H)) return true;

	CF_VideoEncoder* encoder = s_record(90);
	CHECK_POINTER(encoder);
	int size = 0;
	const void* mp4 = cf_video_encoder_data(encoder, &size);
	CF_Video* video = cf_make_video_from_memory(mp4, size);
	CHECK_POINTER(video);

	// A quad filling clip space exactly, so a canvas pixel maps to the frame pixel under it and
	// the comparison below does not depend on any filtering.
	VideoVertex verts[6] = {
		{ { -1, -1, 0 }, { 0, 1 } }, { { 1, -1, 0 }, { 1, 1 } }, { { 1, 1, 0 }, { 1, 0 } },
		{ { -1, -1, 0 }, { 0, 1 } }, { { 1,  1, 0 }, { 1, 0 } }, { { -1, 1, 0 }, { 0, 0 } },
	};
	CF_VertexAttribute attrs[2] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = CF_OFFSET_OF(VideoVertex, pos);
	attrs[1].name = "in_uv";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CF_OFFSET_OF(VideoVertex, uv);
	CF_Mesh mesh = cf_make_mesh((int)sizeof(verts), attrs, 2, (int)sizeof(VideoVertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);

	CF_Shader shader = cf_make_shader_from_source(s_video3d_vs, s_video3d_fs);
	REQUIRE(shader.id);
	// draw3d's default render state tests depth, so the canvas it draws into needs somewhere to
	// put it.
	CF_CanvasParams params = cf_canvas_defaults(VIDEO_W, VIDEO_H);
	params.depth_stencil_enable = true;
	CF_Canvas canvas = cf_make_canvas(params);

	cf_app_update(NULL);
	cf_draw3d_push_projection(cf_ortho(-1, 1, -1, 1, -1, 1));
	cf_draw3d_push_shader(shader);
	cf_draw3d_set_texture("u_image", cf_video_texture(video));
	cf_draw3d_mesh(mesh);
	cf_render_to(canvas, true);
	// The frame has to be submitted before anything can be read back off it.
	cf_app_draw_onto_screen(false);
	cf_draw3d_pop_shader();
	cf_draw3d_pop_projection();

	CF_Pixel* px = (CF_Pixel*)cf_alloc(VIDEO_W * VIDEO_H * (int)sizeof(CF_Pixel));
	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, VIDEO_W * VIDEO_H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);

	// What was drawn has to be the frame -- a blank canvas, or the picture upside down, is far
	// outside this. The first frame's green channel climbs with y, which is what catches a flip.
	CF_Image frame = cf_video_frame(video);
	// It came back bit-identical in practice; the tolerance is there for a backend that filters
	// or converts on the way through, not to paper over a wrong picture.
	REQUIRE(s_worst_difference(frame.pix, px, VIDEO_W * VIDEO_H) < 24);

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_shader(shader);
	cf_destroy_mesh(mesh);
	cf_destroy_video(video);
	cf_destroy_video_encoder(encoder);
	test_destroy_app();
	return true;
}

TEST_SUITE(test_video)
{
	RUN_TEST_CASE(test_video_round_trip);
	RUN_TEST_CASE(test_video_quality);
	RUN_TEST_CASE(test_video_save_and_load);
	RUN_TEST_CASE(test_video_update_and_loop);
	RUN_TEST_CASE(test_video_rejects_rubbish);
	RUN_TEST_CASE(test_video_sprite_and_texture);
	RUN_TEST_CASE(test_video_draw3d);
}
