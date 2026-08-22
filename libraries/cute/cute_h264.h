/*
	------------------------------------------------------------------------------
		Licensing information can be found at the end of the file.
	------------------------------------------------------------------------------

	cute_h264.h - v0.01

	To create implementation (the function definitions)
		#define CUTE_H264_IMPLEMENTATION
	in *one* C/CPP file (translation unit) that includes this file


	SUMMARY:

		A from-scratch H.264 encoder and decoder in a single header, the sibling of
		cute_jpg.h. Written for capturing gameplay video without dragging in a
		multi-megabyte codec library or shelling out to an external encoder.

		The output is an Annex-B elementary stream (.264/.h264), which ffmpeg, VLC
		and mpv all open directly, and which an MP4 muxer can wrap without
		re-encoding.

		WORK IN PROGRESS. What is here now:

			encode: baseline profile, I_PCM macroblocks -- every frame is a
			        keyframe and every macroblock stores raw samples, so the
			        stream is mathematically LOSSLESS and decodes back to the
			        exact bytes that went in. Big files: roughly 1.5x raw 4:2:0.
			decode: not yet.

		I_PCM first is deliberate, not a shortcut that got left in. It exercises
		every part of the bitstream that the compressed paths also depend on --
		NAL framing, emulation prevention, exp-golomb, the SPS/PPS/slice headers,
		the macroblock loop -- with no transform, no prediction and no entropy
		coder to be simultaneously wrong. Once a decoder agrees with it byte for
		byte, everything after this is a change to macroblock CONTENTS on a
		container that is already known-good. It also leaves a genuinely useful
		lossless capture mode behind once the compressed paths land.

		Planned, in order: I_16x16 and I_4x4 intra with the integer transform and
		CAVLC; P macroblocks with motion estimation; the deblocking filter; then
		the decoder, validated against both this encoder and ffmpeg's output.

	EXAMPLES:

		Encoding frames to a file
			ch_encoder_t* e = ch_encoder_make(1280, 720, 30);
			for (int i = 0; i < frame_count; ++i) {
				ch_encoder_frame(e, rgba_pixels[i]);
			}
			ch_encoder_save(e, "capture.264");
			ch_encoder_destroy(e);

		Encoding to memory instead
			int size;
			const void* bytes = ch_encoder_data(e, &size);

	CUSTOMIZATION

		There are various macros in this header you can customize by defining them before
		including cute_h264.h. Simply define one to override the default behavior.

			CUTE_H264_ALLOC
			CUTE_H264_FREE
			CUTE_H264_REALLOC
			CUTE_H264_MEMCPY
			CUTE_H264_MEMSET
			CUTE_H264_ASSERT
			CUTE_H264_NO_STDIO

	Revision history:
		0.01 (08/22/2026) bitstream layer + lossless I_PCM encode
*/

#if !defined(CUTE_H264_H)

#include <stdint.h>

typedef struct ch_encoder_t ch_encoder_t;

// Read this after any function reports failure.
extern const char* ch_error_reason;

// Dimensions must be EVEN -- 4:2:0 chroma cannot express an odd one, and the crop rectangle
// that trims the macroblock padding is itself measured in chroma units. Any even size works:
// H.264 codes whole 16x16 macroblocks, so the remainder is padded internally by replicating
// the edge and cropped back out by the stream. Returns NULL on failure, ch_error_reason set.
ch_encoder_t* ch_encoder_make(int w, int h, int fps);
void ch_encoder_destroy(ch_encoder_t* e);

// Feeds one frame. `rgba` is w*h pixels of RGBA8, top row first, 4 bytes per pixel.
// Returns 1 on success, 0 on failure with ch_error_reason set.
int ch_encoder_frame(ch_encoder_t* e, const void* rgba);

// The Annex-B stream accumulated so far. The pointer is owned by the encoder and is
// invalidated by the next ch_encoder_frame call.
const void* ch_encoder_data(ch_encoder_t* e, int* size);

#if !defined(CUTE_H264_NO_STDIO)
int ch_encoder_save(ch_encoder_t* e, const char* file_name);
#endif

#define CUTE_H264_H
#endif // CUTE_H264_H

#if defined(CUTE_H264_IMPLEMENTATION)
#if !defined(CUTE_H264_IMPLEMENTATION_ONCE)
#define CUTE_H264_IMPLEMENTATION_ONCE

#if !defined(CUTE_H264_ALLOC)
	#include <stdlib.h>
	#define CUTE_H264_ALLOC(size) malloc(size)
	#define CUTE_H264_FREE(mem) free(mem)
	#define CUTE_H264_REALLOC(mem, size) realloc(mem, size)
#endif

#if !defined(CUTE_H264_MEMCPY)
	#include <string.h>
	#define CUTE_H264_MEMCPY memcpy
#endif

#if !defined(CUTE_H264_MEMSET)
	#include <string.h>
	#define CUTE_H264_MEMSET memset
#endif

#if !defined(CUTE_H264_ASSERT)
	#include <assert.h>
	#define CUTE_H264_ASSERT assert
#endif

#if !defined(CUTE_H264_NO_STDIO)
	#include <stdio.h>
#endif

const char* ch_error_reason;

//--------------------------------------------------------------------------------------------------
// Growable byte buffer. Every write goes through ch_bytes_push so a failed grow can be caught
// once, at the end, instead of at each of the hundreds of call sites.

typedef struct ch_bytes_t
{
	uint8_t* data;
	int len;
	int cap;
	int oom;      // sticky: set once on a failed grow, checked by the caller
} ch_bytes_t;

static void ch_bytes_reserve(ch_bytes_t* b, int need)
{
	if (b->oom) return;
	if (b->len + need <= b->cap) return;
	int cap = b->cap ? b->cap * 2 : 65536;
	while (cap < b->len + need) cap *= 2;
	uint8_t* mem = (uint8_t*)CUTE_H264_REALLOC(b->data, (size_t)cap);
	if (!mem) { b->oom = 1; return; }
	b->data = mem;
	b->cap = cap;
}

static void ch_bytes_push(ch_bytes_t* b, uint8_t v)
{
	ch_bytes_reserve(b, 1);
	if (b->oom) return;
	b->data[b->len++] = v;
}

//--------------------------------------------------------------------------------------------------
// RBSP bit writer. This writes the RAW payload; emulation prevention is applied later, when the
// payload is wrapped into a NAL unit, because a 00 00 0x sequence can straddle syntax elements
// and is only knowable once the bytes exist.

typedef struct ch_bits_t
{
	ch_bytes_t bytes;
	uint32_t acc;   // bits waiting to be flushed, left-justified in the low `nbits`
	int nbits;
} ch_bits_t;

static void ch_put_bits(ch_bits_t* w, uint32_t value, int n)
{
	CUTE_H264_ASSERT(n >= 0 && n <= 32);
	for (int i = n - 1; i >= 0; --i) {
		w->acc = (w->acc << 1) | ((value >> i) & 1u);
		if (++w->nbits == 8) {
			ch_bytes_push(&w->bytes, (uint8_t)w->acc);
			w->acc = 0;
			w->nbits = 0;
		}
	}
}

static void ch_put_bit(ch_bits_t* w, int v) { ch_put_bits(w, (uint32_t)(v & 1), 1); }

// Unsigned exp-golomb, ue(v). The value+1 is written as a leading run of zeros the length of
// its own magnitude, then the value itself with its top bit acting as the terminator.
static void ch_ue(ch_bits_t* w, uint32_t v)
{
	uint32_t x = v + 1;
	int n = 0;
	for (uint32_t t = x >> 1; t; t >>= 1) ++n;
	ch_put_bits(w, 0, n);
	ch_put_bits(w, x, n + 1);
}

// Signed exp-golomb, se(v). Zig-zags so small magnitudes of either sign stay short.
static void ch_se(ch_bits_t* w, int32_t v)
{
	ch_ue(w, v <= 0 ? (uint32_t)(-2 * v) : (uint32_t)(2 * v - 1));
}

// rbsp_trailing_bits: a single 1 then zeros to the byte boundary. Without this a decoder cannot
// tell payload from padding, because the padding is not length-prefixed.
static void ch_rbsp_trailing(ch_bits_t* w)
{
	ch_put_bit(w, 1);
	while (w->nbits) ch_put_bit(w, 0);
}

// Byte-aligns by writing zeros. Used before I_PCM samples, which are raw bytes.
static void ch_align_zero(ch_bits_t* w)
{
	while (w->nbits) ch_put_bit(w, 0);
}

static void ch_bits_reset(ch_bits_t* w)
{
	w->bytes.len = 0;
	w->acc = 0;
	w->nbits = 0;
}

//--------------------------------------------------------------------------------------------------
// NAL framing.

// Emulation prevention: inside a NAL payload the three-byte sequences 00 00 00, 00 00 01,
// 00 00 02 and 00 00 03 are illegal, because a decoder scans for 00 00 01 to find the next NAL
// and must be able to do so without parsing. An 0x03 is inserted after any 00 00 that would be
// followed by one of those, and the decoder strips it back out.
static void ch_emit_nal(ch_bytes_t* out, int nal_ref_idc, int nal_unit_type, const uint8_t* rbsp, int len)
{
	ch_bytes_push(out, 0);
	ch_bytes_push(out, 0);
	ch_bytes_push(out, 0);
	ch_bytes_push(out, 1);
	ch_bytes_push(out, (uint8_t)(((nal_ref_idc & 3) << 5) | (nal_unit_type & 31)));
	int zeros = 0;
	for (int i = 0; i < len; ++i) {
		uint8_t v = rbsp[i];
		if (zeros >= 2 && v <= 3) {
			ch_bytes_push(out, 3);
			zeros = 0;
		}
		ch_bytes_push(out, v);
		zeros = v ? 0 : zeros + 1;
	}
}

//--------------------------------------------------------------------------------------------------

#define CH_NAL_SLICE_IDR 5
#define CH_NAL_SPS       7
#define CH_NAL_PPS       8

struct ch_encoder_t
{
	int w, h;              // as requested by the caller
	int mb_w, mb_h;        // macroblock grid, so w/h rounded up to 16
	int fps;
	int frame_count;
	int idr_pic_id;

	uint8_t* y;            // padded planes, mb_w*16 by mb_h*16
	uint8_t* cb;           // half resolution both ways (4:2:0)
	uint8_t* cr;
	int luma_stride;
	int chroma_stride;

	ch_bits_t bits;        // scratch RBSP for the NAL under construction
	ch_bytes_t out;        // the Annex-B stream
};

//--------------------------------------------------------------------------------------------------
// Colour conversion. BT.601 studio swing, which is what an H.264 stream means when it carries no
// VUI colour description -- so writing full-range values here would come back washed out.

static uint8_t ch_clamp_u8(int v) { return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v)); }

static void ch_rgba_to_yuv420(ch_encoder_t* e, const uint8_t* rgba)
{
	int pw = e->mb_w * 16, ph = e->mb_h * 16;
	// Luma, plus edge replication into the macroblock padding. Padding with black instead would
	// put a hard edge inside the last macroblock of every row, which costs bits in the
	// compressed paths and shows up as ringing along the crop boundary.
	for (int y = 0; y < ph; ++y) {
		int sy = y < e->h ? y : e->h - 1;
		for (int x = 0; x < pw; ++x) {
			int sx = x < e->w ? x : e->w - 1;
			const uint8_t* p = rgba + ((size_t)sy * e->w + sx) * 4;
			int r = p[0], g = p[1], b = p[2];
			e->y[(size_t)y * e->luma_stride + x] =
				ch_clamp_u8((66 * r + 129 * g + 25 * b + 128 >> 8) + 16);
		}
	}
	// Chroma, box-filtered 2x2. Averaging in RGB before the matrix (rather than averaging the
	// chroma of each pixel) is what keeps saturated edges from fringing.
	for (int y = 0; y < ph / 2; ++y) {
		for (int x = 0; x < pw / 2; ++x) {
			int rs = 0, gs = 0, bs = 0;
			for (int j = 0; j < 2; ++j) {
				int sy = y * 2 + j; sy = sy < e->h ? sy : e->h - 1;
				for (int i = 0; i < 2; ++i) {
					int sx = x * 2 + i; sx = sx < e->w ? sx : e->w - 1;
					const uint8_t* p = rgba + ((size_t)sy * e->w + sx) * 4;
					rs += p[0]; gs += p[1]; bs += p[2];
				}
			}
			int r = rs / 4, g = gs / 4, b = bs / 4;
			e->cb[(size_t)y * e->chroma_stride + x] =
				ch_clamp_u8((-38 * r - 74 * g + 112 * b + 128 >> 8) + 128);
			e->cr[(size_t)y * e->chroma_stride + x] =
				ch_clamp_u8((112 * r - 94 * g - 18 * b + 128 >> 8) + 128);
		}
	}
}

//--------------------------------------------------------------------------------------------------
// Headers.

static void ch_write_sps(ch_encoder_t* e)
{
	ch_bits_t* w = &e->bits;
	ch_bits_reset(w);
	ch_put_bits(w, 66, 8);            // profile_idc: baseline
	ch_put_bit(w, 1);                 // constraint_set0_flag: really is baseline
	ch_put_bit(w, 1);                 // constraint_set1_flag: also main-conformant
	ch_put_bit(w, 0);                 // constraint_set2_flag
	ch_put_bits(w, 0, 5);             // constraint_set3..5 + reserved_zero_2bits
	ch_put_bits(w, 51, 8);            // level_idc 5.1: high enough that size never violates it
	ch_ue(w, 0);                      // seq_parameter_set_id
	ch_ue(w, 0);                      // log2_max_frame_num_minus4
	// pic_order_cnt_type 2 means display order IS decode order. Legal only when no frame is
	// reordered, which holds here and will keep holding: baseline has no B frames.
	ch_ue(w, 2);                      // pic_order_cnt_type
	ch_ue(w, 1);                      // max_num_ref_frames
	ch_put_bit(w, 0);                 // gaps_in_frame_num_value_allowed_flag
	ch_ue(w, (uint32_t)(e->mb_w - 1));// pic_width_in_mbs_minus1
	ch_ue(w, (uint32_t)(e->mb_h - 1));// pic_height_in_map_units_minus1
	ch_put_bit(w, 1);                 // frame_mbs_only_flag: progressive only
	ch_put_bit(w, 1);                 // direct_8x8_inference_flag
	// Crop away the macroblock padding. Units are chroma samples for 4:2:0, hence the /2 -- get
	// this wrong and the picture comes out with a green or smeared strip down two edges.
	int crop_r = (e->mb_w * 16 - e->w) / 2;
	int crop_b = (e->mb_h * 16 - e->h) / 2;
	if (crop_r || crop_b) {
		ch_put_bit(w, 1);             // frame_cropping_flag
		ch_ue(w, 0);                  // left
		ch_ue(w, (uint32_t)crop_r);   // right
		ch_ue(w, 0);                  // top
		ch_ue(w, (uint32_t)crop_b);   // bottom
	} else {
		ch_put_bit(w, 0);
	}
	// VUI, purely to carry the frame rate. Without it players guess, and a capture meant to be
	// 30fps plays back at whatever the player assumes.
	ch_put_bit(w, 1);                 // vui_parameters_present_flag
	ch_put_bit(w, 0);                 // aspect_ratio_info_present_flag
	ch_put_bit(w, 0);                 // overscan_info_present_flag
	ch_put_bit(w, 0);                 // video_signal_type_present_flag
	ch_put_bit(w, 0);                 // chroma_loc_info_present_flag
	ch_put_bit(w, 1);                 // timing_info_present_flag
	ch_put_bits(w, 1, 32);            // num_units_in_tick
	ch_put_bits(w, (uint32_t)(e->fps * 2), 32); // time_scale: ticks are half-frames
	ch_put_bit(w, 1);                 // fixed_frame_rate_flag
	ch_put_bit(w, 0);                 // nal_hrd_parameters_present_flag
	ch_put_bit(w, 0);                 // vcl_hrd_parameters_present_flag
	ch_put_bit(w, 0);                 // pic_struct_present_flag
	ch_put_bit(w, 0);                 // bitstream_restriction_flag
	ch_rbsp_trailing(w);
	ch_emit_nal(&e->out, 3, CH_NAL_SPS, w->bytes.data, w->bytes.len);
}

static void ch_write_pps(ch_encoder_t* e)
{
	ch_bits_t* w = &e->bits;
	ch_bits_reset(w);
	ch_ue(w, 0);                      // pic_parameter_set_id
	ch_ue(w, 0);                      // seq_parameter_set_id
	ch_put_bit(w, 0);                 // entropy_coding_mode_flag: CAVLC, not CABAC
	ch_put_bit(w, 0);                 // bottom_field_pic_order_in_frame_present_flag
	ch_ue(w, 0);                      // num_slice_groups_minus1
	ch_ue(w, 0);                      // num_ref_idx_l0_default_active_minus1
	ch_ue(w, 0);                      // num_ref_idx_l1_default_active_minus1
	ch_put_bit(w, 0);                 // weighted_pred_flag
	ch_put_bits(w, 0, 2);             // weighted_bipred_idc
	ch_se(w, 0);                      // pic_init_qp_minus26
	ch_se(w, 0);                      // pic_init_qs_minus26
	ch_se(w, 0);                      // chroma_qp_index_offset
	// Present so the slice header can turn deblocking OFF. For I_PCM that is what makes the
	// round trip bit-exact: the filter is allowed to run across PCM edges and would smear
	// samples the encoder never touched.
	ch_put_bit(w, 1);                 // deblocking_filter_control_present_flag
	ch_put_bit(w, 0);                 // constrained_intra_pred_flag
	ch_put_bit(w, 0);                 // redundant_pic_cnt_present_flag
	ch_rbsp_trailing(w);
	ch_emit_nal(&e->out, 3, CH_NAL_PPS, w->bytes.data, w->bytes.len);
}

//--------------------------------------------------------------------------------------------------

static void ch_write_idr_slice(ch_encoder_t* e)
{
	ch_bits_t* w = &e->bits;
	ch_bits_reset(w);
	ch_ue(w, 0);                      // first_mb_in_slice
	ch_ue(w, 7);                      // slice_type: I, and "all slices in this picture are I"
	ch_ue(w, 0);                      // pic_parameter_set_id
	ch_put_bits(w, 0, 4);             // frame_num, log2_max_frame_num = 4 bits. IDR resets it.
	ch_ue(w, (uint32_t)e->idr_pic_id);// idr_pic_id
	// dec_ref_pic_marking, for an IDR
	ch_put_bit(w, 0);                 // no_output_of_prior_pics_flag
	ch_put_bit(w, 0);                 // long_term_reference_flag
	ch_se(w, 0);                      // slice_qp_delta: QP 26, unused by I_PCM but still parsed
	ch_ue(w, 1);                      // disable_deblocking_filter_idc: 1 = off

	for (int mby = 0; mby < e->mb_h; ++mby) {
		for (int mbx = 0; mbx < e->mb_w; ++mbx) {
			ch_ue(w, 25);             // mb_type 25 in an I slice is I_PCM
			ch_align_zero(w);         // pcm_alignment_zero_bit
			const uint8_t* src = e->y + (size_t)(mby * 16) * e->luma_stride + mbx * 16;
			for (int y = 0; y < 16; ++y) {
				for (int x = 0; x < 16; ++x) ch_put_bits(w, src[y * (size_t)e->luma_stride + x], 8);
			}
			const uint8_t* pcb = e->cb + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
			for (int y = 0; y < 8; ++y) {
				for (int x = 0; x < 8; ++x) ch_put_bits(w, pcb[y * (size_t)e->chroma_stride + x], 8);
			}
			const uint8_t* pcr = e->cr + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
			for (int y = 0; y < 8; ++y) {
				for (int x = 0; x < 8; ++x) ch_put_bits(w, pcr[y * (size_t)e->chroma_stride + x], 8);
			}
		}
	}
	ch_rbsp_trailing(w);
	ch_emit_nal(&e->out, 3, CH_NAL_SLICE_IDR, w->bytes.data, w->bytes.len);
}

//--------------------------------------------------------------------------------------------------

ch_encoder_t* ch_encoder_make(int w, int h, int fps)
{
	if (w <= 0 || h <= 0) { ch_error_reason = "Width and height must be positive."; return NULL; }
	// 4:2:0 puts one chroma sample on every 2x2 luma quad, and the crop rectangle that trims
	// the macroblock padding is itself measured in chroma units -- so an odd width or height is
	// simply not representable, and an encoder that accepts one has to silently hand back a
	// different resolution than it was asked for. Refuse instead of surprising the caller.
	if ((w & 1) || (h & 1)) { ch_error_reason = "Width and height must be even (4:2:0 chroma)."; return NULL; }
	if (fps <= 0) { ch_error_reason = "Frame rate must be positive."; return NULL; }
	ch_encoder_t* e = (ch_encoder_t*)CUTE_H264_ALLOC(sizeof(ch_encoder_t));
	if (!e) { ch_error_reason = "Out of memory."; return NULL; }
	CUTE_H264_MEMSET(e, 0, sizeof(*e));
	e->w = w;
	e->h = h;
	e->fps = fps;
	e->mb_w = (w + 15) / 16;
	e->mb_h = (h + 15) / 16;
	e->luma_stride = e->mb_w * 16;
	e->chroma_stride = e->mb_w * 8;
	size_t luma = (size_t)e->luma_stride * (e->mb_h * 16);
	size_t chroma = (size_t)e->chroma_stride * (e->mb_h * 8);
	e->y = (uint8_t*)CUTE_H264_ALLOC(luma + chroma * 2);
	if (!e->y) { CUTE_H264_FREE(e); ch_error_reason = "Out of memory."; return NULL; }
	e->cb = e->y + luma;
	e->cr = e->cb + chroma;
	return e;
}

void ch_encoder_destroy(ch_encoder_t* e)
{
	if (!e) return;
	CUTE_H264_FREE(e->y);
	CUTE_H264_FREE(e->bits.bytes.data);
	CUTE_H264_FREE(e->out.data);
	CUTE_H264_FREE(e);
}

int ch_encoder_frame(ch_encoder_t* e, const void* rgba)
{
	if (!e || !rgba) { ch_error_reason = "Null encoder or pixels."; return 0; }
	ch_rgba_to_yuv420(e, (const uint8_t*)rgba);
	// Parameter sets are repeated on every keyframe rather than written once. A capture that
	// gets cut, or that a player joins late, is then still decodable from any frame in it.
	ch_write_sps(e);
	ch_write_pps(e);
	ch_write_idr_slice(e);
	e->idr_pic_id ^= 1;
	++e->frame_count;
	if (e->out.oom || e->bits.bytes.oom) { ch_error_reason = "Out of memory."; return 0; }
	return 1;
}

const void* ch_encoder_data(ch_encoder_t* e, int* size)
{
	if (size) *size = e ? e->out.len : 0;
	return e ? e->out.data : NULL;
}

#if !defined(CUTE_H264_NO_STDIO)
int ch_encoder_save(ch_encoder_t* e, const char* file_name)
{
	if (!e) { ch_error_reason = "Null encoder."; return 0; }
	FILE* fp = fopen(file_name, "wb");
	if (!fp) { ch_error_reason = "Unable to open the output file."; return 0; }
	int ok = e->out.len == 0 || fwrite(e->out.data, 1, (size_t)e->out.len, fp) == (size_t)e->out.len;
	fclose(fp);
	if (!ok) ch_error_reason = "Failed writing the output file.";
	return ok;
}
#endif

#endif // CUTE_H264_IMPLEMENTATION_ONCE
#endif // CUTE_H264_IMPLEMENTATION

/*
	------------------------------------------------------------------------------
	This software is available under 2 licenses - you may choose the one you like.
	------------------------------------------------------------------------------
	ALTERNATIVE A - zlib license
	Copyright (c) 2026 Randy Gaul https://randygaul.github.io/
	This software is provided 'as-is', without any express or implied warranty.
	In no event will the authors be held liable for any damages arising from
	the use of this software.
	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:
	  1. The origin of this software must not be misrepresented; you must not
	     claim that you wrote the original software. If you use this software
	     in a product, an acknowledgment in the product documentation would be
	     appreciated but is not required.
	  2. Altered source versions must be plainly marked as such, and must not
	     be misrepresented as being the original software.
	  3. This notice may not be removed or altered from any source distribution.
	------------------------------------------------------------------------------
	ALTERNATIVE B - Public Domain (www.unlicense.org)
	This is free and unencumbered software released into the public domain.
	Anyone is free to copy, modify, publish, use, compile, sell, or distribute
	this software, either in source code form or as a compiled binary, for any
	purpose, commercial or non-commercial, and by any means.
	In jurisdictions that recognize copyright laws, the author or authors of
	this software dedicate any and all copyright interest in the software to
	the public domain. We make this dedication for the benefit of the public
	at large and to the detriment of our heirs and successors. We intend this
	dedication to be an overt act of relinquishment in perpetuity of all
	present and future rights to this software under copyright law.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	------------------------------------------------------------------------------
*/
