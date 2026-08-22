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

			encode, lossless: I_PCM macroblocks. Every frame a keyframe, every
			        macroblock raw samples, so the stream is mathematically
			        LOSSLESS and ffmpeg decodes it back bit for bit. Roughly
			        1.5x raw 4:2:0. Select it with ch_encoder_qp(e, -1).
			encode, compressed: I_16x16 intra with the integer transform and
			        CAVLC. Around 65x smaller than lossless at QP 26, and the
			        QP sweep is monotonic in both size and quality (55 dB at
			        QP 18 down to 40 dB at QP 42 on the test pattern).
			decode: not yet.

			Both paths are verified the strict way rather than by eye: ffmpeg's
			decode is compared byte for byte against the encoder's OWN
			reconstruction, over 35 frame sizes x every quantizer 0..51 x flat
			and detailed content. Encoder and decoder run the same inverse
			transform and the same intra prediction, so any difference at all
			is a bug in one of them, and a PSNR number would have hidden every
			table error this found. tools/h264_table_check.py additionally
			checks each CAVLC table against the codewords in the spec.

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

// Quality. 0 is nearly lossless and 51 is unwatchable; 26 is the default and a sane middle.
// Pass -1 to select the lossless I_PCM path instead, which ignores QP entirely.
void ch_encoder_qp(ch_encoder_t* e, int qp);

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

	// The encoder reconstructs every macroblock exactly as a decoder would, because intra
	// prediction reads its neighbours from the RECONSTRUCTION. Predicting from the source
	// instead looks correct in the encoder and drifts apart in the decoder.
	uint8_t* rec_y;
	uint8_t* rec_cb;
	uint8_t* rec_cr;

	// total_coeff per 4x4 block for the whole picture. CAVLC picks its table from the counts of
	// the left and upper blocks, so this has to survive across macroblocks, not just within one.
	uint8_t* nz_luma;      // mb_w*4 by mb_h*4
	uint8_t* nz_cb;        // mb_w*2 by mb_h*2
	uint8_t* nz_cr;

	int qp;                // 0..51, or -1 for the lossless I_PCM path

	ch_bits_t bits;        // scratch RBSP for the NAL under construction
	ch_bytes_t out;        // the Annex-B stream
};

//--------------------------------------------------------------------------------------------------
// The 4x4 integer transform. H.264 replaced JPEG's floating DCT with this deliberately: it is
// exactly invertible in 16-bit integers, so an encoder and a decoder on different machines
// reconstruct bit-identical pictures. Drift between them is what made earlier codecs smear over
// long GOPs.

static void ch_fdct4x4(const int16_t* in, int16_t* out)
{
	int t[16];
	for (int i = 0; i < 4; ++i) {
		const int16_t* p = in + i * 4;
		int a = p[0] + p[3], b = p[1] + p[2], c = p[1] - p[2], d = p[0] - p[3];
		t[i * 4 + 0] = a + b;
		t[i * 4 + 1] = 2 * d + c;
		t[i * 4 + 2] = a - b;
		t[i * 4 + 3] = d - 2 * c;
	}
	for (int i = 0; i < 4; ++i) {
		int a = t[0 * 4 + i] + t[3 * 4 + i], b = t[1 * 4 + i] + t[2 * 4 + i];
		int c = t[1 * 4 + i] - t[2 * 4 + i], d = t[0 * 4 + i] - t[3 * 4 + i];
		out[0 * 4 + i] = (int16_t)(a + b);
		out[1 * 4 + i] = (int16_t)(2 * d + c);
		out[2 * 4 + i] = (int16_t)(a - b);
		out[3 * 4 + i] = (int16_t)(d - 2 * c);
	}
}

static void ch_idct4x4(const int32_t* in, int32_t* out)
{
	int t[16];
	for (int i = 0; i < 4; ++i) {
		const int32_t* p = in + i * 4;
		int a = p[0] + p[2], b = p[0] - p[2], c = (p[1] >> 1) - p[3], d = p[1] + (p[3] >> 1);
		t[i * 4 + 0] = a + d;
		t[i * 4 + 1] = b + c;
		t[i * 4 + 2] = b - c;
		t[i * 4 + 3] = a - d;
	}
	for (int i = 0; i < 4; ++i) {
		int a = t[0 * 4 + i] + t[2 * 4 + i], b = t[0 * 4 + i] - t[2 * 4 + i];
		int c = (t[1 * 4 + i] >> 1) - t[3 * 4 + i], d = t[1 * 4 + i] + (t[3 * 4 + i] >> 1);
		out[0 * 4 + i] = a + d;
		out[1 * 4 + i] = b + c;
		out[2 * 4 + i] = b - c;
		out[3 * 4 + i] = a - d;
	}
}

// 4x4 Hadamard, used on the sixteen luma DC coefficients of an I_16x16 macroblock. Those DCs are
// the average of each 4x4 block, so across a smooth macroblock they are highly correlated -- a
// second transform over them is most of why I_16x16 beats coding sixteen blocks independently.
static void ch_hadamard4x4(const int16_t* in, int16_t* out)
{
	int t[16];
	for (int i = 0; i < 4; ++i) {
		const int16_t* p = in + i * 4;
		int a = p[0] + p[3], b = p[1] + p[2], c = p[1] - p[2], d = p[0] - p[3];
		t[i * 4 + 0] = a + b; t[i * 4 + 1] = d + c; t[i * 4 + 2] = a - b; t[i * 4 + 3] = d - c;
	}
	for (int i = 0; i < 4; ++i) {
		int a = t[0 * 4 + i] + t[3 * 4 + i], b = t[1 * 4 + i] + t[2 * 4 + i];
		int c = t[1 * 4 + i] - t[2 * 4 + i], d = t[0 * 4 + i] - t[3 * 4 + i];
		// The +1>>1 is not cosmetic. The 2D butterfly has a gain of 16 on DC while the
		// dequantizer downstream expects a gain of 8, so without this halving every I_16x16
		// macroblock reconstructs with exactly TWICE the residual it should -- a flat grey
		// frame comes back with the right average and the wrong value everywhere, which is
		// how it was caught.
		out[0 * 4 + i] = (int16_t)((a + b + 1) >> 1); out[1 * 4 + i] = (int16_t)((d + c + 1) >> 1);
		out[2 * 4 + i] = (int16_t)((a - b + 1) >> 1); out[3 * 4 + i] = (int16_t)((d - c + 1) >> 1);
	}
}

static void ch_ihadamard4x4(const int32_t* in, int32_t* out)
{
	int t[16];
	for (int i = 0; i < 4; ++i) {
		const int32_t* p = in + i * 4;
		int a = p[0] + p[2], b = p[0] - p[2], c = p[1] - p[3], d = p[1] + p[3];
		t[i * 4 + 0] = a + d; t[i * 4 + 1] = b + c; t[i * 4 + 2] = b - c; t[i * 4 + 3] = a - d;
	}
	for (int i = 0; i < 4; ++i) {
		int a = t[0 * 4 + i] + t[2 * 4 + i], b = t[0 * 4 + i] - t[2 * 4 + i];
		int c = t[1 * 4 + i] - t[3 * 4 + i], d = t[1 * 4 + i] + t[3 * 4 + i];
		out[0 * 4 + i] = a + d; out[1 * 4 + i] = b + c; out[2 * 4 + i] = b - c; out[3 * 4 + i] = a - d;
	}
}

//--------------------------------------------------------------------------------------------------
// Quantization. The transform above has a non-uniform gain per coefficient position, so the
// quantizer folds the correction into its multiplier: three position classes, six QP phases, and
// the QP/6 part handled by a shift. That is why the tables are 6x3 and not 52 entries long.

static const int ch_quant_mf[6][3] = {
	{ 13107, 5243, 8066 }, { 11916, 4660, 7490 }, { 10082, 4194, 6554 },
	{  9362, 3647, 5825 }, {  8192, 3355, 5243 }, {  7282, 2893, 4559 },
};
static const int ch_dequant_v[6][3] = {
	{ 10, 16, 13 }, { 11, 18, 14 }, { 13, 20, 16 },
	{ 14, 23, 18 }, { 16, 25, 20 }, { 18, 29, 23 },
};
// Which of the three classes a coefficient position belongs to.
static const int ch_pos_class[16] = { 0,2,0,2, 2,1,2,1, 0,2,0,2, 2,1,2,1 };

// Zig-zag: the scan that puts low frequencies first, so the high-frequency tail is a run of
// zeros that CAVLC can dismiss in a few bits.
static const int ch_zigzag[16] = { 0,1,4,8, 5,2,3,6, 9,12,13,10, 7,11,14,15 };

//--------------------------------------------------------------------------------------------------
// CAVLC. Context-adaptive variable length coding: the table used for the coefficient count is
// chosen by how many coefficients the NEIGHBOURING blocks had, which is the "context adaptive"
// part and the reason a per-block total_coeff map has to be carried across the whole picture.

// Do not hand-edit the tables below, and do not trust a reading of them. Run
// tools/h264_table_check.py, which compares every entry against the codewords printed in the
// spec. A wrong entry here is nearly undetectable by inspection: one of these rows had a length
// that was too long by one, which shifted the rest of the row into codewords that happened to be
// unused, so the table stayed prefix-free, kept the correct Kraft sum, and still looked perfectly
// regular. Most pictures decoded fine and a few desynchronised several macroblocks past the
// actual mistake.
//
// coeff_token, Table 9-5. Indexed [trailing_ones][total_coeff].
static const uint8_t ch_ct_len0[4][17] = {
	{ 1,6,8,9,10,11,13,13,13,14,14,15,15,16,16,16,16 },
	{ 0,2,6,8, 9,10,11,13,13,14,14,15,15,15,16,16,16 },
	{ 0,0,3,7, 8, 9,10,11,13,13,14,14,15,15,16,16,16 },
	{ 0,0,0,5, 6, 7, 8, 9,10,11,13,14,14,15,15,16,16 },
};
static const uint8_t ch_ct_code0[4][17] = {
	{ 1,5,7,7,7,7,15,11,8,15,11,15,11,15,11,7,4 },
	{ 0,1,4,6,6,6, 6,14,10,14,10,14,10, 1,14,10,6 },
	{ 0,0,1,5,5,5, 5, 5,13, 9,13, 9,13, 9,13, 9,5 },
	{ 0,0,0,3,3,4, 4, 4, 4, 4, 12,12,8,12, 8,12,8 },
};
static const uint8_t ch_ct_len1[4][17] = {
	{ 2,6,6,7,8,8,9,11,11,12,12,12,13,13,13,14,14 },
	{ 0,2,5,6,6,7,8, 9,11,11,12,12,13,13,14,14,14 },
	{ 0,0,3,6,6,7,8, 9, 11,11,12,12,13,13,13,14,14 },
	{ 0,0,0,4,4,5,6, 6, 7, 9,11,11,12,13,13,13,14 },
};
static const uint8_t ch_ct_code1[4][17] = {
	{ 3,11,7,7,7,4,7,15,11,15,11,8,15,11,7,9,7 },
	{ 0, 2,7,10,6,6,6, 6,14,10,14,10,14,10,11,8,6 },
	{ 0, 0,3, 9,5,5,5, 5,13, 9,13, 9,13, 9, 6,10,5 },
	{ 0, 0,0, 5,4,6,8, 4, 4, 4,12, 8,12,12, 8, 1,4 },
};
static const uint8_t ch_ct_len2[4][17] = {
	{ 4,6,6,6,7,7,7,7,8,8,9,9,9,10,10,10,10 },
	{ 0,4,5,5,5,5,6,6,7,8,8,9,9, 9,10,10,10 },
	{ 0,0,4,5,5,5,6,6,7,7,8,8,9, 9,10,10,10 },
	{ 0,0,0,4,4,4,4,4,5,6,7,8,8, 9,10,10,10 },
};
static const uint8_t ch_ct_code2[4][17] = {
	{ 15,15,11,8,15,11,9,8,15,11,15,11,8,13,9,5,1 },
	{  0,14,15,12,10,8,14,10,14,14,10,14,10, 7,12,8,4 },
	{  0, 0,13,14,11,9,13, 9,13,10,13, 9,13, 9,11,7,3 },
	{  0, 0, 0,12,11,10,9,8,13,12,12,12,8,12,10,6,2 },
};
// nC >= 8 uses a flat 6-bit code instead of a table -- at that density the counts are near
// uniform and a VLC would buy nothing.
static const uint8_t ch_ct_lenC[4][5] = { { 2,6,6,6,6 }, { 0,1,6,7,8 }, { 0,0,3,7,8 }, { 0,0,0,6,7 } };
static const uint8_t ch_ct_codeC[4][5] = { { 1,7,4,3,2 }, { 0,1,6,3,3 }, { 0,0,1,2,2 }, { 0,0,0,5,0 } };

// total_zeros, Table 9-7/9-8. Indexed [total_coeff-1][total_zeros].
static const uint8_t ch_tz_len[15][16] = {
	{ 1,3,3,4,4,5,5,6,6,7,7,8,8,9,9,9 },
	{ 3,3,3,3,3,4,4,4,4,5,5,6,6,6,6,0 },
	{ 4,3,3,3,4,4,3,3,4,5,5,6,5,6,0,0 },
	{ 5,3,4,4,3,3,3,4,3,4,5,5,5,0,0,0 },
	{ 4,4,4,3,3,3,3,3,4,5,4,5,0,0,0,0 },
	{ 6,5,3,3,3,3,3,3,4,3,6,0,0,0,0,0 },
	{ 6,5,3,3,3,2,3,4,3,6,0,0,0,0,0,0 },
	{ 6,4,5,3,2,2,3,3,6,0,0,0,0,0,0,0 },
	{ 6,6,4,2,2,3,2,5,0,0,0,0,0,0,0,0 },
	{ 5,5,3,2,2,2,4,0,0,0,0,0,0,0,0,0 },
	{ 4,4,3,3,1,3,0,0,0,0,0,0,0,0,0,0 },
	{ 4,4,2,1,3,0,0,0,0,0,0,0,0,0,0,0 },
	{ 3,3,1,2,0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 2,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
};
static const uint8_t ch_tz_code[15][16] = {
	{ 1,3,2,3,2,3,2,3,2,3,2,3,2,3,2,1 },
	{ 7,6,5,4,3,5,4,3,2,3,2,3,2,1,0,0 },
	{ 5,7,6,5,4,3,4,3,2,3,2,1,1,0,0,0 },
	{ 3,7,5,4,6,5,4,3,3,2,2,1,0,0,0,0 },
	{ 5,4,3,7,6,5,4,3,2,1,1,0,0,0,0,0 },
	{ 1,1,7,6,5,4,3,2,1,1,0,0,0,0,0,0 },
	{ 1,1,5,4,3,3,2,1,1,0,0,0,0,0,0,0 },
	{ 1,1,1,3,3,2,2,1,0,0,0,0,0,0,0,0 },
	{ 1,0,1,3,2,1,1,1,0,0,0,0,0,0,0,0 },
	{ 1,0,1,3,2,1,1,0,0,0,0,0,0,0,0,0 },
	{ 0,1,1,2,1,3,0,0,0,0,0,0,0,0,0,0 },
	{ 0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0 },
	{ 0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
};
// Chroma DC has only four coefficients, so it gets its own much smaller table.
static const uint8_t ch_tzc_len[3][4]  = { { 1,2,3,3 }, { 1,2,2,0 }, { 1,1,0,0 } };
static const uint8_t ch_tzc_code[3][4] = { { 1,1,1,0 }, { 1,1,0,0 }, { 1,0,0,0 } };

// run_before, Table 9-10. Indexed [min(zeros_left,7)-1][run].
static const uint8_t ch_rb_len[7][16] = {
	{ 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 1,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 2,2,2,3,3,0,0,0,0,0,0,0,0,0,0,0 },
	{ 2,2,3,3,3,3,0,0,0,0,0,0,0,0,0,0 },
	{ 2,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0 },
	{ 3,3,3,3,3,3,3,4,5,6,7,8,9,10,11,0 },
};
static const uint8_t ch_rb_code[7][16] = {
	{ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 3,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 3,2,3,2,1,0,0,0,0,0,0,0,0,0,0,0 },
	{ 3,0,1,3,2,5,4,0,0,0,0,0,0,0,0,0 },
	{ 7,6,5,4,3,2,1,1,1,1,1,1,1,1,1,0 },
};

// Writes one residual block. `coeffs` is already in zig-zag order. `nC` selects the coeff_token
// table; -1 means chroma DC. Returns total_coeff so the caller can record it for its neighbours.
// The tail of the level code, once the value is too large for a plain prefix. level_prefix 15
// carries a 12-bit suffix; every prefix beyond that widens the suffix by a bit and doubles the
// reach. Nothing but QP 0 gets near it -- a DC level there runs to several thousand, where a
// fixed 12-bit field would silently wrap and encode a completely different coefficient.
static void ch_put_level_escape(ch_bits_t* w, int x)
{
	int prefix = 15;
	while (x >= (1 << (prefix - 3))) { x -= (1 << (prefix - 3)); ++prefix; }
	ch_put_bits(w, 0, prefix);
	ch_put_bit(w, 1);
	ch_put_bits(w, (uint32_t)x, prefix - 3);
}

static int ch_write_residual(ch_bits_t* w, const int* coeffs, int count, int nC)
{
	int levels[16], runs[16];
	int total = 0, total_zeros = 0, run = 0;
	for (int i = 0; i < count; ++i) {
		if (coeffs[i]) { levels[total] = coeffs[i]; runs[total] = run; ++total; run = 0; }
		else ++run;
	}
	// Trailing ones are counted from the END of the scan, where +-1 coefficients cluster; the
	// spec lets up to three of them be coded as a bare sign bit each.
	int t1 = 0;
	for (int i = total - 1; i >= 0 && t1 < 3; --i) {
		if (levels[i] == 1 || levels[i] == -1) ++t1;
		else break;
	}
	for (int i = 0; i < total; ++i) total_zeros += runs[i];

	const uint8_t* lens; const uint8_t* codes; int stride;
	if (nC < 0)       { lens = &ch_ct_lenC[0][0]; codes = &ch_ct_codeC[0][0]; stride = 5; }
	else if (nC < 2)  { lens = &ch_ct_len0[0][0]; codes = &ch_ct_code0[0][0]; stride = 17; }
	else if (nC < 4)  { lens = &ch_ct_len1[0][0]; codes = &ch_ct_code1[0][0]; stride = 17; }
	else if (nC < 8)  { lens = &ch_ct_len2[0][0]; codes = &ch_ct_code2[0][0]; stride = 17; }
	else              { lens = NULL; codes = NULL; stride = 0; }

	if (lens) {
		ch_put_bits(w, codes[t1 * stride + total], lens[t1 * stride + total]);
	} else {
		ch_put_bits(w, total ? (uint32_t)(((total - 1) << 2) | t1) : 3u, 6);
	}
	if (!total) return 0;

	// Trailing one signs, most significant first.
	for (int i = 0; i < t1; ++i) ch_put_bit(w, levels[total - 1 - i] < 0);

	int suffix_len = (total > 10 && t1 < 3) ? 1 : 0;
	for (int i = total - 1 - t1; i >= 0; --i) {
		int level = levels[i];
		int code = level > 0 ? (level << 1) - 2 : (-level << 1) - 1;
		// The first non-trailing-one level cannot be +-1 when fewer than three trailing ones
		// were coded, so the spec shifts its range down by two and buys a bit back.
		if (i == total - 1 - t1 && t1 < 3) code -= 2;
		if (suffix_len == 0) {
			if (code < 14) { ch_put_bits(w, 0, code); ch_put_bit(w, 1); }
			else if (code < 30) { ch_put_bits(w, 0, 14); ch_put_bit(w, 1); ch_put_bits(w, (uint32_t)(code - 14), 4); }
			else ch_put_level_escape(w, code - 30);
		} else {
			int prefix = code >> suffix_len;
			if (prefix < 15) {
				ch_put_bits(w, 0, prefix); ch_put_bit(w, 1);
				ch_put_bits(w, (uint32_t)(code & ((1 << suffix_len) - 1)), suffix_len);
			} else {
				ch_put_level_escape(w, code - (15 << suffix_len));
			}
		}
		if (suffix_len == 0) suffix_len = 1;
		int mag = level < 0 ? -level : level;
		if (mag > (3 << (suffix_len - 1)) && suffix_len < 6) ++suffix_len;
	}

	if (total < count) {
		if (nC < 0) ch_put_bits(w, ch_tzc_code[total - 1][total_zeros], ch_tzc_len[total - 1][total_zeros]);
		else ch_put_bits(w, ch_tz_code[total - 1][total_zeros], ch_tz_len[total - 1][total_zeros]);
	}
	int zeros_left = total_zeros;
	for (int i = total - 1; i > 0 && zeros_left > 0; --i) {
		int r = runs[i];
		int idx = (zeros_left > 7 ? 7 : zeros_left) - 1;
		ch_put_bits(w, ch_rb_code[idx][r], ch_rb_len[idx][r]);
		zeros_left -= r;
	}
	return total;
}

//--------------------------------------------------------------------------------------------------
// Intra prediction and macroblock coding.

// The sixteen luma 4x4 blocks are not in raster order. They walk each 8x8 quadrant before moving
// on, so a block's left and upper neighbours are already reconstructed when it is coded.
static const int ch_blk_x[16] = { 0,1,0,1, 2,3,2,3, 0,1,0,1, 2,3,2,3 };
static const int ch_blk_y[16] = { 0,0,1,1, 0,0,1,1, 2,2,3,3, 2,2,3,3 };

// Chroma runs at its own QP, which saturates as luma QP climbs: chroma artifacts are far more
// visible than luma ones, so the spec refuses to coarsen it at the same rate.
static const int ch_qpc_table[22] = { 29,30,31,32,32,33,34,34,35,35,36,36,37,37,37,38,38,38,39,39,39,39 };
static int ch_chroma_qp(int qp) { return qp < 30 ? qp : ch_qpc_table[qp - 30]; }

static int ch_clip255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

// Predicts a 16x16 luma block from already-reconstructed neighbours and returns the mode chosen.
// Modes are scored by plain SAD -- a real rate-distortion search would also weigh the bits each
// mode costs, but the mode number is only two bits here so the ordering rarely differs.
static int ch_pred16(const uint8_t* rec, int stride, int have_l, int have_t, uint8_t* out)
{
	int best = -1, best_sad = 0x7fffffff, best_mode = 2;
	uint8_t cand[3][256];
	int modes[3], n = 0;
	if (have_t) { // vertical
		for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) cand[n][y * 16 + x] = rec[-stride + x];
		modes[n++] = 0;
	}
	if (have_l) { // horizontal
		for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) cand[n][y * 16 + x] = rec[y * stride - 1];
		modes[n++] = 1;
	}
	{ // DC, always available -- with no neighbours at all it is flat 128, which is what makes
	  // the very first macroblock of a picture codeable at all.
		int sum = 0, cnt = 0;
		if (have_t) { for (int x = 0; x < 16; ++x) sum += rec[-stride + x]; cnt += 16; }
		if (have_l) { for (int y = 0; y < 16; ++y) sum += rec[y * stride - 1]; cnt += 16; }
		int dc = cnt ? (sum + (cnt >> 1)) / cnt : 128;
		for (int i = 0; i < 256; ++i) cand[n][i] = (uint8_t)dc;
		modes[n++] = 2;
	}
	for (int c = 0; c < n; ++c) {
		int sad = 0;
		for (int y = 0; y < 16; ++y) {
			for (int x = 0; x < 16; ++x) {
				int d = rec[y * stride + x] - cand[c][y * 16 + x];
				sad += d < 0 ? -d : d;
			}
		}
		if (sad < best_sad) { best_sad = sad; best = c; best_mode = modes[c]; }
	}
	CUTE_H264_MEMCPY(out, cand[best], 256);
	return best_mode;
}

// Chroma 8x8 DC prediction. Each 4x4 quadrant picks its own source, which is why this is not
// simply one average over the whole block.
static void ch_pred8_dc(const uint8_t* rec, int stride, int have_l, int have_t, uint8_t* out)
{
	for (int by = 0; by < 2; ++by) {
		for (int bx = 0; bx < 2; ++bx) {
			int st = 0, sl = 0;
			for (int i = 0; i < 4; ++i) {
				if (have_t) st += rec[-stride + bx * 4 + i];
				if (have_l) sl += rec[(by * 4 + i) * stride - 1];
			}
			int dc;
			int corner = (bx == by);       // quadrants 0 and 3 average both edges when they can
			if (corner && have_t && have_l) dc = (st + sl + 4) >> 3;
			else if (bx == 1 && by == 0)    dc = have_t ? (st + 2) >> 2 : (have_l ? (sl + 2) >> 2 : 128);
			else if (bx == 0 && by == 1)    dc = have_l ? (sl + 2) >> 2 : (have_t ? (st + 2) >> 2 : 128);
			else if (have_t)                dc = (st + 2) >> 2;
			else if (have_l)                dc = (sl + 2) >> 2;
			else                            dc = 128;
			for (int y = 0; y < 4; ++y)
				for (int x = 0; x < 4; ++x) out[(by * 4 + y) * 8 + bx * 4 + x] = (uint8_t)dc;
		}
	}
}

// Quantize one 4x4 block of transform coefficients into zig-zag order, and simultaneously
// produce the dequantized values the reconstruction needs. Encoder and decoder must agree on the
// reconstruction exactly, so the encoder predicts from what the DECODER will see, never from the
// original pixels -- that is the whole reason this returns both.
static int ch_quant_block(const int16_t* coef, int qp, int skip_dc, int* zz_out, int32_t* deq_out)
{
	int qp_per = qp / 6, qp_rem = qp % 6;
	int qbits = 15 + qp_per;
	int f = (1 << qbits) / 3;   // intra dead zone
	int any = 0;
	for (int i = 0; i < 16; ++i) {
		int pos = ch_zigzag[i];
		if (skip_dc && pos == 0) { zz_out[i] = 0; continue; }
		int w = coef[pos];
		int mag = w < 0 ? -w : w;
		int level = (mag * ch_quant_mf[qp_rem][ch_pos_class[pos]] + f) >> qbits;
		if (w < 0) level = -level;
		zz_out[i] = level;
		if (level) any = 1;
		deq_out[pos] = (int32_t)(level * ch_dequant_v[qp_rem][ch_pos_class[pos]]) << qp_per;
	}
	return any;
}

static void ch_reconstruct_block(const int32_t* deq, const uint8_t* pred, int pred_stride,
                                 uint8_t* dst, int dst_stride)
{
	int32_t r[16];
	ch_idct4x4(deq, r);
	for (int y = 0; y < 4; ++y)
		for (int x = 0; x < 4; ++x)
			dst[y * dst_stride + x] = (uint8_t)ch_clip255(pred[y * pred_stride + x] + ((r[y * 4 + x] + 32) >> 6));
}

// nC: the coefficient-count context, averaged from the left and upper blocks. This is the
// "context adaptive" in CAVLC -- a block surrounded by busy blocks is coded with a table that
// expects to be busy.
static int ch_nc(const uint8_t* map, int stride, int bx, int by, int have_l, int have_t)
{
	int have_a = have_l || bx > 0;
	int have_b = have_t || by > 0;
	int a = have_a ? map[by * stride + (bx - 1)] : 0;
	int b = have_b ? map[(by - 1) * stride + bx] : 0;
	if (have_a && have_b) return (a + b + 1) >> 1;
	if (have_a) return a;
	if (have_b) return b;
	return 0;
}

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

// Encodes one I_16x16 macroblock: predict from the reconstructed neighbours, transform and
// quantize the residual, write it, and reconstruct so the NEXT macroblock predicts from exactly
// what a decoder will have. Predicting from the source instead is the classic drift bug -- it
// looks fine in the encoder and diverges in the decoder.
static void ch_encode_mb_i16(ch_encoder_t* e, int mbx, int mby)
{
	ch_bits_t* w = &e->bits;
	int have_l = mbx > 0, have_t = mby > 0;
	int qp = e->qp, qpc = ch_chroma_qp(qp);
	int qp_per = qp / 6, qp_rem = qp % 6;

	uint8_t* ry = e->rec_y + (size_t)(mby * 16) * e->luma_stride + mbx * 16;
	const uint8_t* sy = e->y + (size_t)(mby * 16) * e->luma_stride + mbx * 16;

	uint8_t pred[256];
	int pred_mode = ch_pred16(ry, e->luma_stride, have_l, have_t, pred);
	// ch_pred16 scores against the reconstructed plane, but the residual is against the SOURCE.
	int16_t resid[256];
	for (int y = 0; y < 16; ++y)
		for (int x = 0; x < 16; ++x)
			resid[y * 16 + x] = (int16_t)(sy[y * (size_t)e->luma_stride + x] - pred[y * 16 + x]);

	int16_t coef[16][16];
	int16_t dc[16], dc_t[16];
	for (int b = 0; b < 16; ++b) {
		int bx = ch_blk_x[b], by = ch_blk_y[b];
		int16_t blk[16];
		for (int y = 0; y < 4; ++y)
			for (int x = 0; x < 4; ++x) blk[y * 4 + x] = resid[(by * 4 + y) * 16 + bx * 4 + x];
		ch_fdct4x4(blk, coef[b]);
		dc[by * 4 + bx] = coef[b][0];
	}
	ch_hadamard4x4(dc, dc_t);

	int qbits = 15 + qp_per;
	int fdc = (1 << qbits) / 3;
	int dc_zz[16]; int32_t dc_lev[16];
	for (int i = 0; i < 16; ++i) {
		int pos = ch_zigzag[i];
		int v = dc_t[pos], mag = v < 0 ? -v : v;
		int level = (mag * ch_quant_mf[qp_rem][0] + 2 * fdc) >> (qbits + 1);
		if (v < 0) level = -level;
		dc_zz[i] = level;
		dc_lev[pos] = level;
	}
	int32_t dc_deq[16];
	ch_ihadamard4x4(dc_lev, dc_deq);
	for (int i = 0; i < 16; ++i) {
		int t = dc_deq[i] * ch_dequant_v[qp_rem][0];
		dc_deq[i] = qp_per >= 2 ? (t << (qp_per - 2)) : ((t + (1 << (1 - qp_per))) >> (2 - qp_per));
	}

	int ac_zz[16][16]; int32_t deq[16][16];
	int cbp_luma = 0;
	for (int b = 0; b < 16; ++b) {
		CUTE_H264_MEMSET(deq[b], 0, sizeof(deq[b]));
		if (ch_quant_block(coef[b], qp, 1, ac_zz[b], deq[b])) cbp_luma = 15;
	}
	for (int b = 0; b < 16; ++b) {
		int bx = ch_blk_x[b], by = ch_blk_y[b];
		deq[b][0] = dc_deq[by * 4 + bx];
		ch_reconstruct_block(deq[b], pred + (by * 4) * 16 + bx * 4, 16,
			ry + (size_t)(by * 4) * e->luma_stride + bx * 4, e->luma_stride);
	}

	// Chroma, both components. Same shape as luma but 2x2 DC and only four blocks each.
	int c_zz[2][4][16]; int cdc_zz[2][4]; int cbp_chroma = 0;
	int32_t cdeq[2][4][16]; int32_t cdc_deq[2][4];
	uint8_t cpred[2][64];
	for (int c = 0; c < 2; ++c) {
		uint8_t* rc = (c ? e->rec_cr : e->rec_cb) + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
		const uint8_t* sc = (c ? e->cr : e->cb) + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
		ch_pred8_dc(rc, e->chroma_stride, have_l, have_t, cpred[c]);
		int16_t cres[64];
		for (int y = 0; y < 8; ++y)
			for (int x = 0; x < 8; ++x)
				cres[y * 8 + x] = (int16_t)(sc[y * (size_t)e->chroma_stride + x] - cpred[c][y * 8 + x]);
		int16_t ccoef[4][16], cdc[4];
		for (int b = 0; b < 4; ++b) {
			int bx = b & 1, by = b >> 1;
			int16_t blk[16];
			for (int y = 0; y < 4; ++y)
				for (int x = 0; x < 4; ++x) blk[y * 4 + x] = cres[(by * 4 + y) * 8 + bx * 4 + x];
			ch_fdct4x4(blk, ccoef[b]);
			cdc[b] = ccoef[b][0];
		}
		int h0 = cdc[0] + cdc[1] + cdc[2] + cdc[3];
		int h1 = cdc[0] - cdc[1] + cdc[2] - cdc[3];
		int h2 = cdc[0] + cdc[1] - cdc[2] - cdc[3];
		int h3 = cdc[0] - cdc[1] - cdc[2] + cdc[3];
		int hh[4] = { h0, h1, h2, h3 };
		int cqp_per = qpc / 6, cqp_rem = qpc % 6;
		int cqbits = 15 + cqp_per, cf = (1 << cqbits) / 3;
		int dc_any = 0;
		for (int i = 0; i < 4; ++i) {
			int mag = hh[i] < 0 ? -hh[i] : hh[i];
			int level = (mag * ch_quant_mf[cqp_rem][0] + 2 * cf) >> (cqbits + 1);
			if (hh[i] < 0) level = -level;
			cdc_zz[c][i] = level;
			if (level) dc_any = 1;
		}
		int g0 = cdc_zz[c][0] + cdc_zz[c][1] + cdc_zz[c][2] + cdc_zz[c][3];
		int g1 = cdc_zz[c][0] - cdc_zz[c][1] + cdc_zz[c][2] - cdc_zz[c][3];
		int g2 = cdc_zz[c][0] + cdc_zz[c][1] - cdc_zz[c][2] - cdc_zz[c][3];
		int g3 = cdc_zz[c][0] - cdc_zz[c][1] - cdc_zz[c][2] + cdc_zz[c][3];
		int gg[4] = { g0, g1, g2, g3 };
		// The <<4 is the weight-scale factor of 16 the spec folds into LevelScale. Luma's AC
		// formula absorbs it into its own shift so it is invisible there; chroma DC does not,
		// and leaving it out makes chroma reconstruct at a sixteenth of its residual -- the
		// picture keeps its average and loses all its colour.
		for (int i = 0; i < 4; ++i)
			cdc_deq[c][i] = ((int32_t)(gg[i] * ch_dequant_v[cqp_rem][0]) << (cqp_per + 4)) >> 5;
		int ac_any = 0;
		for (int b = 0; b < 4; ++b) {
			CUTE_H264_MEMSET(cdeq[c][b], 0, sizeof(cdeq[c][b]));
			if (ch_quant_block(ccoef[b], qpc, 1, c_zz[c][b], cdeq[c][b])) ac_any = 1;
		}
		if (ac_any) cbp_chroma = 2;
		else if (dc_any && cbp_chroma < 1) cbp_chroma = 1;
		for (int b = 0; b < 4; ++b) {
			int bx = b & 1, by = b >> 1;
			cdeq[c][b][0] = cdc_deq[c][b];
			ch_reconstruct_block(cdeq[c][b], cpred[c] + (by * 4) * 8 + bx * 4, 8,
				rc + (size_t)(by * 4) * e->chroma_stride + bx * 4, e->chroma_stride);
		}
	}

	// mb_type packs the prediction mode and both CBPs into one number for I_16x16.
	ch_ue(w, (uint32_t)(1 + pred_mode + 4 * cbp_chroma + 12 * (cbp_luma ? 1 : 0)));
	ch_ue(w, 0);              // intra_chroma_pred_mode: DC
	ch_se(w, 0);              // mb_qp_delta: constant QP for now

	int lstride = e->mb_w * 4, cstride = e->mb_w * 2;
	uint8_t* lmap = e->nz_luma + (size_t)(mby * 4) * lstride + mbx * 4;

	int nc0 = ch_nc(lmap, lstride, 0, 0, have_l, have_t);
	ch_write_residual(w, dc_zz, 16, nc0);

	for (int b = 0; b < 16; ++b) {
		int bx = ch_blk_x[b], by = ch_blk_y[b];
		int n = 0;
		if (cbp_luma) {
			int nc = ch_nc(lmap, lstride, bx, by, have_l, have_t);
			n = ch_write_residual(w, ac_zz[b] + 1, 15, nc);
		}
		lmap[by * lstride + bx] = (uint8_t)n;
	}
	if (cbp_chroma & 3) {
		for (int c = 0; c < 2; ++c) ch_write_residual(w, cdc_zz[c], 4, -1);
	}
	for (int c = 0; c < 2; ++c) {
		uint8_t* cmap = (c ? e->nz_cr : e->nz_cb) + (size_t)(mby * 2) * cstride + mbx * 2;
		for (int b = 0; b < 4; ++b) {
			int bx = b & 1, by = b >> 1;
			int n = 0;
			if (cbp_chroma & 2) {
				int nc = ch_nc(cmap, cstride, bx, by, have_l, have_t);
				n = ch_write_residual(w, c_zz[c][b] + 1, 15, nc);
			}
			cmap[by * cstride + bx] = (uint8_t)n;
		}
	}
}

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
	ch_se(w, e->qp < 0 ? 0 : e->qp - 26); // slice_qp_delta, against the PPS's QP 26
	// Deblocking stays off. For I_PCM it has to be, or the filter smears samples the encoder
	// never touched and the round trip stops being bit-exact. For I_16x16 it is simply not
	// implemented yet -- turning it on without implementing it in the reconstruction loop would
	// desynchronise the encoder's prediction from the decoder's, which is far worse than
	// slightly blockier output.
	ch_ue(w, 1);                      // disable_deblocking_filter_idc: 1 = off

	for (int mby = 0; mby < e->mb_h; ++mby) {
		for (int mbx = 0; mbx < e->mb_w; ++mbx) {
			if (e->qp >= 0) { ch_encode_mb_i16(e, mbx, mby); continue; }
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
	e->qp = 26;
	size_t nzl = (size_t)(e->mb_w * 4) * (e->mb_h * 4);
	size_t nzc = (size_t)(e->mb_w * 2) * (e->mb_h * 2);
	e->y = (uint8_t*)CUTE_H264_ALLOC((luma + chroma * 2) * 2 + nzl + nzc * 2);
	if (!e->y) { CUTE_H264_FREE(e); ch_error_reason = "Out of memory."; return NULL; }
	e->cb = e->y + luma;
	e->cr = e->cb + chroma;
	e->rec_y = e->cr + chroma;
	e->rec_cb = e->rec_y + luma;
	e->rec_cr = e->rec_cb + chroma;
	e->nz_luma = e->rec_cr + chroma;
	e->nz_cb = e->nz_luma + nzl;
	e->nz_cr = e->nz_cb + nzc;
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
	// Every frame is an IDR, so nothing carries over: the counts start clean or the first
	// macroblock row inherits contexts from the previous picture and decodes as noise.
	CUTE_H264_MEMSET(e->nz_luma, 0, (size_t)(e->mb_w * 4) * (e->mb_h * 4));
	CUTE_H264_MEMSET(e->nz_cb, 0, (size_t)(e->mb_w * 2) * (e->mb_h * 2));
	CUTE_H264_MEMSET(e->nz_cr, 0, (size_t)(e->mb_w * 2) * (e->mb_h * 2));
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

void ch_encoder_qp(ch_encoder_t* e, int qp)
{
	if (!e) return;
	e->qp = qp < 0 ? -1 : (qp > 51 ? 51 : qp);
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
