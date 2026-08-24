/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_alloc.h>

// Buffers cross the boundary in both directions -- the decoder hands out pictures this file copies
// into textures, and ch_mp4_unwrap hands back a stream this file frees -- so both sides must be on
// CF's allocator.
#define CUTE_H264_ALLOC(size) cf_alloc(size)
#define CUTE_H264_FREE(mem) cf_free(mem)
#define CUTE_H264_REALLOC(mem, size) cf_realloc(mem, size)
#define CUTE_H264_NO_STDIO // CF writes through its VFS.
#define CUTE_H264_IMPLEMENTATION
#include <cute/cute_h264.h>

#include <cute_video.h>
#include <cute_file_system.h>
#include <cute_c_runtime.h>

#include <internal/cute_alloc_internal.h>

// The codec reports its own failures through ch_error_reason and ch_decoder_error; this covers the
// ones that happen before it is reached, and gives every entry point one place to look.
static const char* s_video_error;

struct CF_Video
{
	uint8_t* annexb;       // the elementary stream, kept so the video can be restarted
	int annexb_size;
	ch_decoder_t* decoder;
	int w, h, fps;
	bool looped;
	bool finished;
	bool has_frame;
	int frame;             // how many frames have been decoded
	float clock;           // seconds of playback owed but not yet taken
	CF_Texture texture;
	bool has_texture;
	int texture_frame;     // which frame the texture holds, so it is not re-uploaded needlessly
	CF_Sprite sprite;
	bool has_sprite;
	int sprite_frame;
	int total_frames;      // counted from the stream when the video is opened
	int* key_offsets;      // byte offsets decoding can start cold at: the SPS ahead of a keyframe
	int* key_bases;        // the display index of each such keyframe
	int key_count;
};

// How many canvas captures may be crossing back from the GPU at once. Harvested every update, so
// reaching the cap means the GPU is a full 8 frames behind -- at that point skipping a capture is
// the right call anyway.
#define CF_VIDEO_GRABS 8

struct CF_VideoEncoder
{
	ch_encoder_t* encoder;
	int w, h, fps;
	const void* mp4;       // owned by this, handed out by cf_video_encoder_data
	CF_Readback grabs[CF_VIDEO_GRABS];   // captures in flight, oldest at grab_first
	int grab_repeats[CF_VIDEO_GRABS];    // how many encoded frames each capture becomes
	int grab_first, grab_num;
	float clock;           // seconds of recording owed but not yet captured
	uint8_t* grab_pixels;  // scratch a capture is read back into, w*h*4
};

// Walks the stream once, counting pictures and remembering every keyframe that can be decoded
// cold -- one with its parameter sets right in front of it, which is how the CF encoder writes
// them. cf_video_seek starts at the nearest of these; a stream carrying none still seeks, it just
// decodes forward from the beginning. A picture is a slice NAL whose first_mb_in_slice is 0, and
// ue(0) is the single bit 1, so the top bit of the first payload byte is the whole test. Nothing
// coded after a keyframe shows before it -- the encoder flushes any held picture first -- so a
// keyframe's display index is simply how many pictures precede it.
static void s_scan(CF_Video* video)
{
	const uint8_t* s = video->annexb;
	int n = video->annexb_size;
	int sps_at = -1;       // start code of an SPS seen since the last picture
	bool pps_after = false;
	int cap = 0;
	for (int i = 0; i + 4 < n; ++i) {
		if (!(s[i] == 0 && s[i + 1] == 0 && s[i + 2] == 1)) continue;
		int sc = (i > 0 && s[i - 1] == 0) ? i - 1 : i;
		int type = s[i + 3] & 0x1f;
		if (type == 7) {                             // sequence parameter set
			sps_at = sc;
			pps_after = false;
		} else if (type == 8) {                      // picture parameter set
			if (sps_at >= 0) pps_after = true;
		} else if ((type == 1 || type == 5) && (s[i + 4] & 0x80)) {
			if (type == 5 && sps_at >= 0 && pps_after) {
				if (video->key_count == cap) {
					cap = cap ? cap * 2 : 16;
					video->key_offsets = (int*)CF_REALLOC(video->key_offsets, sizeof(int) * (size_t)cap);
					video->key_bases = (int*)CF_REALLOC(video->key_bases, sizeof(int) * (size_t)cap);
				}
				video->key_offsets[video->key_count] = sps_at;
				video->key_bases[video->key_count] = video->total_frames;
				++video->key_count;
			}
			++video->total_frames;
			sps_at = -1;
			pps_after = false;
		}
	}
}

// A file is either an elementary stream, which starts with a NAL start code, or a container, which
// starts with a box. Nothing else gets this far.
static bool s_is_annexb(const uint8_t* d, int n)
{
	if (n < 4) return false;
	if (d[0] == 0 && d[1] == 0 && d[2] == 1) return true;
	return d[0] == 0 && d[1] == 0 && d[2] == 0 && d[3] == 1;
}

const char* cf_video_error()
{
	if (s_video_error) return s_video_error;
	if (ch_decoder_error) return ch_decoder_error;
	if (ch_error_reason) return ch_error_reason;
	return "No error.";
}

CF_Video* cf_make_video_from_memory(const void* data, int size)
{
	s_video_error = NULL;
	ch_decoder_error = NULL;
	ch_error_reason = NULL;
	if (!data || size <= 0) {
		s_video_error = "Null or empty video data.";
		return NULL;
	}

	const uint8_t* bytes = (const uint8_t*)data;
	uint8_t* annexb = NULL;
	int annexb_size = 0;
	if (s_is_annexb(bytes, size)) {
		annexb = (uint8_t*)CF_ALLOC((size_t)size);
		if (!annexb) { s_video_error = "Out of memory."; return NULL; }
		CF_MEMCPY(annexb, data, (size_t)size);
		annexb_size = size;
	} else {
		// A container. ch_mp4_unwrap allocates through CF, so the buffer is ours either way.
		annexb = (uint8_t*)ch_mp4_unwrap(data, size, &annexb_size);
		if (!annexb) return NULL;   // ch_error_reason says why
	}

	CF_Video* video = (CF_Video*)CF_CALLOC(sizeof(CF_Video));
	if (!video) { CF_FREE(annexb); s_video_error = "Out of memory."; return NULL; }
	video->annexb = annexb;
	video->annexb_size = annexb_size;
	video->decoder = ch_decoder_make(annexb, annexb_size);
	if (!video->decoder) { CF_FREE(annexb); CF_FREE(video); return NULL; }

	// The size and rate come out of the parameter sets, which sit ahead of the first picture, so
	// one decode is enough to answer them -- and it is a decode the caller was going to need.
	if (!ch_decoder_next(video->decoder)) {
		s_video_error = ch_decoder_error ? ch_decoder_error : "The file carries no pictures.";
		cf_destroy_video(video);
		return NULL;
	}
	ch_decoder_size(video->decoder, &video->w, &video->h);
	video->fps = ch_decoder_fps(video->decoder);
	video->has_frame = true;
	video->frame = 1;
	video->texture_frame = -1;
	video->sprite_frame = -1;
	s_scan(video);
	return video;
}

CF_Video* cf_make_video(const char* virtual_path)
{
	s_video_error = NULL;
	size_t size = 0;
	void* data = cf_fs_read_entire_file_to_memory(virtual_path, &size);
	if (!data) {
		s_video_error = "Unable to read the video file from the virtual file system.";
		return NULL;
	}
	if (size > 0x7fffffff) {
		CF_FREE(data);
		s_video_error = "Video files over 2GB are not supported.";
		return NULL;
	}
	CF_Video* video = cf_make_video_from_memory(data, (int)size);
	CF_FREE(data);
	return video;
}

void cf_destroy_video(CF_Video* video)
{
	if (!video) return;
	if (video->has_sprite) cf_easy_sprite_unload(&video->sprite);
	if (video->has_texture) cf_destroy_texture(video->texture);
	if (video->decoder) ch_decoder_destroy(video->decoder);
	CF_FREE(video->key_offsets);
	CF_FREE(video->key_bases);
	CF_FREE(video->annexb);
	CF_FREE(video);
}

int cf_video_width(CF_Video* video) { return video ? video->w : 0; }
int cf_video_height(CF_Video* video) { return video ? video->h : 0; }
int cf_video_fps(CF_Video* video) { return video ? video->fps : 0; }
bool cf_video_is_finished(CF_Video* video) { return video ? video->finished : true; }
void cf_video_set_looped(CF_Video* video, bool looped) { if (video) video->looped = looped; }
int cf_video_frame_count(CF_Video* video) { return video ? video->total_frames : 0; }
int cf_video_frame_index(CF_Video* video) { return video && video->has_frame ? video->frame - 1 : 0; }
float cf_video_duration(CF_Video* video) { return video && video->fps > 0 ? (float)video->total_frames / (float)video->fps : 0; }

bool cf_video_seek(CF_Video* video, int frame)
{
	if (!video) return false;
	if (frame < 0 || frame >= video->total_frames) {
		s_video_error = "No such frame.";
		return false;
	}
	// The nearest cold-start keyframe at or before the target; a stream without any decodes
	// forward from the top.
	int base = 0, offset = 0;
	for (int i = 0; i < video->key_count && video->key_bases[i] <= frame; ++i) {
		base = video->key_bases[i];
		offset = video->key_offsets[i];
	}
	ch_decoder_error = NULL;
	ch_decoder_t* decoder = ch_decoder_make(video->annexb + offset, video->annexb_size - offset);
	for (int i = frame - base + 1; decoder && i > 0; --i) {
		if (!ch_decoder_next(decoder)) {
			ch_decoder_destroy(decoder);
			decoder = NULL;
		}
	}
	if (!decoder) {
		s_video_error = ch_decoder_error ? ch_decoder_error : "Unable to seek.";
		return false;   // the old decoder was never touched, so playback stands where it was
	}
	if (video->decoder) ch_decoder_destroy(video->decoder);
	video->decoder = decoder;
	video->frame = frame + 1;
	video->has_frame = true;
	video->finished = false;
	video->clock = 0;
	return true;
}

void cf_video_restart(CF_Video* video)
{
	if (!video) return;
	ch_decoder_error = NULL;
	if (video->decoder) ch_decoder_destroy(video->decoder);
	video->decoder = ch_decoder_make(video->annexb, video->annexb_size);
	video->finished = false;
	video->has_frame = false;
	video->frame = 0;
	video->clock = 0;
	if (video->decoder && ch_decoder_next(video->decoder)) {
		video->has_frame = true;
		video->frame = 1;
	}
}

bool cf_video_next_frame(CF_Video* video)
{
	if (!video || !video->decoder) return false;
	ch_decoder_error = NULL;
	if (!ch_decoder_next(video->decoder)) {
		if (ch_decoder_error) s_video_error = ch_decoder_error;
		video->finished = true;
		return false;
	}
	video->has_frame = true;
	++video->frame;
	return true;
}

bool cf_video_update(CF_Video* video, float dt)
{
	if (!video || !video->decoder) return false;
	int fps = video->fps > 0 ? video->fps : 30;
	float step = 1.0f / (float)fps;
	video->clock += dt;
	// A stall must not turn into a long catch-up that stalls the game in turn, so the clock is
	// clamped rather than being allowed to owe an unbounded number of frames.
	if (video->clock > step * 4) video->clock = step * 4;

	bool advanced = false;
	while (video->clock >= step) {
		video->clock -= step;
		if (!cf_video_next_frame(video)) {
			if (!video->looped) { video->clock = 0; break; }
			cf_video_restart(video);
			advanced = true;
			break;
		}
		advanced = true;
	}
	return advanced;
}

CF_Image cf_video_frame(CF_Video* video)
{
	CF_Image image = { 0, 0, NULL };
	if (!video || !video->has_frame) return image;
	const void* rgba = ch_decoder_rgba(video->decoder);
	if (!rgba) return image;
	image.w = video->w;
	image.h = video->h;
	image.pix = (CF_Pixel*)rgba;
	return image;
}

CF_Sprite cf_video_sprite(CF_Video* video)
{
	if (!video) return cf_sprite_defaults();
	CF_Image image = cf_video_frame(video);
	if (!video->has_sprite) {
		if (!image.pix) return cf_sprite_defaults();
		video->sprite = cf_make_easy_sprite_from_pixels(image.pix, image.w, image.h);
		video->has_sprite = true;
		video->sprite_frame = video->frame;
		return video->sprite;
	}
	if (video->sprite_frame != video->frame && image.pix) {
		cf_easy_sprite_update_pixels(&video->sprite, image.pix);
		video->sprite_frame = video->frame;
	}
	return video->sprite;
}

CF_Texture cf_video_texture(CF_Video* video)
{
	CF_Texture none = { 0 };
	if (!video) return none;
	if (!video->has_texture) {
		CF_TextureParams params = cf_texture_defaults(video->w, video->h);
		params.filter = CF_FILTER_LINEAR;
		video->texture = cf_make_texture(params);
		video->has_texture = true;
		video->texture_frame = -1;
	}
	if (video->texture_frame != video->frame) {
		CF_Image image = cf_video_frame(video);
		if (image.pix) {
			cf_texture_update(video->texture, image.pix, image.w * image.h * (int)sizeof(CF_Pixel));
			video->texture_frame = video->frame;
		}
	}
	return video->texture;
}

// -------------------------------------------------------------------------------------------------

CF_VideoEncoder* cf_make_video_encoder(int w, int h, int fps)
{
	s_video_error = NULL;
	ch_error_reason = NULL;
	CF_VideoEncoder* encoder = (CF_VideoEncoder*)CF_CALLOC(sizeof(CF_VideoEncoder));
	if (!encoder) { s_video_error = "Out of memory."; return NULL; }
	encoder->encoder = ch_encoder_make(w, h, fps);
	if (!encoder->encoder) { CF_FREE(encoder); return NULL; }
	encoder->w = w;
	encoder->h = h;
	encoder->fps = fps;
	// The defaults this API promises: the better entropy coder, and pictures that predict from
	// both sides. Both cost encode time and neither costs compatibility with anything modern.
	ch_encoder_cabac(encoder->encoder, 1);
	ch_encoder_bframes(encoder->encoder, 1);
	cf_video_encoder_set_quality(encoder, 50);
	return encoder;
}

void cf_destroy_video_encoder(CF_VideoEncoder* encoder)
{
	if (!encoder) return;
	// Captures still in flight are simply dropped -- their pixels have nowhere to go.
	for (int i = 0; i < encoder->grab_num; ++i) {
		cf_destroy_readback(encoder->grabs[(encoder->grab_first + i) % CF_VIDEO_GRABS]);
	}
	CF_FREE(encoder->grab_pixels);
	if (encoder->encoder) ch_encoder_destroy(encoder->encoder);
	CF_FREE((void*)encoder->mp4);
	CF_FREE(encoder);
}

void cf_video_encoder_set_quality(CF_VideoEncoder* encoder, int quality)
{
	if (!encoder) return;
	if (quality < 0) quality = 0;
	if (quality > 100) quality = 100;
	// The codec's knob is a quantizer: 51 throws nearly everything away, 0 keeps nearly all of it,
	// and -1 is the separate lossless path. 100 is the only value that reaches it, because
	// lossless is a different kind of thing rather than the top of the same scale.
	if (quality == 100) {
		ch_encoder_qp(encoder->encoder, -1);
		// Lossless codes every macroblock as raw samples, where B pictures have nothing to offer
		// and cost a frame of delay for it.
		ch_encoder_bframes(encoder->encoder, 0);
	} else {
		ch_encoder_qp(encoder->encoder, 51 - quality * 51 / 100);
		// Coming back from lossless has to undo the line above, or a dip to 100 and back would
		// leave B pictures off for the rest of the recording.
		ch_encoder_bframes(encoder->encoder, 1);
	}
}

// Compresses one finished capture, feeding it in `repeats` times when the recording owes more
// than one frame of time -- the pictures are identical so the copies land as P_Skip and cost a
// few bytes each, which is what keeps the file true to the wall clock.
static int s_feed(CF_VideoEncoder* encoder, CF_Readback readback, int repeats)
{
	int size = encoder->w * encoder->h * (int)sizeof(CF_Pixel);
	if (cf_readback_size(readback) != size) {
		s_video_error = "The canvas does not match the size the encoder was created with.";
		return 0;
	}
	if (!encoder->grab_pixels) encoder->grab_pixels = (uint8_t*)CF_ALLOC((size_t)size);
	if (!encoder->grab_pixels) { s_video_error = "Out of memory."; return 0; }
	if (cf_readback_data(readback, encoder->grab_pixels, size) != size) {
		s_video_error = "Unable to read the capture back.";
		return 0;
	}
	int added = 0;
	for (int i = 0; i < repeats; ++i) {
		ch_error_reason = NULL;
		if (!ch_encoder_frame(encoder->encoder, encoder->grab_pixels)) {
			s_video_error = ch_error_reason ? ch_error_reason : "Unable to encode the frame.";
			break;
		}
		++added;
	}
	return added;
}

// Compresses every capture the GPU has finished with, in the order they were asked for. Stops at
// the first one still in flight -- order is part of what is being recorded.
static int s_harvest(CF_VideoEncoder* encoder)
{
	int added = 0;
	while (encoder->grab_num) {
		CF_Readback readback = encoder->grabs[encoder->grab_first];
		if (!cf_readback_ready(readback)) break;
		added += s_feed(encoder, readback, encoder->grab_repeats[encoder->grab_first]);
		cf_destroy_readback(readback);
		encoder->grab_first = (encoder->grab_first + 1) % CF_VIDEO_GRABS;
		--encoder->grab_num;
	}
	return added;
}

// The blocking version, for save: everything in flight is waited for, so the file carries every
// frame the recording owes. The wait is bounded -- a readback whose fence never signals (a lost
// device, a driver fault) drops the remaining captures rather than hanging the save.
static int s_drain(CF_VideoEncoder* encoder)
{
	int added = 0;
	while (encoder->grab_num) {
		CF_Readback readback = encoder->grabs[encoder->grab_first];
		for (int64_t spin = 0; !cf_readback_ready(readback) && spin < ((int64_t)1 << 28); ++spin) {}
		if (!cf_readback_ready(readback)) {
			for (int i = 0; i < encoder->grab_num; ++i) {
				cf_destroy_readback(encoder->grabs[(encoder->grab_first + i) % CF_VIDEO_GRABS]);
			}
			encoder->grab_num = 0;
			s_video_error = "A capture never came back from the GPU.";
			break;
		}
		added += s_feed(encoder, readback, encoder->grab_repeats[encoder->grab_first]);
		cf_destroy_readback(readback);
		encoder->grab_first = (encoder->grab_first + 1) % CF_VIDEO_GRABS;
		--encoder->grab_num;
	}
	return added;
}

int cf_video_encoder_update(CF_VideoEncoder* encoder, CF_Canvas canvas, float dt)
{
	if (!encoder) { s_video_error = "Null encoder."; return 0; }
	int added = s_harvest(encoder);
	float step = 1.0f / (float)encoder->fps;
	encoder->clock += dt;
	// A stall must not turn into a burst of catch-up frames, so the recording drops behind
	// instead -- the mirror of the clamp in cf_video_update.
	if (encoder->clock > step * 4) encoder->clock = step * 4;
	int owed = (int)(encoder->clock / step);
	if (owed > 0 && encoder->grab_num < CF_VIDEO_GRABS) {
		CF_Readback readback = cf_canvas_readback(canvas);
		if (!readback.id) {
			s_video_error = "Canvas readback failed, or is unsupported on this platform.";
			return added;
		}
		int at = (encoder->grab_first + encoder->grab_num) % CF_VIDEO_GRABS;
		encoder->grabs[at] = readback;
		encoder->grab_repeats[at] = owed;
		++encoder->grab_num;
		encoder->clock -= step * (float)owed;
	}
	return added;
}

CF_Result cf_video_encoder_add_frame(CF_VideoEncoder* encoder, CF_Image frame)
{
	if (!encoder) return cf_result_error("Null encoder.");
	if (!frame.pix) return cf_result_error("Null pixels.");
	if (frame.w != encoder->w || frame.h != encoder->h) {
		return cf_result_error("Frame size does not match the encoder.");
	}
	ch_error_reason = NULL;
	if (!ch_encoder_frame(encoder->encoder, frame.pix)) {
		return cf_result_error(ch_error_reason ? ch_error_reason : "Unable to encode the frame.");
	}
	return cf_result_success();
}

const void* cf_video_encoder_data(CF_VideoEncoder* encoder, int* size)
{
	if (size) *size = 0;
	if (!encoder) { s_video_error = "Null encoder."; return NULL; }
	s_drain(encoder);
	ch_error_reason = NULL;
	int raw_size = 0;
	const void* raw = ch_encoder_data(encoder->encoder, &raw_size);
	if (!raw || !raw_size) { s_video_error = "The encoder has no frames in it."; return NULL; }
	CF_FREE((void*)encoder->mp4);
	int mp4_size = 0;
	encoder->mp4 = ch_mp4_wrap(raw, raw_size, encoder->w, encoder->h,
		encoder->fps, &mp4_size);
	if (!encoder->mp4) return NULL;
	if (size) *size = mp4_size;
	return encoder->mp4;
}

CF_Result cf_video_encoder_save(CF_VideoEncoder* encoder, const char* virtual_path)
{
	if (!encoder) return cf_result_error("Null encoder.");
	if (!virtual_path) return cf_result_error("Null path.");
	size_t len = CF_STRLEN(virtual_path);
	bool raw_stream = (len > 5 && !CF_STRCMP(virtual_path + len - 5, ".h264"))
	               || (len > 4 && !CF_STRCMP(virtual_path + len - 4, ".264"));
	if (raw_stream) {
		s_drain(encoder);
		ch_error_reason = NULL;
		int size = 0;
		const void* data = ch_encoder_data(encoder->encoder, &size);
		if (!data || !size) return cf_result_error("The encoder has no frames in it.");
		return cf_fs_write_entire_buffer_to_file(virtual_path, data, (size_t)size);
	}
	int size = 0;
	const void* mp4 = cf_video_encoder_data(encoder, &size);
	if (!mp4) return cf_result_error(cf_video_error());
	return cf_fs_write_entire_buffer_to_file(virtual_path, mp4, (size_t)size);
}
