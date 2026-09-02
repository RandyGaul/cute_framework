/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>
#include <cute/cute_h264.h>
#include <stdlib.h>
#include <string.h>
using namespace Cute;

#define VIDEO_W 96
#define VIDEO_H 64
#define VIDEO_FRAMES 12

// A moving pattern rather than a flat colour: a codec asked to compress nothing compresses it
// perfectly and proves nothing about whether it works.
static void s_fill_size(CF_Pixel* pix, int w, int h, int frame)
{
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			int bar = ((x + frame * 5) % w) < w / 4;
			pix[y * w + x] = cf_make_pixel_rgb(
				(uint8_t)(bar ? 240 : x * 255 / (w - 1)),
				(uint8_t)(y * 255 / (h - 1)),
				(uint8_t)((frame * 20) & 255));
		}
	}
}

static void s_fill(CF_Pixel* pix, int frame) { s_fill_size(pix, VIDEO_W, VIDEO_H, frame); }

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

static CF_VideoEncoder* s_record_n(int quality, int frames)
{
	CF_VideoEncoder* encoder = cf_make_video_encoder(VIDEO_W, VIDEO_H, 30);
	if (!encoder) return NULL;
	cf_video_encoder_set_quality(encoder, quality);
	CF_Pixel* pix = (CF_Pixel*)cf_alloc(sizeof(CF_Pixel) * VIDEO_W * VIDEO_H);
	for (int i = 0; i < frames; ++i) {
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

static CF_VideoEncoder* s_record(int quality) { return s_record_n(quality, VIDEO_FRAMES); }

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

/* Seeking: land on any frame, forwards or backwards, and get that frame's pixels. The stream is
   long enough to span several keyframes, so seeks start mid-file rather than always at the top. */
TEST_CASE(test_video_seek)
{
	// 150 frames at 30fps is five seconds, and the encoder writes a keyframe every two.
	const int FRAMES = 150;
	CF_VideoEncoder* encoder = s_record_n(50, FRAMES);
	CHECK_POINTER(encoder);
	int size = 0;
	const void* mp4 = cf_video_encoder_data(encoder, &size);
	CF_Video* video = cf_make_video_from_memory(mp4, size);
	CHECK_POINTER(video);

	REQUIRE(cf_video_frame_count(video) == FRAMES);
	REQUIRE(cf_video_frame_index(video) == 0);
	float duration = cf_video_duration(video);
	REQUIRE(duration > 4.99f && duration < 5.01f);

	// Forward past a keyframe, backward, dead on a keyframe boundary, and the very ends. Each
	// landing is checked against the pattern that frame was encoded from.
	CF_Pixel* expected = (CF_Pixel*)cf_alloc(sizeof(CF_Pixel) * VIDEO_W * VIDEO_H);
	int targets[] = { 120, 61, 45, 60, 0, FRAMES - 1 };
	for (int k = 0; k < (int)(sizeof(targets) / sizeof(targets[0])); ++k) {
		REQUIRE(cf_video_seek(video, targets[k]));
		REQUIRE(cf_video_frame_index(video) == targets[k]);
		CF_Image got = cf_video_frame(video);
		CHECK_POINTER(got.pix);
		s_fill(expected, targets[k]);
		// This check is what surfaced the keyframe-flush encoder bug: it compares against the
		// SOURCE pattern, which conformance sweeps -- output vs its own decode -- cannot see.
		REQUIRE(s_worst_difference(expected, got.pix, VIDEO_W * VIDEO_H) < 90);
	}
	cf_free(expected);

	// Out of range refuses and moves nothing.
	REQUIRE(!cf_video_seek(video, FRAMES));
	REQUIRE(!cf_video_seek(video, -1));
	REQUIRE(cf_video_frame_index(video) == FRAMES - 1);

	// Playback carries on from wherever the seek landed.
	REQUIRE(cf_video_seek(video, FRAMES - 2));
	REQUIRE(cf_video_next_frame(video));
	REQUIRE(cf_video_frame_index(video) == FRAMES - 1);
	REQUIRE(!cf_video_next_frame(video));
	REQUIRE(cf_video_is_finished(video));

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
	CF_Canvas no_canvas = { 0 };
	REQUIRE(cf_video_encoder_update(NULL, no_canvas, 1.0f / 30.0f) == 0);
	cf_destroy_video(NULL);
	cf_destroy_video_encoder(NULL);
	return true;
}

/* Recording a canvas with cf_video_encoder_update: captures pace against dt, cross back from the
   GPU asynchronously, and the file must still carry every owed frame, in order. */
TEST_CASE(test_video_record_canvas)
{
	if (!test_make_app(VIDEO_W, VIDEO_H)) return true;

	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(VIDEO_W, VIDEO_H));
	CF_VideoEncoder* encoder = cf_make_video_encoder(VIDEO_W, VIDEO_H, 30);
	CHECK_POINTER(encoder);

	// Twelve frames at exactly the recording rate, each a flat shade climbing with i so the
	// decode below can check the frames arrived in order. Flat is fine here: this test is about
	// the capture plumbing, not the codec, which test_video_round_trip already covers.
	for (int i = 0; i < 12; ++i) {
		cf_app_update(NULL);
		cf_draw_push_color(cf_make_color_rgb_f((float)i / 15.0f, 0.25f, 0.5f));
		cf_draw_circle_fill(cf_make_circle(cf_v2(0, 0), (float)(VIDEO_W + VIDEO_H)));
		cf_draw_pop_color();
		cf_render_to(canvas, true);
		// Captures come after submission, or they would race the render and see stale pixels.
		cf_app_draw_onto_screen(false);
		cf_video_encoder_update(encoder, canvas, 1.0f / 30.0f);
	}
	// One more canvas handed over with three and a half frames of time owed: the capture must be
	// encoded three times over to keep the recording true to the clock.
	cf_app_update(NULL);
	cf_draw_push_color(cf_make_color_rgb_f(12.0f / 15.0f, 0.25f, 0.5f));
	cf_draw_circle_fill(cf_make_circle(cf_v2(0, 0), (float)(VIDEO_W + VIDEO_H)));
	cf_draw_pop_color();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);
	cf_video_encoder_update(encoder, canvas, 3.5f / 30.0f);

	// Data drains the captures still in flight, so nothing above is lost.
	int size = 0;
	const void* mp4 = cf_video_encoder_data(encoder, &size);
	CHECK_POINTER((void*)mp4);

	CF_Video* video = cf_make_video_from_memory(mp4, size);
	CHECK_POINTER(video);
	REQUIRE(cf_video_width(video) == VIDEO_W);
	int frames = 0;
	do {
		CF_Image got = cf_video_frame(video);
		CHECK_POINTER(got.pix);
		// The shade this frame should carry, remembering the last capture plays three times.
		int expect = (int)((frames < 12 ? frames : 12) / 15.0f * 255.0f);
		int center = (VIDEO_H / 2) * VIDEO_W + VIDEO_W / 2;
		int dr = (int)got.pix[center].colors.r - expect;
		if (dr < 0) dr = -dr;
		if (dr >= 32) printf("frame %d: expect r=%d got r=%d g=%d b=%d a=%d\n", frames, expect,
			got.pix[center].colors.r, got.pix[center].colors.g, got.pix[center].colors.b, got.pix[center].colors.a);
		REQUIRE(dr < 32);
		++frames;
	} while (cf_video_next_frame(video));
	REQUIRE(frames == 15);

	cf_destroy_video(video);
	cf_destroy_video_encoder(encoder);
	cf_destroy_canvas(canvas);
	test_destroy_app();
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

//--------------------------------------------------------------------------------------------------
// The presentation timeline. An MP4 carries two orders: the one pictures are decoded in, and the
// one they are shown in. Players follow the second, so it is the second that has to be right --
// and none of the round trips above can see it, because the decoder here orders pictures by the
// counts inside the stream and never reads the container's table. These tests read the table.

static uint32_t s_be32(const uint8_t* p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

// Finds a box by walking the tree rather than scanning for its tag, so that picture data which
// happens to spell one cannot be mistaken for it. Returns the payload, or null.
static const uint8_t* s_find_box(const uint8_t* p, const uint8_t* end, const char* tag, int* out_len)
{
	static const char* containers[] = { "moov", "trak", "mdia", "minf", "stbl", "edts" };
	while (p + 8 <= end) {
		uint32_t size = s_be32(p);
		if (size < 8 || size > (uint32_t)(end - p)) return NULL;
		if (!memcmp(p + 4, tag, 4)) { *out_len = (int)size - 8; return p + 8; }
		for (int i = 0; i < (int)(sizeof(containers) / sizeof(containers[0])); ++i) {
			if (!memcmp(p + 4, containers[i], 4)) {
				const uint8_t* found = s_find_box(p + 8, p + size, tag, out_len);
				if (found) return found;
			}
		}
		p += size;
	}
	return NULL;
}

static int s_int_cmp(const void* a, const void* b) { return *(const int*)a - *(const int*)b; }

// Reads the timeline back out of the container: when each picture is shown, from its decode time
// (stts), its composition offset (ctts) and the edit that trims off the reorder lead (elst). Every
// picture must land on its own slot and the slots must run 0, 1, 2 ... with no gaps: a shared slot
// plays the clip fast, a skipped one plays it slow. Returns the picture count, or -1 with the
// reason printed. *out_lead is how far display runs ahead of decode, in frames.
static int s_check_timeline(const void* mp4, int size, int* out_lead)
{
	const uint8_t* p = (const uint8_t*)mp4;
	const uint8_t* end = p + size;
	int len = 0;
	const uint8_t* stts = s_find_box(p, end, "stts", &len);
	if (!stts || len < 16 || s_be32(stts + 4) != 1) { printf("stts missing, or not a single run\n"); return -1; }
	int count = (int)s_be32(stts + 8);
	int delta = (int)s_be32(stts + 12);
	if (count <= 0 || delta <= 0) { printf("stts is empty\n"); return -1; }

	// A version 0 edit list: entry count, then segment duration, media time, rate.
	int lead = 0;
	const uint8_t* elst = s_find_box(p, end, "elst", &len);
	if (elst && len >= 20) lead = (int)s_be32(elst + 12) / delta;

	int* pts = (int*)cf_alloc(sizeof(int) * (size_t)count);
	for (int i = 0; i < count; ++i) pts[i] = (i - lead) * delta;
	const uint8_t* ctts = s_find_box(p, end, "ctts", &len);
	if (ctts && len >= 8) {
		int entries = (int)s_be32(ctts + 4);
		for (int e = 0, i = 0; e < entries && i < count; ++e) {
			int run = (int)s_be32(ctts + 8 + e * 8);
			int offset = (int)s_be32(ctts + 12 + e * 8);
			for (int k = 0; k < run && i < count; ++k) pts[i++] += offset;
		}
	}
	qsort(pts, (size_t)count, sizeof(int), s_int_cmp);
	int ok = 1;
	for (int i = 0; i < count && ok; ++i) {
		if (pts[i] != i * delta) {
			printf("slot %d holds pts %d, expected %d (%d pictures, lead %d)\n", i, pts[i], i * delta, count, lead);
			ok = 0;
		}
	}
	cf_free(pts);
	if (out_lead) *out_lead = lead;
	return ok ? count : -1;
}

#if defined(CF_STATIC)
// A hand-made Annex-B stream whose slice headers carry exactly what the muxer reads -- the frame
// number and the picture order count -- and nothing a decoder could use. That is the point: the
// order counts can be whatever the test says, which no encoder here would write.
typedef struct s_annexb_t { uint8_t data[2048]; int len; uint8_t bits[32]; int nbits; } s_annexb_t;

static void s_put(s_annexb_t* s, uint32_t v, int n)
{
	while (n--) {
		if ((v >> n) & 1) s->bits[s->nbits >> 3] |= (uint8_t)(0x80 >> (s->nbits & 7));
		++s->nbits;
	}
}

// Unsigned exp-golomb: v+1 in binary, led in by one zero for every bit of it past the first.
static void s_put_ue(s_annexb_t* s, uint32_t v)
{
	int n = 0;
	while (((v + 1) >> n) > 1) ++n;
	s_put(s, 0, n);
	s_put(s, v + 1, n + 1);
}

// Closes the bits written so far into one unit with the given header byte, after a start code.
static void s_nal(s_annexb_t* s, uint8_t header, int pad)
{
	s_put(s, 1, 1);                                  // rbsp stop bit
	const uint8_t start[4] = { 0, 0, 0, 1 };
	memcpy(s->data + s->len, start, 4); s->len += 4;
	s->data[s->len++] = header;
	int bytes = (s->nbits + 7) / 8;
	memcpy(s->data + s->len, s->bits, (size_t)bytes); s->len += bytes;
	// Something to stand in for the picture, on bytes the start code scanner cannot mistake.
	for (int i = 0; i < pad; ++i) s->data[s->len++] = 0xAA;
	memset(s->bits, 0, sizeof(s->bits)); s->nbits = 0;
}

static void s_parameter_sets(s_annexb_t* s)
{
	s_put(s, 77, 8);   // profile_idc: main, so nothing about chroma comes before the order count fields
	s_put(s, 0, 8);    // constraint flags
	s_put(s, 30, 8);   // level_idc
	s_put_ue(s, 0);    // seq_parameter_set_id
	s_put_ue(s, 0);    // log2_max_frame_num_minus4: four bits of frame number
	s_put_ue(s, 0);    // pic_order_cnt_type: the count rides in every slice header
	s_put_ue(s, 2);    // log2_max_pic_order_cnt_lsb_minus4: six bits, as VideoToolbox writes
	s_nal(s, 0x67, 0);
	s_put_ue(s, 0);    // pic_parameter_set_id
	s_put_ue(s, 0);    // seq_parameter_set_id
	s_nal(s, 0x68, 0);
}

// One slice: 'I' opens a keyframe group, 'P' and 'B' follow it.
static void s_slice(s_annexb_t* s, char type, int frame_num, int poc)
{
	int idr = type == 'I';
	s_put_ue(s, 0);                                     // first_mb_in_slice
	s_put_ue(s, type == 'I' ? 7 : type == 'P' ? 5 : 6); // slice_type
	s_put_ue(s, 0);                                     // pic_parameter_set_id
	s_put(s, (uint32_t)frame_num & 15, 4);
	if (idr) s_put_ue(s, 0);                            // idr_pic_id
	s_put(s, (uint32_t)poc & 63, 6);                    // pic_order_cnt_lsb
	s_nal(s, idr ? 0x65 : type == 'P' ? 0x41 : 0x01, 8);
}

typedef struct s_picture_t { char type; int frame_num, poc; } s_picture_t;
#endif // CF_STATIC

/* The muxer must place pictures by the ORDER of their order counts, never by their value. The
   specification counts fields, so most encoders step the count by two per frame, but VideoToolbox
   steps it by one, and either may leave gaps. Halving the count put every two VideoToolbox frames
   on one slot, and the clip played at twice speed (issue #602). */
TEST_CASE(test_video_mp4_order_counts)
{
#if !defined(CF_STATIC)
	// Feeds the muxer directly (ch_mp4_wrap), which is internal to the library and which shared
	// (DLL) builds don't export -- and shouldn't. Static builds cover it.
	return true;
#else
	// VideoToolbox: a step of one, B pictures, and a second keyframe group that has to follow the
	// first rather than land on top of it. The first five are the head of the clip in the issue.
	static const s_picture_t step_one[] = {
		{ 'I', 0, 0 }, { 'P', 1, 4 }, { 'B', 2, 2 }, { 'B', 3, 1 }, { 'B', 3, 3 },
		{ 'I', 0, 0 }, { 'P', 1, 2 }, { 'B', 2, 1 },
	};
	// The software encoder here: the same pictures with a step of two.
	static const s_picture_t step_two[] = {
		{ 'I', 0, 0 }, { 'P', 1, 8 }, { 'B', 2, 4 }, { 'B', 3, 2 }, { 'B', 3, 6 },
	};
	// Gaps in the count, as an encoder that dropped a frame would leave. Every picture still
	// lasts exactly one frame in the file.
	static const s_picture_t gaps[] = {
		{ 'I', 0, 0 }, { 'P', 1, 10 }, { 'B', 2, 4 }, { 'B', 3, 2 }, { 'B', 3, 8 },
	};
	// No reordering at all: decode order is display order, and the file needs no offsets.
	static const s_picture_t in_order[] = {
		{ 'I', 0, 0 }, { 'P', 1, 1 }, { 'P', 2, 2 }, { 'P', 3, 3 },
	};
	struct { const char* name; const s_picture_t* pictures; int count, lead; } cases[] = {
		{ "step of one", step_one, 8, 2 },
		{ "step of two", step_two, 5, 2 },
		{ "gaps", gaps, 5, 2 },
		{ "in order", in_order, 4, 0 },
	};
	for (int c = 0; c < (int)(sizeof(cases) / sizeof(cases[0])); ++c) {
		s_annexb_t s; memset(&s, 0, sizeof(s));
		s_parameter_sets(&s);
		for (int i = 0; i < cases[c].count; ++i) {
			s_slice(&s, cases[c].pictures[i].type, cases[c].pictures[i].frame_num, cases[c].pictures[i].poc);
		}
		int size = 0;
		const void* mp4 = ch_mp4_wrap(s.data, s.len, 64, 64, 60, &size);
		CHECK_POINTER((void*)mp4);
		int lead = -1;
		int count = s_check_timeline(mp4, size, &lead);
		cf_free((void*)mp4);
		if (count != cases[c].count || lead != cases[c].lead) {
			printf("%s: %d pictures placed, expected %d; lead %d, expected %d\n",
				cases[c].name, count, cases[c].count, lead, cases[c].lead);
		}
		REQUIRE(count == cases[c].count);
		REQUIRE(lead == cases[c].lead);
	}

	// A run long enough for the six-bit count to wrap around, which has to be carried across
	// rather than started over.
	s_annexb_t s; memset(&s, 0, sizeof(s));
	s_parameter_sets(&s);
	for (int k = 0; k < 40; ++k) s_slice(&s, k ? 'P' : 'I', k, 2 * k);
	int size = 0;
	const void* mp4 = ch_mp4_wrap(s.data, s.len, 64, 64, 60, &size);
	CHECK_POINTER((void*)mp4);
	int lead = -1;
	int count = s_check_timeline(mp4, size, &lead);
	cf_free((void*)mp4);
	REQUIRE(count == 40);
	REQUIRE(lead == 0);
	return true;
#endif // CF_STATIC
}

/* The same check on what the encoders here actually write: the software one at the size the other
   tests use, and whichever backend the platform picks at a size the hardware encoders accept --
   VideoToolbox on macOS, which is where issue #602 was found. */
TEST_CASE(test_video_mp4_timeline)
{
	CF_VideoEncoder* encoder = s_record(50);
	CHECK_POINTER(encoder);
	int size = 0;
	const void* mp4 = cf_video_encoder_data(encoder, &size);
	CHECK_POINTER((void*)mp4);
	REQUIRE(s_check_timeline(mp4, size, NULL) == VIDEO_FRAMES);
	cf_destroy_video_encoder(encoder);

#if !defined(CF_WINDOWS)
	// Hardware encoders are only tried from 128 pixels up, and a second of frames gives one that
	// writes B pictures room to reorder them. Not on Windows: Media Foundation writes its own
	// container rather than going through the muxer here, so there is nothing of ours to check.
	const int W = 128, H = 128, FRAMES = 30;
	encoder = cf_make_video_encoder(W, H, 30);
	CHECK_POINTER(encoder);
	CF_Pixel* pix = (CF_Pixel*)cf_alloc(sizeof(CF_Pixel) * W * H);
	for (int i = 0; i < FRAMES; ++i) {
		s_fill_size(pix, W, H, i);
		CF_Image image = { W, H, pix };
		REQUIRE(!cf_is_error(cf_video_encoder_add_frame(encoder, image)));
	}
	cf_free(pix);
	mp4 = cf_video_encoder_data(encoder, &size);
	CHECK_POINTER((void*)mp4);
	REQUIRE(s_check_timeline(mp4, size, NULL) == FRAMES);
	cf_destroy_video_encoder(encoder);
#endif // CF_WINDOWS
	return true;
}

TEST_SUITE(test_video)
{
	RUN_TEST_CASE(test_video_round_trip);
	RUN_TEST_CASE(test_video_quality);
	RUN_TEST_CASE(test_video_save_and_load);
	RUN_TEST_CASE(test_video_update_and_loop);
	RUN_TEST_CASE(test_video_seek);
	RUN_TEST_CASE(test_video_rejects_rubbish);
	RUN_TEST_CASE(test_video_record_canvas);
	RUN_TEST_CASE(test_video_sprite_and_texture);
	RUN_TEST_CASE(test_video_draw3d);
	RUN_TEST_CASE(test_video_mp4_order_counts);
	RUN_TEST_CASE(test_video_mp4_timeline);
}
