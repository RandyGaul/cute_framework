/*
	------------------------------------------------------------------------------
		Licensing information can be found at the end of the file.
	------------------------------------------------------------------------------

	cute_h264.h - v0.02

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
			encode, compressed: I_16x16 and I_4x4 intra with the integer
			        transform and CAVLC. Every macroblock is encoded both ways
			        and the cheaper one kept, weighing squared error against
			        bits rather than counting bits alone -- the two types do
			        not produce the same quality at a given QP, so comparing
			        size would systematically pick the wrong one. I_4x4 is
			        worth about 8.5% fewer bits at matched quality, measured
			        against the I_16x16-only encoder on a rate/quality curve
			        rather than at matched QP, which flatters it.
			encode, inter: P macroblocks predicted from the previous frame at
			        quarter-sample precision, split into 16x16, 16x8, 8x16 or
			        four 8x8 pieces as the motion warrants, plus P_Skip. A keyframe every two
			        seconds so a capture can still be seeked. Worth 32% fewer
			        bits at matched quality on hard-edged content that moves by
			        whole pixels and 50% on smooth content that does not; a
			        still picture costs about ten bytes a frame.
			deblocking: on, except for the lossless path. Bitrate is unchanged
			        within about 1%, for 0.4 to 0.5 dB at mid-to-high quality.
			        At very high QP it costs a little PSNR on synthetic
			        hard-edged content -- which is the region where its real
			        job, not showing block boundaries, is something PSNR does
			        not measure either way.
			decode: I and P slices, every intra mode including the two this
			        encoder never chooses, inter prediction, skipped
			        macroblocks and deblocking. It reads streams from other
			        encoders, not just this one -- x264 output decodes bit-
			        exactly against ffmpeg. What is not supported is rejected
			        by name rather than decoded approximately: CABAC, more
			        than one reference frame, interlacing. Every partition and
			        sub-partition shape decodes, including the ones this
			        encoder never produces.
			container: MP4, so the output opens in players, editors and
			        browsers rather than only in ffmpeg. The picture data goes
			        in unchanged -- this is packaging, not re-encoding -- and
			        costs about 0.2%. Raw Annex-B is still available.

			The decoder shares the reconstruction code rather than
			reimplementing it. That is not a shortcut: the encoder already had
			to reconstruct exactly as a decoder does in order to predict from
			the right samples, so prediction, the inverse transform, motion
			compensation and deblocking were already written and already
			verified. What the decoder adds is reading the bitstream.

			Every path is verified the strict way rather than by eye: ffmpeg's
			decode is compared byte for byte against the encoder's OWN
			reconstruction, over 35 frame sizes x every quantizer 0..51 x three
			kinds of content. Encoder and decoder run the same inverse transform,
			the same intra prediction and the same interpolation, so any
			difference at all is a bug in one of them, and a PSNR number would
			have hidden every table error this found.
			tools/h264_table_check.py additionally checks each CAVLC table
			against the codewords in the spec.

			The decoder is held to the same standard, twice over: it reproduces
			the encoder's reconstruction exactly across that same sweep, and it
			reproduces ffmpeg's output exactly on streams x264 produced. The
			second one is the test that matters most, because an encoder and a
			decoder written together can agree with each other and both be
			wrong -- and it caught exactly that, in chroma intra prediction,
			where this encoder only ever emits one of the four modes.

			Aggregate conformance is not enough on its own, because it only
			exercises what the encoder's own decisions happen to select. Each of
			the nine Intra_4x4 modes and each of the sixteen sub-sample
			interpolation positions is therefore also forced on in isolation and
			checked bit-exact, so a mode the rate-distortion search never picks
			is still proven rather than merely unused.

		I_PCM first is deliberate, not a shortcut that got left in. It exercises
		every part of the bitstream that the compressed paths also depend on --
		NAL framing, emulation prevention, exp-golomb, the SPS/PPS/slice headers,
		the macroblock loop -- with no transform, no prediction and no entropy
		coder to be simultaneously wrong. Once a decoder agrees with it byte for
		byte, everything after this is a change to macroblock CONTENTS on a
		container that is already known-good. It also leaves a genuinely useful
		lossless capture mode behind once the compressed paths land.

		Measured against x264 at matched quality on three clips: restricted to
		the same tools this encoder has, it is within about 10% either way, so
		the coding decisions are sound. Against x264 with everything switched on
		it is 32-55% larger, and that gap is the features below rather than the
		decisions. In order of what they buy: CABAC; B frames; multiple
		reference frames.

		Encoding runs at roughly 17 frames a second at 640x360 on one core with
		no SIMD anywhere. Searching every partition shape exhaustively costs
		four times that for a few percent, so the search abandons a shape once
		it cannot win and does not split a macroblock the single vector already
		predicts well.

	EXAMPLES:

		Encoding frames to a file
			ch_encoder_t* e = ch_encoder_make(1280, 720, 30);
			for (int i = 0; i < frame_count; ++i) {
				ch_encoder_frame(e, rgba_pixels[i]);
			}
			ch_encoder_save_mp4(e, "capture.mp4");   // or ch_encoder_save for raw Annex-B
			ch_encoder_destroy(e);

		Encoding to memory instead
			int size;
			const void* bytes = ch_encoder_data(e, &size);

		Decoding a stream back to pixels
			ch_decoder_t* d = ch_decoder_make(bytes, size);
			int w, h;
			while (ch_decoder_next(d)) {
				ch_decoder_size(d, &w, &h);
				const void* rgba = ch_decoder_rgba(d);   // w*h*4, valid until the next call
			}
			if (ch_decoder_error) printf("%s\n", ch_decoder_error);
			ch_decoder_destroy(d);

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
		0.02 (08/22/2026) intra, inter and deblocking encode; decode
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

//--------------------------------------------------------------------------------------------------
// MP4 container. A raw H.264 stream is a stream of pictures and nothing else -- no duration, no
// frame rate, no seek index, nowhere for audio. Most software will not open one. These wrap the
// same picture data, unchanged, in the file format everything expects.

// Writes the stream as an .mp4. This is the one to use unless you specifically want Annex-B.
int ch_encoder_save_mp4(ch_encoder_t* e, const char* file_name);

// The same, from a stream you already have, returning bytes you must free with CUTE_H264_FREE.
const void* ch_mp4_wrap(const void* annexb, int size, int w, int h, int fps, int* out_size);

//--------------------------------------------------------------------------------------------------
// Decoding.

// Wraps an Annex-B stream. The bytes are NOT copied and must outlive the decoder.
typedef struct ch_decoder_t ch_decoder_t;
ch_decoder_t* ch_decoder_make(const void* annexb, int size);
void ch_decoder_destroy(ch_decoder_t* d);

// Decodes the next picture. Returns 0 at the end of the stream, and also on an error -- check
// ch_decoder_error to tell the two apart.
int ch_decoder_next(ch_decoder_t* d);

// The coded size, available once the first parameter set has been read.
int ch_decoder_size(ch_decoder_t* d, int* w, int* h);

// The picture just decoded, as w*h*4 RGBA bytes owned by the decoder. Valid until the next call.
const void* ch_decoder_rgba(ch_decoder_t* d);

// The same picture without the colour conversion: the luma plane is returned, chroma through the
// out parameters. Planes are padded out to whole macroblocks, hence the strides.
const void* ch_decoder_yuv(ch_decoder_t* d, int* luma_stride, int* chroma_stride,
                           const void** cb, const void** cr);

// Why the last call failed, and null if it did not. Cleared at the start of each
// ch_decoder_next, so a zero return with this still null means the stream simply ended.
extern const char* ch_decoder_error;

//--------------------------------------------------------------------------------------------------

// The same, for callers that already have planar 4:2:0 and would otherwise convert to RGB and
// back for nothing. Chroma planes are half size in both directions.
int ch_encoder_frame_yuv(ch_encoder_t* e, const void* y, const void* cb, const void* cr,
                         int y_stride, int chroma_stride);

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

#define CH_NAL_SLICE     1
#define CH_NAL_SLICE_IDR 5
#define CH_NAL_SPS       7
#define CH_NAL_PPS       8

// Everything that describes a decoded picture and the data a reconstruction needs to reproduce
// it. Split out of the encoder because a decoder needs exactly the same, and the reconstruction
// half of this file -- prediction, motion compensation, deblocking -- is then shared rather than
// written twice and drifting apart.
typedef struct ch_pic_t
{
	int mb_w, mb_h;
	int luma_stride, chroma_stride;

	// The reconstruction before the deblocking filter, which is what intra prediction reads, and
	// the picture after it, which is what a decoder outputs and what the next frame predicts from.
	uint8_t* rec_y;
	uint8_t* rec_cb;
	uint8_t* rec_cr;
	uint8_t* ref_y;
	uint8_t* ref_cb;
	uint8_t* ref_cr;

	uint8_t* nz_luma;      // per 4x4 block: CAVLC context, and deblocking strength
	int16_t* mv;           // two per 4x4 block, quarter samples
	int8_t* ref_idx;       // -1 marks a block coded intra
} ch_pic_t;

struct ch_encoder_t
{
	ch_pic_t pic;          // a view of the buffers below, for the shared reconstruction code

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

	// The previous frame's reconstruction, which is what P macroblocks predict from. Swapped with
	// the rec_ planes at the end of each frame rather than copied.
	uint8_t* ref_y;
	uint8_t* ref_cb;
	uint8_t* ref_cr;

	// total_coeff per 4x4 block for the whole picture. CAVLC picks its table from the counts of
	// the left and upper blocks, so this has to survive across macroblocks, not just within one.
	uint8_t* nz_luma;      // mb_w*4 by mb_h*4
	uint8_t* nz_cb;        // mb_w*2 by mb_h*2
	uint8_t* nz_cr;

	// Intra4x4PredMode per 4x4 block for the whole picture. A block's mode is coded relative to
	// its left and upper neighbours, so like the coefficient counts this crosses macroblocks.
	// Macroblocks that are not I_NxN store 2 (DC), which is what the spec says to predict from.
	uint8_t* i4_mode;      // mb_w*4 by mb_h*4

	// Motion per 4x4 block, in quarter samples, with -1 in ref_idx marking a block that was coded
	// intra. A macroblock's vector is sent as a difference from a prediction made out of this, so
	// it has to survive across macroblocks the same way the coefficient counts do.
	int16_t* mv;           // two per block
	int8_t* ref_idx;

	int frame_num;         // as the slice header carries it, reset by each IDR
	int mb_type_offset;    // intra mb_type is biased by 5 inside a P slice

	int qp;                // 0..51, or -1 for the lossless I_PCM path

	ch_bits_t bits;        // scratch RBSP for the NAL under construction
	ch_bytes_t out;        // the Annex-B stream
};

//--------------------------------------------------------------------------------------------------
// Decoder. The reconstruction half -- prediction, the inverse transform, motion compensation,
// deblocking -- is the same code the encoder reconstructs with, which is the point: the encoder
// already had to be a decoder internally to predict correctly, so what is left here is reading
// the bitstream rather than a second implementation of the picture.

typedef struct ch_rbits_t
{
	const uint8_t* data;
	int len;         // bytes of RBSP, emulation prevention already removed
	int pos;         // bit position
	int error;       // sticky: set on a read past the end
} ch_rbits_t;

static int ch_get_bit(ch_rbits_t* r)
{
	if (r->pos >= r->len * 8) { r->error = 1; return 0; }
	int b = (r->data[r->pos >> 3] >> (7 - (r->pos & 7))) & 1;
	++r->pos;
	return b;
}

static uint32_t ch_get_bits(ch_rbits_t* r, int n)
{
	uint32_t v = 0;
	for (int i = 0; i < n; ++i) v = (v << 1) | (uint32_t)ch_get_bit(r);
	return v;
}

// Exp-golomb: a run of zeros says how many more bits to read, so small values are short and the
// code needs no maximum. Reading it wrong does not fail here, it fails several syntax elements
// later, which is why the reader carries a sticky error rather than returning one.
static uint32_t ch_get_ue(ch_rbits_t* r)
{
	int zeros = 0;
	while (!ch_get_bit(r)) {
		if (r->error || ++zeros > 31) { r->error = 1; return 0; }
	}
	if (!zeros) return 0;
	return ((1u << zeros) | ch_get_bits(r, zeros)) - 1;
}

static int32_t ch_get_se(ch_rbits_t* r)
{
	uint32_t k = ch_get_ue(r);
	return (k & 1) ? (int32_t)((k + 1) >> 1) : -(int32_t)(k >> 1);
}

// Whether any syntax elements remain, as opposed to the stop bit and its padding. A slice ends
// when this goes false, not at a macroblock count, which is how a trailing run of skipped
// macroblocks is able to terminate a picture.
static int ch_more_rbsp(ch_rbits_t* r)
{
	int last = r->len * 8;
	while (last > 0) {
		--last;
		if (r->data[last >> 3] & (0x80 >> (last & 7))) break;
	}
	return !r->error && r->pos < last;
}

//--------------------------------------------------------------------------------------------------
// The arithmetic coder's probability model, spec Tables 9-44 and 9-45. Sixty-four probability
// states; each coded bin moves to another state depending on whether the more or the less
// probable symbol turned up.
static const uint8_t ch_range_lps[64][4] = {
	{ 128,176,208,240 }, { 128,167,197,227 }, { 128,158,187,216 }, { 123,150,178,205 },
	{ 116,142,169,195 }, { 111,135,160,185 }, { 105,128,152,175 }, { 100,122,144,166 },
	{  95,116,137,158 }, {  90,110,130,150 }, {  85,104,123,142 }, {  81, 99,117,135 },
	{  77, 94,111,128 }, {  73, 89,105,122 }, {  69, 85,100,116 }, {  66, 80, 95,110 },
	{  62, 76, 90,104 }, {  59, 72, 86, 99 }, {  56, 69, 81, 94 }, {  53, 65, 77, 89 },
	{  51, 62, 73, 85 }, {  48, 59, 69, 80 }, {  46, 56, 66, 76 }, {  43, 53, 63, 72 },
	{  41, 50, 59, 69 }, {  39, 48, 56, 65 }, {  37, 45, 54, 62 }, {  35, 43, 51, 59 },
	{  33, 41, 48, 56 }, {  32, 39, 46, 53 }, {  30, 37, 43, 50 }, {  29, 35, 41, 48 },
	{  27, 33, 39, 45 }, {  26, 31, 37, 43 }, {  24, 30, 35, 41 }, {  23, 28, 33, 39 },
	{  22, 27, 32, 37 }, {  21, 26, 30, 35 }, {  20, 24, 29, 33 }, {  19, 23, 27, 31 },
	{  18, 22, 26, 30 }, {  17, 21, 25, 28 }, {  16, 20, 23, 27 }, {  15, 19, 22, 25 },
	{  14, 18, 21, 24 }, {  14, 17, 20, 23 }, {  13, 16, 19, 22 }, {  12, 15, 18, 21 },
	{  12, 14, 17, 20 }, {  11, 14, 16, 19 }, {  11, 13, 15, 18 }, {  10, 12, 15, 17 },
	{  10, 12, 14, 16 }, {   9, 11, 13, 15 }, {   9, 11, 12, 14 }, {   8, 10, 12, 14 },
	{   8,  9, 11, 13 }, {   7,  9, 11, 12 }, {   7,  9, 10, 12 }, {   7,  8, 10, 11 },
	{   6,  8,  9, 11 }, {   6,  7,  9, 10 }, {   6,  7,  8,  9 }, {   2,  2,  2,  2 },
};
static const uint8_t ch_trans_lps[64] = {
	 0, 0, 1, 2, 2, 4, 4, 5, 6, 7, 8, 9, 9,11,11,12,
	13,13,15,15,16,16,18,18,19,19,21,21,22,22,23,24,
	24,25,26,26,27,27,28,29,29,30,30,30,31,32,32,33,
	33,33,34,34,35,35,35,36,36,36,37,37,37,38,38,63,
};
static const uint8_t ch_trans_mps[64] = {
	 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,
	17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
	33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
	49,50,51,52,53,54,55,56,57,58,59,60,61,62,62,63,
};

// Context initialisation, spec Tables 9-12 to 9-21. Each context starts from a line in the
// quantizer: (m, n) give a slope and an intercept, so a context begins already biased the way
// coefficients at that quality usually are, instead of at even odds.
// Row 0 is I slices; rows 1 to 3 are the three cabac_init_idc choices a P slice can make.
#define CH_CTX_COUNT 277
static const int8_t ch_ctx_init[4][CH_CTX_COUNT][2] = {
	{
		{  20, -15}, {   2,  54}, {   3,  74}, {  20, -15}, {   2,  54}, {   3,  74}, { -28, 127}, { -23, 104},
		{  -6,  53}, {  -1,  54}, {   7,  51}, {  23,  33}, {  23,   2}, {  21,   0}, {   1,   9}, {   0,  49},
		{ -37, 118}, {   5,  57}, { -13,  78}, { -11,  65}, {   1,  62}, {  12,  49}, {  -4,  73}, {  17,  50},
		{   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
		{   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
		{  -3,  69}, {  -6,  81}, { -11,  96}, {   6,  55}, {   7,  67}, {  -5,  86}, {   2,  88}, {   0,  58},
		{  -3,  76}, { -10,  94}, {   5,  54}, {   4,  69}, {  -3,  81}, {   0,  88}, {  -7,  67}, {  -5,  74},
		{  -4,  74}, {  -5,  80}, {  -7,  72}, {   1,  58}, {   0,  41}, {   0,  63}, {   0,  63}, {   0,  63},
		{  -9,  83}, {   4,  86}, {   0,  97}, {  -7,  72}, {  13,  41}, {   3,  62}, {   0,  11}, {   1,  55},
		{   0,  69}, { -17, 127}, { -13, 102}, {   0,  82}, {  -7,  74}, { -21, 107}, { -27, 127}, { -31, 127},
		{ -24, 127}, { -18,  95}, { -27, 127}, { -21, 114}, { -30, 127}, { -17, 123}, { -12, 115}, { -16, 122},
		{ -11, 115}, { -12,  63}, {  -2,  68}, { -15,  84}, { -13, 104}, {  -3,  70}, {  -8,  93}, { -10,  90},
		{ -30, 127}, {  -1,  74}, {  -6,  97}, {  -7,  91}, { -20, 127}, {  -4,  56}, {  -5,  82}, {  -7,  76},
		{ -22, 125}, {  -7,  93}, { -11,  87}, {  -3,  77}, {  -5,  71}, {  -4,  63}, {  -4,  68}, { -12,  84},
		{  -7,  62}, {  -7,  65}, {   8,  61}, {   5,  56}, {  -2,  66}, {   1,  64}, {   0,  61}, {  -2,  78},
		{   1,  50}, {   7,  52}, {  10,  35}, {   0,  44}, {  11,  38}, {   1,  45}, {   0,  46}, {   5,  44},
		{  31,  17}, {   1,  51}, {   7,  50}, {  28,  19}, {  16,  33}, {  14,  62}, { -13, 108}, { -15, 100},
		{ -13, 101}, { -13,  91}, { -12,  94}, { -10,  88}, { -16,  84}, { -10,  86}, {  -7,  83}, { -13,  87},
		{ -19,  94}, {   1,  70}, {   0,  72}, {  -5,  74}, {  18,  59}, {  -8, 102}, { -15, 100}, {   0,  95},
		{  -4,  75}, {   2,  72}, { -11,  75}, {  -3,  71}, {  15,  46}, { -13,  69}, {   0,  62}, {   0,  65},
		{  21,  37}, { -15,  72}, {   9,  57}, {  16,  54}, {   0,  62}, {  12,  72}, {  24,   0}, {  15,   9},
		{   8,  25}, {  13,  18}, {  15,   9}, {  13,  19}, {  10,  37}, {  12,  18}, {   6,  29}, {  20,  33},
		{  15,  30}, {   4,  45}, {   1,  58}, {   0,  62}, {   7,  61}, {  12,  38}, {  11,  45}, {  15,  39},
		{  11,  42}, {  13,  44}, {  16,  45}, {  12,  41}, {  10,  49}, {  30,  34}, {  18,  42}, {  10,  55},
		{  17,  51}, {  17,  46}, {   0,  89}, {  26, -19}, {  22, -17}, {  26, -17}, {  30, -25}, {  28, -20},
		{  33, -23}, {  37, -27}, {  33, -23}, {  40, -28}, {  38, -17}, {  33, -11}, {  40, -15}, {  41,  -6},
		{  38,   1}, {  41,  17}, {  30,  -6}, {  27,   3}, {  26,  22}, {  37, -16}, {  35,  -4}, {  38,  -8},
		{  38,  -3}, {  37,   3}, {  38,   5}, {  42,   0}, {  35,  16}, {  39,  22}, {  14,  48}, {  27,  37},
		{  21,  60}, {  12,  68}, {   2,  97}, {  -3,  71}, {  -6,  42}, {  -5,  50}, {  -3,  54}, {  -2,  62},
		{   0,  58}, {   1,  63}, {  -2,  72}, {  -1,  74}, {  -9,  91}, {  -5,  67}, {  -5,  27}, {  -3,  39},
		{  -2,  44}, {   0,  46}, { -16,  64}, {  -8,  68}, { -10,  78}, {  -6,  77}, { -10,  86}, { -12,  92},
		{ -15,  55}, { -10,  60}, {  -6,  62}, {  -4,  65}, { -12,  73}, {  -8,  76}, {  -7,  80}, {  -9,  88},
		{ -17, 110}, { -11,  97}, { -20,  84}, { -11,  79}, {  -6,  73}, {  -4,  74}, { -13,  86}, { -13,  96},
		{ -11,  97}, { -19, 117}, {  -8,  78}, {  -5,  33}, {  -4,  48}, {  -2,  53}, {  -3,  62}, { -13,  71},
		{ -10,  79}, { -12,  86}, { -13,  90}, { -14,  97}, {   0,   0},
	},
	{
		{  20, -15}, {   2,  54}, {   3,  74}, {  20, -15}, {   2,  54}, {   3,  74}, { -28, 127}, { -23, 104},
		{  -6,  53}, {  -1,  54}, {   7,  51}, {  22,  25}, {  34,   0}, {  16,   0}, {  -2,   9}, {   4,  41},
		{ -29, 118}, {   2,  65}, {  -6,  71}, { -13,  79}, {   5,  52}, {   9,  50}, {  -3,  70}, {  10,  54},
		{   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
		{   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
		{  -2,  69}, {  -5,  82}, { -10,  96}, {   2,  59}, {   2,  75}, {  -3,  87}, {  -3, 100}, {   1,  56},
		{  -3,  74}, {  -6,  85}, {   0,  59}, {  -3,  81}, {  -7,  86}, {  -5,  95}, {  -1,  66}, {  -1,  77},
		{   1,  70}, {  -2,  86}, {  -5,  72}, {   0,  61}, {   0,  41}, {   0,  63}, {   0,  63}, {   0,  63},
		{  -9,  83}, {   4,  86}, {   0,  97}, {  -7,  72}, {  13,  41}, {   3,  62}, {   0,  45}, {  -4,  78},
		{  -3,  96}, { -27, 126}, { -28,  98}, { -25, 101}, { -23,  67}, { -28,  82}, { -20,  94}, { -16,  83},
		{ -22, 110}, { -21,  91}, { -18, 102}, { -13,  93}, { -29, 127}, {  -7,  92}, {  -5,  89}, {  -7,  96},
		{ -13, 108}, {  -3,  46}, {  -1,  65}, {  -1,  57}, {  -9,  93}, {  -3,  74}, {  -9,  92}, {  -8,  87},
		{ -23, 126}, {   5,  54}, {   6,  60}, {   6,  59}, {   6,  69}, {  -1,  48}, {   0,  68}, {  -4,  69},
		{  -8,  88}, {  -2,  85}, {  -6,  78}, {  -1,  75}, {  -7,  77}, {   2,  54}, {   5,  50}, {  -3,  68},
		{   1,  50}, {   6,  42}, {  -4,  81}, {   1,  63}, {  -4,  70}, {   0,  67}, {   2,  57}, {  -2,  76},
		{  11,  35}, {   4,  64}, {   1,  61}, {  11,  35}, {  18,  25}, {  12,  24}, {  13,  29}, {  13,  36},
		{ -10,  93}, {  -7,  73}, {  -2,  73}, {  13,  46}, {   9,  49}, {  -7, 100}, {   9,  53}, {   2,  53},
		{   5,  53}, {  -2,  61}, {   0,  56}, {   0,  56}, { -13,  63}, {  -5,  60}, {  -1,  62}, {   4,  57},
		{  -6,  69}, {   4,  57}, {  14,  39}, {   4,  51}, {  13,  68}, {   3,  64}, {   1,  61}, {   9,  63},
		{   7,  50}, {  16,  39}, {   5,  44}, {   4,  52}, {  11,  48}, {  -5,  60}, {  -1,  59}, {   0,  59},
		{  22,  33}, {   5,  44}, {  14,  43}, {  -1,  78}, {   0,  60}, {   9,  69}, {  11,  28}, {   2,  40},
		{   3,  44}, {   0,  49}, {   0,  46}, {   2,  44}, {   2,  51}, {   0,  47}, {   4,  39}, {   2,  62},
		{   6,  46}, {   0,  54}, {   3,  54}, {   2,  58}, {   4,  63}, {   6,  51}, {   6,  57}, {   7,  53},
		{   6,  52}, {   6,  55}, {  11,  45}, {  14,  36}, {   8,  53}, {  -1,  82}, {   7,  55}, {  -3,  78},
		{  15,  46}, {  22,  31}, {  -1,  84}, {  25,   7}, {  30,  -7}, {  28,   3}, {  28,   4}, {  32,   0},
		{  34,  -1}, {  30,   6}, {  30,   6}, {  32,   9}, {  31,  19}, {  26,  27}, {  26,  30}, {  37,  20},
		{  28,  34}, {  17,  70}, {   1,  67}, {   5,  59}, {   9,  67}, {  16,  30}, {  18,  32}, {  18,  35},
		{  22,  29}, {  24,  31}, {  23,  38}, {  18,  43}, {  20,  41}, {  11,  63}, {   9,  59}, {   9,  64},
		{  -1,  94}, {  -2,  89}, {  -9, 108}, {  -6,  76}, {  -2,  44}, {   0,  45}, {   0,  52}, {  -3,  64},
		{  -2,  59}, {  -4,  70}, {  -4,  75}, {  -8,  82}, { -17, 102}, {  -9,  77}, {   3,  24}, {   0,  42},
		{   0,  48}, {   0,  55}, {  -6,  59}, {  -7,  71}, { -12,  83}, { -11,  87}, { -30, 119}, {   1,  58},
		{  -3,  29}, {  -1,  36}, {   1,  38}, {   2,  43}, {  -6,  55}, {   0,  58}, {   0,  64}, {  -3,  74},
		{ -10,  90}, {   0,  70}, {  -4,  29}, {   5,  31}, {   7,  42}, {   1,  59}, {  -2,  58}, {  -3,  72},
		{  -3,  81}, { -11,  97}, {   0,  58}, {   8,   5}, {  10,  14}, {  14,  18}, {  13,  27}, {   2,  40},
		{   0,  58}, {  -3,  70}, {  -6,  79}, {  -8,  85}, {   0,   0},
	},
	{
		{  20, -15}, {   2,  54}, {   3,  74}, {  20, -15}, {   2,  54}, {   3,  74}, { -28, 127}, { -23, 104},
		{  -6,  53}, {  -1,  54}, {   7,  51}, {  29,  16}, {  25,   0}, {  14,   0}, { -10,  51}, {  -3,  62},
		{ -27,  99}, {  26,  16}, {  -4,  85}, { -24, 102}, {   5,  57}, {   6,  57}, { -17,  73}, {  14,  57},
		{   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
		{   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
		{ -11,  89}, { -15, 103}, { -21, 116}, {  19,  57}, {  20,  58}, {   4,  84}, {   6,  96}, {   1,  63},
		{  -5,  85}, { -13, 106}, {   5,  63}, {   6,  75}, {  -3,  90}, {  -1, 101}, {   3,  55}, {  -4,  79},
		{  -2,  75}, { -12,  97}, {  -7,  50}, {   1,  60}, {   0,  41}, {   0,  63}, {   0,  63}, {   0,  63},
		{  -9,  83}, {   4,  86}, {   0,  97}, {  -7,  72}, {  13,  41}, {   3,  62}, {  13,  15}, {   7,  51},
		{   2,  80}, { -39, 127}, { -18,  91}, { -17,  96}, { -26,  81}, { -35,  98}, { -24, 102}, { -23,  97},
		{ -27, 119}, { -24,  99}, { -21, 110}, { -18, 102}, { -36, 127}, {   0,  80}, {  -5,  89}, {  -7,  94},
		{  -4,  92}, {   0,  39}, {   0,  65}, { -15,  84}, { -35, 127}, {  -2,  73}, { -12, 104}, {  -9,  91},
		{ -31, 127}, {   3,  55}, {   7,  56}, {   7,  55}, {   8,  61}, {  -3,  53}, {   0,  68}, {  -7,  74},
		{  -9,  88}, { -13, 103}, { -13,  91}, {  -9,  89}, { -14,  92}, {  -8,  76}, { -12,  87}, { -23, 110},
		{ -24, 105}, { -10,  78}, { -20, 112}, { -17,  99}, { -78, 127}, { -70, 127}, { -50, 127}, { -46, 127},
		{  -4,  66}, {  -5,  78}, {  -4,  71}, {  -8,  72}, {   2,  59}, {  -1,  55}, {  -7,  70}, {  -6,  75},
		{  -8,  89}, { -34, 119}, {  -3,  75}, {  32,  20}, {  30,  22}, { -44, 127}, {   0,  54}, {  -5,  61},
		{   0,  58}, {  -1,  60}, {  -3,  61}, {  -8,  67}, { -25,  84}, { -14,  74}, {  -5,  65}, {   5,  52},
		{   2,  57}, {   0,  61}, {  -9,  69}, { -11,  70}, {  18,  55}, {  -4,  71}, {   0,  58}, {   7,  61},
		{   9,  41}, {  18,  25}, {   9,  32}, {   5,  43}, {   9,  47}, {   0,  44}, {   0,  51}, {   2,  46},
		{  19,  38}, {  -4,  66}, {  15,  38}, {  12,  42}, {   9,  34}, {   0,  89}, {   4,  45}, {  10,  28},
		{  10,  31}, {  33, -11}, {  52, -43}, {  18,  15}, {  28,   0}, {  35, -22}, {  38, -25}, {  34,   0},
		{  39, -18}, {  32, -12}, { 102, -94}, {   0,   0}, {  56, -15}, {  33,  -4}, {  29,  10}, {  37,  -5},
		{  51, -29}, {  39,  -9}, {  52, -34}, {  69, -58}, {  67, -63}, {  44,  -5}, {  32,   7}, {  55, -29},
		{  32,   1}, {   0,   0}, {  27,  36}, {  33, -25}, {  34, -30}, {  36, -28}, {  38, -28}, {  38, -27},
		{  34, -18}, {  35, -16}, {  34, -14}, {  32,  -8}, {  37,  -6}, {  35,   0}, {  30,  10}, {  28,  18},
		{  26,  25}, {  29,  41}, {   0,  75}, {   2,  72}, {   8,  77}, {  14,  35}, {  18,  31}, {  17,  35},
		{  21,  30}, {  17,  45}, {  20,  42}, {  18,  45}, {  27,  26}, {  16,  54}, {   7,  66}, {  16,  56},
		{  11,  73}, {  10,  67}, { -10, 116}, { -23, 112}, { -15,  71}, {  -7,  61}, {   0,  53}, {  -5,  66},
		{ -11,  77}, {  -9,  80}, {  -9,  84}, { -10,  87}, { -34, 127}, { -21, 101}, {  -3,  39}, {  -5,  53},
		{  -7,  61}, { -11,  75}, { -15,  77}, { -17,  91}, { -25, 107}, { -25, 111}, { -28, 122}, { -11,  76},
		{ -10,  44}, { -10,  52}, { -10,  57}, {  -9,  58}, { -16,  72}, {  -7,  69}, {  -4,  69}, {  -5,  74},
		{  -9,  86}, {   2,  66}, {  -9,  34}, {   1,  32}, {  11,  31}, {   5,  52}, {  -2,  55}, {  -2,  67},
		{   0,  73}, {  -8,  89}, {   3,  52}, {   7,   4}, {  10,   8}, {  17,   8}, {  16,  19}, {   3,  37},
		{  -1,  61}, {  -5,  73}, {  -1,  70}, {  -4,  78}, {   0,   0},
	},
	{
		{  20, -15}, {   2,  54}, {   3,  74}, {  20, -15}, {   2,  54}, {   3,  74}, { -28, 127}, { -23, 104},
		{  -6,  53}, {  -1,  54}, {   7,  51}, {  29,  16}, {  25,   0}, {  14,   0}, { -10,  51}, {  -3,  62},
		{ -27,  99}, {  26,  16}, {  -4,  85}, { -24, 102}, {   5,  57}, {   6,  57}, { -17,  73}, {  14,  57},
		{   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
		{   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
		{ -11,  89}, { -15, 103}, { -21, 116}, {  19,  57}, {  20,  58}, {   4,  84}, {   6,  96}, {   1,  63},
		{  -5,  85}, { -13, 106}, {   5,  63}, {   6,  75}, {  -3,  90}, {  -1, 101}, {   3,  55}, {  -4,  79},
		{  -2,  75}, { -12,  97}, {  -7,  50}, {   1,  60}, {   0,  41}, {   0,  63}, {   0,  63}, {   0,  63},
		{  -9,  83}, {   4,  86}, {   0,  97}, {  -7,  72}, {  13,  41}, {   3,  62}, {   7,  34}, {  -9,  88},
		{ -20, 127}, { -36, 127}, { -17,  91}, { -14,  95}, { -25,  84}, { -25,  86}, { -12,  89}, { -17,  91},
		{ -31, 127}, { -14,  76}, { -18, 103}, { -13,  90}, { -37, 127}, {  11,  80}, {   5,  76}, {   2,  84},
		{   5,  78}, {  -6,  55}, {   4,  61}, { -14,  83}, { -37, 127}, {  -5,  79}, { -11, 104}, { -11,  91},
		{ -30, 127}, {   0,  65}, {  -2,  79}, {   0,  72}, {  -4,  92}, {  -6,  56}, {   3,  68}, {  -8,  71},
		{ -13,  98}, {  -4,  86}, { -12,  88}, {  -5,  82}, {  -3,  72}, {  -4,  67}, {  -8,  72}, { -16,  89},
		{  -9,  69}, {  -1,  59}, {   5,  66}, {   4,  57}, {  -4,  71}, {  -2,  71}, {   2,  58}, {  -1,  74},
		{  -4,  44}, {  -1,  69}, {   0,  62}, {  -7,  51}, {  -4,  47}, {  -6,  42}, {  -3,  41}, {  -6,  53},
		{   8,  76}, {  -9,  78}, { -11,  83}, {   9,  52}, {   0,  67}, {  -5,  90}, {   1,  67}, { -15,  72},
		{  -5,  75}, {  -8,  80}, { -21,  83}, { -21,  64}, { -13,  31}, { -25,  64}, { -29,  94}, {   9,  75},
		{  17,  63}, {  -8,  74}, {  -5,  35}, {  -2,  27}, {  13,  91}, {   3,  65}, {  -7,  69}, {   8,  77},
		{ -10,  66}, {   3,  62}, {  -3,  68}, { -20,  81}, {   0,  30}, {   1,   7}, {  -3,  23}, { -21,  74},
		{  16,  66}, { -23, 124}, {  17,  37}, {  44, -18}, {  50, -34}, { -22, 127}, {   4,  39}, {   0,  42},
		{   7,  34}, {  11,  29}, {   8,  31}, {   6,  37}, {   7,  42}, {   3,  40}, {   8,  33}, {  13,  43},
		{  13,  36}, {   4,  47}, {   3,  55}, {   2,  58}, {   6,  60}, {   8,  44}, {  11,  44}, {  14,  42},
		{   7,  48}, {   4,  56}, {   4,  52}, {  13,  37}, {   9,  49}, {  19,  58}, {  10,  48}, {  12,  45},
		{   0,  69}, {  20,  33}, {   8,  63}, {  35, -18}, {  33, -25}, {  28,  -3}, {  24,  10}, {  27,   0},
		{  34, -14}, {  52, -44}, {  39, -24}, {  19,  17}, {  31,  25}, {  36,  29}, {  24,  33}, {  34,  15},
		{  30,  20}, {  22,  73}, {  20,  34}, {  19,  31}, {  27,  44}, {  19,  16}, {  15,  36}, {  15,  36},
		{  21,  28}, {  25,  21}, {  30,  20}, {  31,  12}, {  27,  16}, {  24,  42}, {   0,  93}, {  14,  56},
		{  15,  57}, {  26,  38}, { -24, 127}, { -24, 115}, { -22,  82}, {  -9,  62}, {   0,  53}, {   0,  59},
		{ -14,  85}, { -13,  89}, { -13,  94}, { -11,  92}, { -29, 127}, { -21, 100}, { -14,  57}, { -12,  67},
		{ -11,  71}, { -10,  77}, { -21,  85}, { -16,  88}, { -23, 104}, { -15,  98}, { -37, 127}, { -10,  82},
		{  -8,  48}, {  -8,  61}, {  -8,  66}, {  -7,  70}, { -14,  75}, { -10,  79}, {  -9,  83}, { -12,  92},
		{ -18, 108}, {  -4,  79}, { -22,  69}, { -16,  75}, {  -2,  58}, {   1,  58}, { -13,  78}, {  -9,  83},
		{  -4,  81}, { -13,  99}, { -13,  81}, {  -6,  38}, { -13,  62}, {  -6,  58}, {  -2,  59}, { -16,  73},
		{ -10,  76}, { -13,  86}, {  -9,  83}, { -10,  87}, {   0,   0},
	},
};

//--------------------------------------------------------------------------------------------------
// CABAC. Where CAVLC picks a codeword out of a table, this narrows an interval: every decision
// splits the remaining range in proportion to how likely the coder currently thinks each outcome
// is, and the final number identifies the whole sequence. A decision the model expects can
// therefore cost well under a bit, which a table-based code can never do.
//
// The model is adaptive, so encoder and decoder must walk identical state. There is no
// resynchronisation: one disagreeing context and everything after it in the slice is noise.

typedef struct ch_cabac_t
{
	ch_bits_t* w;
	uint32_t low;
	uint32_t range;
	int outstanding;      // bits whose value is not decided yet, pending a carry
	int first;            // the very first bit out is a carry artifact and is discarded
	uint8_t ctx[CH_CTX_COUNT];
} ch_cabac_t;

typedef struct ch_cabac_dec_t
{
	ch_rbits_t* r;
	uint32_t offset;
	uint32_t range;
	uint8_t ctx[CH_CTX_COUNT];
} ch_cabac_dec_t;

// Each context starts on a line through the quantizer rather than at even odds, so a slice begins
// with the model already biased the way that quality usually behaves.
static void ch_cabac_ctx_init(uint8_t* ctx, int qp, int table)
{
	int q = ch_clamp(qp, 0, 51);
	for (int i = 0; i < CH_CTX_COUNT; ++i) {
		int m = ch_ctx_init[table][i][0], n = ch_ctx_init[table][i][1];
		int pre = ch_clamp(((m * q) >> 4) + n, 1, 126);
		int state = pre <= 63 ? 63 - pre : pre - 64;
		int mps = pre <= 63 ? 0 : 1;
		ctx[i] = (uint8_t)((state << 1) | mps);
	}
}

static void ch_cabac_put(ch_cabac_t* c, int b)
{
	// The first bit is an artifact of the interval always starting below one half, and the
	// outstanding count is the run of bits that a later carry could still flip.
	if (c->first) c->first = 0;
	else ch_put_bit(c->w, b);
	while (c->outstanding > 0) { ch_put_bit(c->w, !b); --c->outstanding; }
}

static void ch_cabac_renorm(ch_cabac_t* c)
{
	while (c->range < 256) {
		if (c->low < 256) {
			ch_cabac_put(c, 0);
		} else if (c->low >= 512) {
			c->low -= 512;
			ch_cabac_put(c, 1);
		} else {
			// Straddling the midpoint: the bit depends on a carry that has not happened yet.
			c->low -= 256;
			++c->outstanding;
		}
		c->range <<= 1;
		c->low <<= 1;
	}
}

static void ch_cabac_start(ch_cabac_t* c, ch_bits_t* w, int qp, int table)
{
	c->w = w;
	c->low = 0;
	c->range = 510;
	c->outstanding = 0;
	c->first = 1;
	ch_cabac_ctx_init(c->ctx, qp, table);
}

static void ch_cabac_encode(ch_cabac_t* c, int ctx_idx, int bin)
{
	uint8_t* s = &c->ctx[ctx_idx];
	int state = *s >> 1, mps = *s & 1;
	uint32_t lps = ch_range_lps[state][(c->range >> 6) & 3];
	c->range -= lps;
	if (bin != mps) {
		c->low += c->range;
		c->range = lps;
		if (!state) mps ^= 1;         // at the least certain state the model flips which is likely
		state = ch_trans_lps[state];
	} else {
		state = ch_trans_mps[state];
	}
	*s = (uint8_t)((state << 1) | mps);
	ch_cabac_renorm(c);
}

// For values the model has no opinion about -- signs, and the tails of large magnitudes -- where
// half the range each is exactly right and adapting would only add noise.
static void ch_cabac_bypass(ch_cabac_t* c, int bin)
{
	c->low <<= 1;
	if (bin) c->low += c->range;
	if (c->low >= 1024) { ch_cabac_put(c, 1); c->low -= 1024; }
	else if (c->low < 512) { ch_cabac_put(c, 0); }
	else { c->low -= 512; ++c->outstanding; }
}

static void ch_cabac_flush(ch_cabac_t* c)
{
	c->range = 2;
	ch_cabac_renorm(c);
	ch_cabac_put(c, (int)((c->low >> 9) & 1));
	ch_put_bits(c->w, (uint32_t)(((c->low >> 7) & 3) | 1), 2);
}

// The end-of-slice flag, which gets a fixed sliver of the range rather than a context.
static void ch_cabac_terminate(ch_cabac_t* c, int bin)
{
	c->range -= 2;
	if (bin) {
		c->low += c->range;
		ch_cabac_flush(c);
	} else {
		ch_cabac_renorm(c);
	}
}

//--------------------------------------------------------------------------------------------------

static void ch_cabac_dec_start(ch_cabac_dec_t* c, ch_rbits_t* r, int qp, int table)
{
	c->r = r;
	c->range = 510;
	c->offset = ch_get_bits(r, 9);
	ch_cabac_ctx_init(c->ctx, qp, table);
}

static void ch_cabac_dec_renorm(ch_cabac_dec_t* c)
{
	while (c->range < 256) {
		c->range <<= 1;
		c->offset = (c->offset << 1) | (uint32_t)ch_get_bit(c->r);
	}
}

static int ch_cabac_decode(ch_cabac_dec_t* c, int ctx_idx)
{
	uint8_t* s = &c->ctx[ctx_idx];
	int state = *s >> 1, mps = *s & 1, bin;
	uint32_t lps = ch_range_lps[state][(c->range >> 6) & 3];
	c->range -= lps;
	if (c->offset >= c->range) {
		bin = !mps;
		c->offset -= c->range;
		c->range = lps;
		if (!state) mps ^= 1;
		state = ch_trans_lps[state];
	} else {
		bin = mps;
		state = ch_trans_mps[state];
	}
	*s = (uint8_t)((state << 1) | mps);
	ch_cabac_dec_renorm(c);
	return bin;
}

static int ch_cabac_dec_bypass(ch_cabac_dec_t* c)
{
	c->offset = (c->offset << 1) | (uint32_t)ch_get_bit(c->r);
	if (c->offset >= c->range) { c->offset -= c->range; return 1; }
	return 0;
}

static int ch_cabac_dec_terminate(ch_cabac_dec_t* c)
{
	c->range -= 2;
	if (c->offset >= c->range) return 1;
	ch_cabac_dec_renorm(c);
	return 0;
}

//--------------------------------------------------------------------------------------------------

#define CH_MAX_REF_FRAMES 1

struct ch_decoder_t
{
	const uint8_t* stream;
	int stream_len;
	int stream_pos;        // byte offset of the next NAL to look at

	ch_bytes_t rbsp;       // scratch: one NAL with emulation prevention removed
	ch_bytes_t rgba;       // the most recent picture, converted on request

	// Sequence and picture parameters, as far as this decoder cares about them.
	int have_sps, have_pps;
	int w, h;              // cropped, as the caller sees it
	int mb_w, mb_h;
	int log2_max_frame_num;
	int pic_order_cnt_type;
	int log2_max_poc_lsb;
	int pps_qp;            // pic_init_qp
	int deblock_control;   // deblocking_filter_control_present_flag
	int num_ref_idx_l0;

	ch_pic_t pic;
	uint8_t* mem;          // one allocation behind every plane and map in pic
	uint8_t* i4_mode;
	uint8_t* nz_cb;
	uint8_t* nz_cr;

	int qp;
	int slice_is_p;
	int mb_type_offset;
	int has_picture;       // a picture has been decoded and is available
};

const char* ch_decoder_error;

// Strips the start code and the emulation prevention bytes from one NAL unit. The 00 00 03 escape
// exists so a payload can never contain a start code; it has to come back out before the payload
// can be parsed, and it cannot be done while parsing because it straddles syntax elements.
static int ch_next_nal(ch_decoder_t* d, int* nal_type)
{
	const uint8_t* s = d->stream;
	int n = d->stream_len, i = d->stream_pos;
	// Find a start code: 00 00 01, optionally preceded by another 00.
	while (i + 2 < n && !(s[i] == 0 && s[i + 1] == 0 && s[i + 2] == 1)) ++i;
	if (i + 2 >= n) { d->stream_pos = n; return 0; }
	i += 3;
	int start = i;
	// Run to the next start code, which is where this NAL ends.
	while (i + 2 < n && !(s[i] == 0 && s[i + 1] == 0 && (s[i + 2] == 1 || s[i + 2] == 0))) ++i;
	while (i + 2 < n && !(s[i] == 0 && s[i + 1] == 0 && s[i + 2] == 1)) ++i;
	int end = (i + 2 >= n) ? n : i;
	// Trailing zero bytes belong to the next start code, not to this payload.
	while (end > start && s[end - 1] == 0) --end;
	d->stream_pos = end;
	if (end <= start) return 0;

	*nal_type = s[start] & 0x1f;
	d->rbsp.len = 0;
	int zeros = 0;
	for (int k = start + 1; k < end; ++k) {
		if (zeros == 2 && s[k] == 3) { zeros = 0; continue; }
		zeros = s[k] == 0 ? zeros + 1 : 0;
		ch_bytes_push(&d->rbsp, s[k]);
	}
	return 1;
}

static void ch_skip_scaling_lists(ch_rbits_t* r, int count)
{
	for (int i = 0; i < count; ++i) {
		if (!ch_get_bit(r)) continue;
		int size = i < 6 ? 16 : 64, last = 8, next = 8;
		for (int j = 0; j < size; ++j) {
			if (next) { next = (last + ch_get_se(r) + 256) % 256; }
			last = next ? next : last;
		}
	}
}

static int ch_parse_sps(ch_decoder_t* d, ch_rbits_t* r)
{
	int profile = (int)ch_get_bits(r, 8);
	ch_get_bits(r, 8);                     // constraint flags and reserved bits
	ch_get_bits(r, 8);                     // level_idc
	ch_get_ue(r);                          // seq_parameter_set_id
	if (profile == 100 || profile == 110 || profile == 122 || profile == 244 || profile == 44 ||
	    profile == 83 || profile == 86 || profile == 118 || profile == 128 || profile == 138 ||
	    profile == 139 || profile == 134 || profile == 135) {
		int chroma_format = (int)ch_get_ue(r);
		if (chroma_format == 3) ch_get_bit(r);   // separate_colour_plane_flag
		if (chroma_format != 1) { ch_decoder_error = "Only 4:2:0 is supported."; return 0; }
		ch_get_ue(r);                      // bit_depth_luma_minus8
		ch_get_ue(r);                      // bit_depth_chroma_minus8
		ch_get_bit(r);                     // qpprime_y_zero_transform_bypass_flag
		if (ch_get_bit(r)) ch_skip_scaling_lists(r, 8);
	}
	d->log2_max_frame_num = (int)ch_get_ue(r) + 4;
	d->pic_order_cnt_type = (int)ch_get_ue(r);
	if (d->pic_order_cnt_type == 0) {
		d->log2_max_poc_lsb = (int)ch_get_ue(r) + 4;
	} else if (d->pic_order_cnt_type == 1) {
		ch_get_bit(r);                     // delta_pic_order_always_zero_flag
		ch_get_se(r); ch_get_se(r);
		int n = (int)ch_get_ue(r);
		for (int i = 0; i < n; ++i) ch_get_se(r);
	}
	ch_get_ue(r);                          // max_num_ref_frames
	ch_get_bit(r);                         // gaps_in_frame_num_value_allowed_flag
	d->mb_w = (int)ch_get_ue(r) + 1;
	d->mb_h = (int)ch_get_ue(r) + 1;
	if (!ch_get_bit(r)) { ch_decoder_error = "Interlaced streams are not supported."; return 0; }
	ch_get_bit(r);                         // direct_8x8_inference_flag
	int crop_l = 0, crop_r = 0, crop_t = 0, crop_b = 0;
	if (ch_get_bit(r)) {
		crop_l = (int)ch_get_ue(r); crop_r = (int)ch_get_ue(r);
		crop_t = (int)ch_get_ue(r); crop_b = (int)ch_get_ue(r);
	}
	// Crop units are chroma samples in 4:2:0, so two luma samples each.
	d->w = d->mb_w * 16 - (crop_l + crop_r) * 2;
	d->h = d->mb_h * 16 - (crop_t + crop_b) * 2;
	if (r->error) { ch_decoder_error = "Truncated sequence parameter set."; return 0; }
	d->have_sps = 1;
	return 1;
}

static int ch_parse_pps(ch_decoder_t* d, ch_rbits_t* r)
{
	ch_get_ue(r);                          // pic_parameter_set_id
	ch_get_ue(r);                          // seq_parameter_set_id
	if (ch_get_bit(r)) { ch_decoder_error = "CABAC is not supported."; return 0; }
	ch_get_bit(r);                         // bottom_field_pic_order_in_frame_present_flag
	if (ch_get_ue(r) != 0) { ch_decoder_error = "Slice groups are not supported."; return 0; }
	d->num_ref_idx_l0 = (int)ch_get_ue(r) + 1;
	ch_get_ue(r);                          // num_ref_idx_l1_default_active_minus1
	if (ch_get_bit(r)) { ch_decoder_error = "Weighted prediction is not supported."; return 0; }
	ch_get_bits(r, 2);                     // weighted_bipred_idc
	d->pps_qp = 26 + ch_get_se(r);
	ch_get_se(r);                          // pic_init_qs_minus26
	ch_get_se(r);                          // chroma_qp_index_offset
	d->deblock_control = ch_get_bit(r);
	ch_get_bit(r);                         // constrained_intra_pred_flag
	ch_get_bit(r);                         // redundant_pic_cnt_present_flag
	if (r->error) { ch_decoder_error = "Truncated picture parameter set."; return 0; }
	d->have_pps = 1;
	return 1;
}

// Allocates the picture buffers once the sequence parameters are known.
static int ch_decoder_alloc(ch_decoder_t* d)
{
	if (d->mem) return 1;
	size_t luma = (size_t)(d->mb_w * 16) * (d->mb_h * 16);
	size_t chroma = luma / 4;
	size_t nzl = (size_t)(d->mb_w * 4) * (d->mb_h * 4);
	size_t nzc = (size_t)(d->mb_w * 2) * (d->mb_h * 2);
	size_t bytes = (luma + chroma * 2) * 2 + nzl * 2 + nzc * 2;
	d->mem = (uint8_t*)CUTE_H264_ALLOC(bytes);
	if (!d->mem) { ch_decoder_error = "Out of memory."; return 0; }
	CUTE_H264_MEMSET(d->mem, 0, bytes);
	int16_t* mv = (int16_t*)CUTE_H264_ALLOC(nzl * (sizeof(int16_t) * 2 + 1));
	if (!mv) { ch_decoder_error = "Out of memory."; return 0; }
	CUTE_H264_MEMSET(mv, 0, nzl * (sizeof(int16_t) * 2 + 1));
	d->pic.mb_w = d->mb_w;
	d->pic.mb_h = d->mb_h;
	d->pic.luma_stride = d->mb_w * 16;
	d->pic.chroma_stride = d->mb_w * 8;
	d->pic.rec_y = d->mem;
	d->pic.rec_cb = d->pic.rec_y + luma;
	d->pic.rec_cr = d->pic.rec_cb + chroma;
	d->pic.ref_y = d->pic.rec_cr + chroma;
	d->pic.ref_cb = d->pic.ref_y + luma;
	d->pic.ref_cr = d->pic.ref_cb + chroma;
	d->pic.nz_luma = d->pic.ref_cr + chroma;
	d->i4_mode = d->pic.nz_luma + nzl;
	d->nz_cb = d->i4_mode + nzl;
	d->nz_cr = d->nz_cb + nzc;
	d->pic.mv = mv;
	d->pic.ref_idx = (int8_t*)(mv + nzl * 2);
	return 1;
}

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

// Chroma 8x8 intra prediction, spec clause 8.3.4. Mode 0 is DC, 1 horizontal, 2 vertical and
// 3 plane. This encoder only ever chooses DC, but the decoder has to handle all four or it can
// only read its own output -- which is not a decoder.
static void ch_pred8(const uint8_t* rec, int stride, int mode, int have_l, int have_t, uint8_t* out)
{
	if (mode == 1 && have_l) {
		for (int y = 0; y < 8; ++y)
			for (int x = 0; x < 8; ++x) out[y * 8 + x] = rec[(size_t)y * stride - 1];
		return;
	}
	if (mode == 2 && have_t) {
		for (int y = 0; y < 8; ++y)
			for (int x = 0; x < 8; ++x) out[y * 8 + x] = rec[-(ptrdiff_t)stride + x];
		return;
	}
	if (mode == 3 && have_l && have_t) {
		int hh = 0, vv = 0;
		for (int i = 0; i < 4; ++i) {
			hh += (i + 1) * (rec[-(ptrdiff_t)stride + 4 + i] - rec[-(ptrdiff_t)stride + 2 - i]);
			vv += (i + 1) * (rec[(size_t)(4 + i) * stride - 1] - rec[(ptrdiff_t)(2 - i) * (ptrdiff_t)stride - 1]);
		}
		int a = 16 * (rec[(size_t)7 * stride - 1] + rec[-(ptrdiff_t)stride + 7]);
		int b = (34 * hh + 32) >> 6, c = (34 * vv + 32) >> 6;
		for (int y = 0; y < 8; ++y)
			for (int x = 0; x < 8; ++x)
				out[y * 8 + x] = (uint8_t)ch_clip255((a + b * (x - 3) + c * (y - 3) + 16) >> 5);
		return;
	}
	// DC, and the fallback for a mode whose neighbours are not there. Each 4x4 quadrant picks its
	// own source, which is why this is not simply one average over the whole block.
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

//--------------------------------------------------------------------------------------------------
// Intra_4x4. Sixteen small predictions instead of one big one: much better on detail, and it costs
// a prediction mode per block, which is why the two macroblock types are worth comparing per
// macroblock rather than picking one of them for the whole picture.

// The inverse of ch_blk_x/ch_blk_y: which coding-order index sits at a given 4x4 position. Used to
// decide whether a block's upper-right neighbour has been reconstructed yet.
static const int ch_blk_order[4][4] = { { 0,1,4,5 }, { 2,3,6,7 }, { 8,9,12,13 }, { 10,11,14,15 } };

// coded_block_pattern is not sent as a number, it is sent as an index into a table ordered by how
// common each pattern is. This is that table for intra macroblocks, inverted for the encoder:
// indexed by the pattern, giving the code number to write. Spec Table 9-4.
static const uint8_t ch_cbp_intra_codenum[48] = {
	 3,29,30,17,31,18,37, 8,32,38,19, 9,
	20,10,11, 2,16,33,34,21,35,22,39, 4,
	36,40,23, 5,24, 6, 7, 1,41,42,43,25,
	44,26,46,12,45,47,27,13,28,14,15, 0,
};

// 16 * 0.85 * 2^((QP-12)/3), the usual H.264 Lagrangian, in sixteenths so that low QPs -- where it
// is far below one -- do not collapse to zero and turn the mode decision into pure distortion.
static const uint32_t ch_lambda16[52] = {
	1,1,1,2,2,3,3,4,5,7,9,11,14,
	17,22,27,34,43,54,69,86,109,137,173,218,274,
	345,435,548,691,870,1097,1382,1741,2193,2763,3482,4387,5527,
	6963,8773,11053,13926,17546,22107,27853,35092,44214,55706,70185,88427,111411,
};

// Which modes the available neighbours allow. DC is always legal, which is what lets the very
// first block of a picture be coded at all.
static int ch_pred4x4_legal(int mode, int avail)
{
	switch (mode) {
	case 0: case 3: case 7: return (avail & 2) != 0;
	case 1: case 8:         return (avail & 1) != 0;
	case 2:                 return 1;
	default:                return (avail & 7) == 7;
	}
}

// One 4x4 intra prediction, spec clause 8.3.1.2. `avail` is a bitmask over the neighbouring
// samples: 1 = the left column, 2 = the row above, 4 = the corner above-left, 8 = the four samples
// above-right. The two index macros exist so that a spec formula reading p[-1,-1] can be written
// the way the spec writes it, instead of being special-cased at each of its uses.
#define CH_T(i) tt[(i) + 1]
#define CH_L(i) ll[(i) + 1]
static void ch_pred4x4(const uint8_t* rec, int stride, int mode, int avail, uint8_t* out)
{
	int tt[9], ll[5];
	int corner = (avail & 4) ? rec[-stride - 1] : 128;
	tt[0] = corner;
	ll[0] = corner;
	for (int i = 0; i < 4; ++i) tt[i + 1] = (avail & 2) ? rec[-stride + i] : 128;
	// The four samples above-right are only sometimes real. Where they are not, the spec repeats
	// the last real one rather than making the mode unusable.
	for (int i = 4; i < 8; ++i) tt[i + 1] = (avail & 8) ? rec[-stride + i] : tt[4];
	for (int i = 0; i < 4; ++i) ll[i + 1] = (avail & 1) ? rec[i * stride - 1] : 128;

	int dc;
	if ((avail & 3) == 3) dc = (CH_T(0) + CH_T(1) + CH_T(2) + CH_T(3) +
	                            CH_L(0) + CH_L(1) + CH_L(2) + CH_L(3) + 4) >> 3;
	else if (avail & 1)   dc = (CH_L(0) + CH_L(1) + CH_L(2) + CH_L(3) + 2) >> 2;
	else if (avail & 2)   dc = (CH_T(0) + CH_T(1) + CH_T(2) + CH_T(3) + 2) >> 2;
	else                  dc = 128;

	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			int v;
			switch (mode) {
			case 0: v = CH_T(x); break;
			case 1: v = CH_L(y); break;
			case 2: v = dc; break;
			case 3: v = (x == 3 && y == 3) ? (CH_T(6) + 3 * CH_T(7) + 2) >> 2
			                               : (CH_T(x+y) + 2 * CH_T(x+y+1) + CH_T(x+y+2) + 2) >> 2; break;
			case 4: v = x > y ? (CH_T(x-y-2) + 2 * CH_T(x-y-1) + CH_T(x-y) + 2) >> 2
			          : x < y ? (CH_L(y-x-2) + 2 * CH_L(y-x-1) + CH_L(y-x) + 2) >> 2
			          : (CH_T(0) + 2 * corner + CH_L(0) + 2) >> 2; break;
			case 5: { int z = 2 * x - y, i = x - (y >> 1);
			          v = (z >= 0 && !(z & 1)) ? (CH_T(i-1) + CH_T(i) + 1) >> 1
			            : (z >= 0)             ? (CH_T(i-2) + 2 * CH_T(i-1) + CH_T(i) + 2) >> 2
			            : (z == -1)            ? (CH_L(0) + 2 * corner + CH_T(0) + 2) >> 2
			            : (CH_L(y-1) + 2 * CH_L(y-2) + CH_L(y-3) + 2) >> 2; } break;
			case 6: { int z = 2 * y - x, i = y - (x >> 1);
			          v = (z >= 0 && !(z & 1)) ? (CH_L(i-1) + CH_L(i) + 1) >> 1
			            : (z >= 0)             ? (CH_L(i-2) + 2 * CH_L(i-1) + CH_L(i) + 2) >> 2
			            : (z == -1)            ? (CH_L(0) + 2 * corner + CH_T(0) + 2) >> 2
			            : (CH_T(x-1) + 2 * CH_T(x-2) + CH_T(x-3) + 2) >> 2; } break;
			case 7: { int i = x + (y >> 1);
			          v = (y & 1) ? (CH_T(i) + 2 * CH_T(i+1) + CH_T(i+2) + 2) >> 2
			                      : (CH_T(i) + CH_T(i+1) + 1) >> 1; } break;
			default:{ int z = x + 2 * y, i = y + (x >> 1);
			          v = (z < 5 && !(z & 1)) ? (CH_L(i) + CH_L(i+1) + 1) >> 1
			            : (z < 5)             ? (CH_L(i) + 2 * CH_L(i+1) + CH_L(i+2) + 2) >> 2
			            : (z == 5)            ? (CH_L(2) + 3 * CH_L(3) + 2) >> 2
			            : CH_L(3); } break;
			}
			out[y * 4 + x] = (uint8_t)v;
		}
	}
}
#undef CH_T
#undef CH_L

// Everything one macroblock touches, so that a trial encode can be thrown away. The bit writer is
// rewound separately; together they make "encode it both ways and keep the better one" possible
// without a second pass over the picture.
typedef struct ch_mb_state_t
{
	uint8_t y[256], cb[64], cr[64];
	uint8_t nz_l[16], nz_c[2][4];
	uint8_t modes[16];
} ch_mb_state_t;

typedef struct ch_bits_mark_t { int len; uint32_t acc; int nbits; } ch_bits_mark_t;

static ch_bits_mark_t ch_bits_mark(ch_bits_t* w)
{
	ch_bits_mark_t m;
	m.len = w->bytes.len; m.acc = w->acc; m.nbits = w->nbits;
	return m;
}

// Only ever truncates. Bytes past the mark are left in place and simply overwritten by whatever is
// encoded next, so restoring the three counters reproduces the earlier state exactly.
static void ch_bits_rewind(ch_bits_t* w, ch_bits_mark_t m)
{
	w->bytes.len = m.len; w->acc = m.acc; w->nbits = m.nbits;
}

static int ch_bits_count(ch_bits_t* w) { return w->bytes.len * 8 + w->nbits; }

static void ch_mb_copy(ch_encoder_t* e, int mbx, int mby, ch_mb_state_t* s, int save)
{
	int lstride = e->mb_w * 4, cstride = e->mb_w * 2;
	uint8_t* ry = e->rec_y + (size_t)(mby * 16) * e->luma_stride + mbx * 16;
	uint8_t* rb = e->rec_cb + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
	uint8_t* rr = e->rec_cr + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
	uint8_t* nl = e->nz_luma + (size_t)(mby * 4) * lstride + mbx * 4;
	uint8_t* nb = e->nz_cb + (size_t)(mby * 2) * cstride + mbx * 2;
	uint8_t* nr = e->nz_cr + (size_t)(mby * 2) * cstride + mbx * 2;
	uint8_t* mm = e->i4_mode + (size_t)(mby * 4) * lstride + mbx * 4;
	for (int i = 0; i < 16; ++i) {
		uint8_t* row = ry + (size_t)i * e->luma_stride;
		if (save) CUTE_H264_MEMCPY(s->y + i * 16, row, 16);
		else      CUTE_H264_MEMCPY(row, s->y + i * 16, 16);
	}
	for (int i = 0; i < 8; ++i) {
		uint8_t* r0 = rb + (size_t)i * e->chroma_stride;
		uint8_t* r1 = rr + (size_t)i * e->chroma_stride;
		if (save) { CUTE_H264_MEMCPY(s->cb + i * 8, r0, 8); CUTE_H264_MEMCPY(s->cr + i * 8, r1, 8); }
		else      { CUTE_H264_MEMCPY(r0, s->cb + i * 8, 8); CUTE_H264_MEMCPY(r1, s->cr + i * 8, 8); }
	}
	for (int i = 0; i < 4; ++i) {
		if (save) {
			CUTE_H264_MEMCPY(s->nz_l + i * 4, nl + (size_t)i * lstride, 4);
			CUTE_H264_MEMCPY(s->modes + i * 4, mm + (size_t)i * lstride, 4);
		} else {
			CUTE_H264_MEMCPY(nl + (size_t)i * lstride, s->nz_l + i * 4, 4);
			CUTE_H264_MEMCPY(mm + (size_t)i * lstride, s->modes + i * 4, 4);
		}
	}
	for (int i = 0; i < 2; ++i) {
		if (save) {
			CUTE_H264_MEMCPY(s->nz_c[0] + i * 2, nb + (size_t)i * cstride, 2);
			CUTE_H264_MEMCPY(s->nz_c[1] + i * 2, nr + (size_t)i * cstride, 2);
		} else {
			CUTE_H264_MEMCPY(nb + (size_t)i * cstride, s->nz_c[0] + i * 2, 2);
			CUTE_H264_MEMCPY(nr + (size_t)i * cstride, s->nz_c[1] + i * 2, 2);
		}
	}
}

// Squared error of the macroblock as reconstructed, against the source. This is the distortion
// half of the mode decision; the bit writer supplies the other half.
static int64_t ch_mb_ssd(ch_encoder_t* e, int mbx, int mby)
{
	int64_t ssd = 0;
	const uint8_t* sy = e->y + (size_t)(mby * 16) * e->luma_stride + mbx * 16;
	const uint8_t* ry = e->rec_y + (size_t)(mby * 16) * e->luma_stride + mbx * 16;
	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			int d = sy[y * (size_t)e->luma_stride + x] - ry[y * (size_t)e->luma_stride + x];
			ssd += d * d;
		}
	}
	for (int c = 0; c < 2; ++c) {
		const uint8_t* sc = (c ? e->cr : e->cb) + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
		const uint8_t* rc = (c ? e->rec_cr : e->rec_cb) + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
		for (int y = 0; y < 8; ++y) {
			for (int x = 0; x < 8; ++x) {
				int d = sc[y * (size_t)e->chroma_stride + x] - rc[y * (size_t)e->chroma_stride + x];
				ssd += d * d;
			}
		}
	}
	return ssd;
}

// Chroma is identical for both intra macroblock types, so it lives here rather than being written
// twice. This predicts, quantizes and reconstructs both components; the bits go out separately,
// because I_16x16 and I_NxN disagree about what comes before them in the macroblock.
typedef struct ch_chroma_t
{
	int cbp;              // 0 = nothing coded, 1 = DC only, 2 = DC and AC
	int dc[2][4];         // quantized 2x2 DC, in coding order
	int ac[2][4][16];     // quantized AC in zig-zag order; entry 0 is unused
} ch_chroma_t;

static void ch_encode_chroma(ch_encoder_t* e, int mbx, int mby, const uint8_t* cpred, ch_chroma_t* out)
{
	int qpc = ch_chroma_qp(e->qp);
	int cqp_per = qpc / 6, cqp_rem = qpc % 6;
	int cqbits = 15 + cqp_per, cf = (1 << cqbits) / 3;
	out->cbp = 0;
	for (int c = 0; c < 2; ++c) {
		uint8_t* rc = (c ? e->rec_cr : e->rec_cb) + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
		const uint8_t* sc = (c ? e->cr : e->cb) + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
		const uint8_t* cp = cpred + c * 64;
		int16_t cres[64];
		for (int y = 0; y < 8; ++y)
			for (int x = 0; x < 8; ++x)
				cres[y * 8 + x] = (int16_t)(sc[y * (size_t)e->chroma_stride + x] - cp[y * 8 + x]);
		int16_t ccoef[4][16], cdc[4];
		for (int b = 0; b < 4; ++b) {
			int bx = b & 1, by = b >> 1;
			int16_t blk[16];
			for (int y = 0; y < 4; ++y)
				for (int x = 0; x < 4; ++x) blk[y * 4 + x] = cres[(by * 4 + y) * 8 + bx * 4 + x];
			ch_fdct4x4(blk, ccoef[b]);
			cdc[b] = ccoef[b][0];
		}
		int hh[4];
		hh[0] = cdc[0] + cdc[1] + cdc[2] + cdc[3];
		hh[1] = cdc[0] - cdc[1] + cdc[2] - cdc[3];
		hh[2] = cdc[0] + cdc[1] - cdc[2] - cdc[3];
		hh[3] = cdc[0] - cdc[1] - cdc[2] + cdc[3];
		int dc_any = 0;
		for (int i = 0; i < 4; ++i) {
			int mag = hh[i] < 0 ? -hh[i] : hh[i];
			int level = (mag * ch_quant_mf[cqp_rem][0] + 2 * cf) >> (cqbits + 1);
			if (hh[i] < 0) level = -level;
			out->dc[c][i] = level;
			if (level) dc_any = 1;
		}
		int gg[4];
		gg[0] = out->dc[c][0] + out->dc[c][1] + out->dc[c][2] + out->dc[c][3];
		gg[1] = out->dc[c][0] - out->dc[c][1] + out->dc[c][2] - out->dc[c][3];
		gg[2] = out->dc[c][0] + out->dc[c][1] - out->dc[c][2] - out->dc[c][3];
		gg[3] = out->dc[c][0] - out->dc[c][1] - out->dc[c][2] + out->dc[c][3];
		int32_t cdc_deq[4];
		// The <<4 is the weight-scale factor of 16 the spec folds into LevelScale. Luma's AC
		// formula absorbs it into its own shift so it is invisible there; chroma DC does not,
		// and leaving it out makes chroma reconstruct at a sixteenth of its residual -- the
		// picture keeps its average and loses all its colour.
		for (int i = 0; i < 4; ++i)
			cdc_deq[i] = ((int32_t)(gg[i] * ch_dequant_v[cqp_rem][0]) << (cqp_per + 4)) >> 5;
		int ac_any = 0;
		int32_t cdeq[4][16];
		for (int b = 0; b < 4; ++b) {
			CUTE_H264_MEMSET(cdeq[b], 0, sizeof(cdeq[b]));
			if (ch_quant_block(ccoef[b], qpc, 1, out->ac[c][b], cdeq[b])) ac_any = 1;
		}
		if (ac_any) out->cbp = 2;
		else if (dc_any && out->cbp < 1) out->cbp = 1;
		for (int b = 0; b < 4; ++b) {
			int bx = b & 1, by = b >> 1;
			cdeq[b][0] = cdc_deq[b];
			ch_reconstruct_block(cdeq[b], cp + (by * 4) * 8 + bx * 4, 8,
				rc + (size_t)(by * 4) * e->chroma_stride + bx * 4, e->chroma_stride);
		}
	}
}

static void ch_write_chroma(ch_encoder_t* e, int mbx, int mby, int have_l, int have_t, const ch_chroma_t* c)
{
	ch_bits_t* w = &e->bits;
	int cstride = e->mb_w * 2;
	if (c->cbp & 3) {
		for (int i = 0; i < 2; ++i) ch_write_residual(w, c->dc[i], 4, -1);
	}
	for (int i = 0; i < 2; ++i) {
		uint8_t* cmap = (i ? e->nz_cr : e->nz_cb) + (size_t)(mby * 2) * cstride + mbx * 2;
		for (int b = 0; b < 4; ++b) {
			int bx = b & 1, by = b >> 1;
			int n = 0;
			if (c->cbp & 2) {
				int nc = ch_nc(cmap, cstride, bx, by, have_l, have_t);
				n = ch_write_residual(w, c->ac[i][b] + 1, 15, nc);
			}
			cmap[by * cstride + bx] = (uint8_t)n;
		}
	}
}


//--------------------------------------------------------------------------------------------------
// Inter prediction. A P macroblock says "this block looks like that block of the previous frame,
// shifted", which for real video is worth far more than any amount of cleverness within a frame.

// The same idea as ch_cbp_intra_codenum, ordered by what is common in inter macroblocks instead --
// notably pattern 0, "nothing left to code", which is the single most likely outcome there and is
// given the shortest code. Spec Table 9-4.
static const uint8_t ch_cbp_inter_codenum[48] = {
	 0, 2, 3, 7, 4, 8,17,13, 5,18, 9,14,
	10,15,16,11, 1,32,33,36,34,37,44,40,
	35,45,38,41,39,42,43,19, 6,24,25,20,
	26,21,46,28,27,47,22,29,23,30,31,12,
};

// 2^((QP-12)/6), the motion-search Lagrangian. It is the square root of the mode-decision lambda
// because it weighs bits against SAD rather than against squared error.
static const uint16_t ch_lambda_mv[52] = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,2,2,2,2,3,3,3,4,4,4,
	5,6,6,7,8,9,10,11,13,14,16,18,20,
	23,25,29,32,36,40,45,51,57,64,72,81,91,
};

// How many bits ch_se would spend on this value. Used to price a candidate vector during the
// search, so that a distant vector has to actually be better rather than just equal.
static int ch_se_bits(int v)
{
	uint32_t k = (uint32_t)(v <= 0 ? -2 * v : 2 * v - 1) + 1;
	int n = 0;
	while (k > 1) { k >>= 1; ++n; }
	return 2 * n + 1;
}

static int ch_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static int ch_median(int a, int b, int c)
{
	int lo = a < b ? a : b, hi = a < b ? b : a;
	if (c < lo) return lo;
	if (c > hi) return hi;
	return c;
}

// Whether a 4x4 block has been reconstructed yet, which is what decides if its motion vector can
// be used to predict another. Blocks in earlier macroblocks always have been; blocks inside this
// one depend on how it is partitioned, so the caller tracks those in a bitmask.
static int ch_blk_avail(const ch_pic_t* p, int mbx, int mby, int bx, int by, unsigned done)
{
	int mx0 = mbx * 4, my0 = mby * 4;
	if (bx < 0 || by < 0 || bx >= p->mb_w * 4 || by >= p->mb_h * 4) return 0;
	if (by < my0) {
		if (bx < mx0) return mbx > 0;            // above-left macroblock
		if (bx < mx0 + 4) return 1;              // directly above
		return mbx + 1 < p->mb_w;                // above-right
	}
	if (by >= my0 + 4) return 0;                 // below: not reconstructed yet
	if (bx < mx0) return mbx > 0;                // left macroblock
	if (bx >= mx0 + 4) return 0;                 // right macroblock: not reconstructed yet
	return (done >> ((by - my0) * 4 + (bx - mx0))) & 1u;
}

// The motion vector predictor for one partition, spec clause 8.4.1.3. `dir` carries the shape
// shortcut: a tall partition next to a tall neighbour, or a wide one under a wide neighbour,
// predicts better from that neighbour alone than from the median of three.
static void ch_mv_predict_part(const ch_pic_t* p, int mbx, int mby, int bx, int by, int bw,
                               unsigned done, int dir, int* out_x, int* out_y)
{
	int stride = p->mb_w * 4;
	int ax = 0, ay = 0, ar = -1;
	int tx = 0, ty = 0, tr = -1;
	int cx = 0, cy = 0, cr = -1;
	int has_a = ch_blk_avail(p, mbx, mby, bx - 1, by, done);
	int has_b = ch_blk_avail(p, mbx, mby, bx, by - 1, done);
	int has_c = ch_blk_avail(p, mbx, mby, bx + bw, by - 1, done);
	int cbx = bx + bw, cby = by - 1;
	if (!has_c) {
		// Where there is nothing above-right the spec substitutes above-left rather than dropping
		// to two candidates.
		cbx = bx - 1;
		has_c = ch_blk_avail(p, mbx, mby, cbx, cby, done);
	}
	if (has_a) { int i = by * stride + bx - 1; ax = p->mv[i * 2]; ay = p->mv[i * 2 + 1]; ar = p->ref_idx[i]; }
	if (has_b) { int i = (by - 1) * stride + bx; tx = p->mv[i * 2]; ty = p->mv[i * 2 + 1]; tr = p->ref_idx[i]; }
	if (has_c) { int i = cby * stride + cbx; cx = p->mv[i * 2]; cy = p->mv[i * 2 + 1]; cr = p->ref_idx[i]; }

	if (dir == 1 && tr == 0) { *out_x = tx; *out_y = ty; return; }
	if (dir == 2 && ar == 0) { *out_x = ax; *out_y = ay; return; }
	if (dir == 3 && cr == 0) { *out_x = cx; *out_y = cy; return; }

	if (!has_b && !has_c && has_a) {
		tx = ax; ty = ay; tr = ar;
		cx = ax; cy = ay; cr = ar;
	}
	int n = (ar == 0) + (tr == 0) + (cr == 0);
	if (n == 1) {
		if (ar == 0)      { *out_x = ax; *out_y = ay; }
		else if (tr == 0) { *out_x = tx; *out_y = ty; }
		else              { *out_x = cx; *out_y = cy; }
	} else {
		*out_x = ch_median(ax, tx, cx);
		*out_y = ch_median(ay, ty, cy);
	}
}

static void ch_mv_predict(const ch_pic_t* p, int mbx, int mby, int* out_x, int* out_y)
{
	ch_mv_predict_part(p, mbx, mby, mbx * 4, mby * 4, 4, 0, 0, out_x, out_y);
}

// Records a partition's vector over the 4x4 blocks it covers.
static void ch_set_motion_part(ch_pic_t* p, int bx, int by, int bw, int bh, int mvx, int mvy)
{
	int stride = p->mb_w * 4;
	for (int y = 0; y < bh; ++y) {
		for (int x = 0; x < bw; ++x) {
			int i = (by + y) * stride + bx + x;
			p->mv[i * 2] = (int16_t)mvx; p->mv[i * 2 + 1] = (int16_t)mvy; p->ref_idx[i] = 0;
		}
	}
}

static void ch_set_motion(ch_pic_t* p, int mbx, int mby, int mvx, int mvy)
{
	ch_set_motion_part(p, mbx * 4, mby * 4, 4, 4, mvx, mvy);
}

// The six-tap half-sample filter, spec equations 8-241 and 8-242. Applied twice, once per axis,
// to reach the centre position.
static int ch_tap6(int a, int b, int c, int d, int f, int g)
{
	return a - 5 * b + 20 * c + 20 * d - 5 * f + g;
}

// Fetches one reference sample. A vector is allowed to point outside the picture, and the spec's
// answer is to clamp the coordinate, extending the edge pixels outwards -- so even a whole-sample
// vector cannot be a plain memcpy.
static int ch_ref_px(const ch_pic_t* p, int x, int y)
{
	int W = p->mb_w * 16, H = p->mb_h * 16;
	return p->ref_y[(size_t)ch_clamp(y, 0, H - 1) * p->luma_stride + ch_clamp(x, 0, W - 1)];
}

// Luma prediction for one partition at quarter-sample precision, spec clause 8.4.2.2.1. Half
// samples come from the six-tap filter and quarter samples from averaging a half sample with its
// neighbour, which is why the centre position j is filtered twice and rounded only at the end.
//
// The intermediates are computed for the whole block rather than per pixel. Done per pixel the
// centre position alone re-runs the horizontal filter six times for every output sample, and the
// motion search calls this often enough for that to dominate the encode.
static void ch_mc_luma_block(const ch_pic_t* p, int ox, int oy, int bw, int bh,
                             int mvx, int mvy, uint8_t* out, int out_stride)
{
	int fx = mvx & 3, fy = mvy & 3;
	ox += mvx >> 2;
	oy += mvy >> 2;
	if (!fx && !fy) {
		int W = p->mb_w * 16, H = p->mb_h * 16;
		for (int y = 0; y < bh; ++y) {
			const uint8_t* row = p->ref_y + (size_t)ch_clamp(oy + y, 0, H - 1) * p->luma_stride;
			for (int x = 0; x < bw; ++x) out[y * out_stride + x] = row[ch_clamp(ox + x, 0, W - 1)];
		}
		return;
	}

	// b1: the horizontal filter, over the rows the vertical pass will need.
	// h1: the vertical filter, over one extra column because position m sits one column right.
	int b1[21][16], h1[16][17], j1[16][16];
	if (fx) {
		for (int r = 0; r < bh + 5; ++r) {
			int Y = oy + r - 2;
			for (int x = 0; x < bw; ++x) {
				int X = ox + x;
				b1[r][x] = ch_tap6(ch_ref_px(p, X - 2, Y), ch_ref_px(p, X - 1, Y), ch_ref_px(p, X, Y),
				                   ch_ref_px(p, X + 1, Y), ch_ref_px(p, X + 2, Y), ch_ref_px(p, X + 3, Y));
			}
		}
	}
	if (fy) {
		for (int y = 0; y < bh; ++y) {
			int Y = oy + y;
			for (int x = 0; x < bw + 1; ++x) {
				int X = ox + x;
				h1[y][x] = ch_tap6(ch_ref_px(p, X, Y - 2), ch_ref_px(p, X, Y - 1), ch_ref_px(p, X, Y),
				                   ch_ref_px(p, X, Y + 1), ch_ref_px(p, X, Y + 2), ch_ref_px(p, X, Y + 3));
			}
		}
	}
	if (fx && fy) {
		// The centre is the vertical filter applied to the horizontal intermediates, kept at full
		// precision until the single rounding at the end.
		for (int y = 0; y < bh; ++y)
			for (int x = 0; x < bw; ++x)
				j1[y][x] = ch_tap6(b1[y][x], b1[y + 1][x], b1[y + 2][x],
				                   b1[y + 3][x], b1[y + 4][x], b1[y + 5][x]);
	}

	for (int y = 0; y < bh; ++y) {
		for (int x = 0; x < bw; ++x) {
			int X = ox + x, Y = oy + y;
			int b = fx ? ch_clip255((b1[y + 2][x] + 16) >> 5) : 0;
			int s = fx ? ch_clip255((b1[y + 3][x] + 16) >> 5) : 0;
			int h = fy ? ch_clip255((h1[y][x] + 16) >> 5) : 0;
			int m = fy ? ch_clip255((h1[y][x + 1] + 16) >> 5) : 0;
			int j = (fx && fy) ? ch_clip255((j1[y][x] + 512) >> 10) : 0;
			int v;
			// Table 8-12, laid out as the spec prints it: G d h n / a e i p / b f j q / c g k r.
			switch (fx * 4 + fy) {
			case 0:  v = ch_ref_px(p, X, Y); break;
			case 1:  v = (ch_ref_px(p, X, Y) + h + 1) >> 1; break;          // d
			case 2:  v = h; break;
			case 3:  v = (ch_ref_px(p, X, Y + 1) + h + 1) >> 1; break;      // n
			case 4:  v = (ch_ref_px(p, X, Y) + b + 1) >> 1; break;          // a
			case 5:  v = (b + h + 1) >> 1; break;                           // e
			case 6:  v = (h + j + 1) >> 1; break;                           // i
			case 7:  v = (h + s + 1) >> 1; break;                           // p
			case 8:  v = b; break;
			case 9:  v = (b + j + 1) >> 1; break;                           // f
			case 10: v = j; break;
			case 11: v = (j + s + 1) >> 1; break;                           // q
			case 12: v = (ch_ref_px(p, X + 1, Y) + b + 1) >> 1; break;      // c
			case 13: v = (b + m + 1) >> 1; break;                           // g
			case 14: v = (j + m + 1) >> 1; break;                           // k
			default: v = (m + s + 1) >> 1; break;                           // r
			}
			out[y * out_stride + x] = (uint8_t)v;
		}
	}
}

// Chroma prediction, spec clause 8.4.2.2.2. In 4:2:0 the chroma vector is the luma vector read at
// eighth-sample precision, so even a whole-sample luma move lands on a half sample in chroma and
// always needs the bilinear filter. Sizes and offsets here are in chroma samples.
static void ch_mc_chroma_block(const ch_pic_t* p, int ox, int oy, int bw, int bh,
                               int mvx, int mvy, uint8_t* out, int out_stride)
{
	int W = p->mb_w * 8, H = p->mb_h * 8;
	int fx = mvx & 7, fy = mvy & 7;
	ox += mvx >> 3;
	oy += mvy >> 3;
	for (int c = 0; c < 2; ++c) {
		const uint8_t* ref = c ? p->ref_cr : p->ref_cb;
		uint8_t* dst = out + c * 64;
		for (int y = 0; y < bh; ++y) {
			const uint8_t* r0 = ref + (size_t)ch_clamp(oy + y, 0, H - 1) * p->chroma_stride;
			const uint8_t* r1 = ref + (size_t)ch_clamp(oy + y + 1, 0, H - 1) * p->chroma_stride;
			for (int x = 0; x < bw; ++x) {
				int x0 = ch_clamp(ox + x, 0, W - 1), x1 = ch_clamp(ox + x + 1, 0, W - 1);
				int v = (8 - fx) * (8 - fy) * r0[x0] + fx * (8 - fy) * r0[x1]
				      + (8 - fx) * fy * r1[x0] + fx * fy * r1[x1];
				dst[y * out_stride + x] = (uint8_t)((v + 32) >> 6);
			}
		}
	}
}

static void ch_mc_luma(const ch_pic_t* p, int mbx, int mby, int mvx, int mvy, uint8_t* out)
{
	ch_mc_luma_block(p, mbx * 16, mby * 16, 16, 16, mvx, mvy, out, 16);
}

static void ch_mc_chroma(const ch_pic_t* p, int mbx, int mby, int mvx, int mvy, uint8_t* out)
{
	ch_mc_chroma_block(p, mbx * 8, mby * 8, 8, 8, mvx, mvy, out, 8);
}

// Records that this macroblock has no motion, so a later neighbour asking for its vector is told
// "intra" rather than reading whatever the previous picture left there.
static void ch_mark_intra_motion(ch_pic_t* p, int mbx, int mby)
{
	int stride = p->mb_w * 4;
	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			int i = (mby * 4 + y) * stride + mbx * 4 + x;
			p->mv[i * 2] = 0; p->mv[i * 2 + 1] = 0; p->ref_idx[i] = -1;
		}
	}
}

// The four ways a P macroblock can be split, in 4x4 units: one 16x16, two 16x8, two 8x16, or four
// 8x8. Smaller pieces track motion that does not move as one block, at the cost of a vector each.
static const int8_t ch_part_shape[4][4][4] = {
	{ { 0,0,4,4 } },
	{ { 0,0,4,2 }, { 0,2,4,2 } },
	{ { 0,0,2,4 }, { 2,0,2,4 } },
	{ { 0,0,2,2 }, { 2,0,2,2 }, { 0,2,2,2 }, { 2,2,2,2 } },
};
static const int ch_part_count[4] = { 1, 2, 2, 4 };
// Which neighbour each partition prefers over the median, per spec equations 8-203 to 8-206.
static const int ch_part_dir[4][4] = { { 0 }, { 1, 2 }, { 2, 3 }, { 0, 0, 0, 0 } };
// Roughly what the shape costs to signal: mb_type, plus four sub_mb_types for the 8x8 case.
static const int ch_part_bits[4] = { 1, 3, 3, 9 };

static unsigned ch_rect_bits(int bx, int by, int bw, int bh)
{
	unsigned m = 0;
	for (int y = 0; y < bh; ++y)
		for (int x = 0; x < bw; ++x) m |= 1u << ((by + y) * 4 + bx + x);
	return m;
}

static int ch_sad(const uint8_t* a, int as, const uint8_t* b, int bs, int w, int h)
{
	int sad = 0;
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			int d = a[(size_t)y * as + x] - b[(size_t)y * bs + x];
			sad += d < 0 ? -d : d;
		}
	}
	return sad;
}

// Finds a vector for one partition. Starts from the predicted vector, which is both the better
// guess and the one that costs nothing to send, then walks downhill with a halving step -- the
// last two rounds landing on half and then quarter samples, where most of the gain is.
static int ch_search_part(ch_encoder_t* e, int mbx, int mby, int bx4, int by4, int bw4, int bh4,
                          unsigned done, int dir, int* best_x, int* best_y)
{
	int px = mbx * 16 + bx4 * 4, py = mby * 16 + by4 * 4;
	int bw = bw4 * 4, bh = bh4 * 4;
	const uint8_t* sy = e->y + (size_t)py * e->luma_stride + px;
	int pmvx, pmvy;
	ch_mv_predict_part(&e->pic, mbx, mby, mbx * 4 + bx4, mby * 4 + by4, bw4, done, dir, &pmvx, &pmvy);
	int lm = ch_lambda_mv[e->qp];
	uint8_t pred[256];
	int bx = pmvx & ~3, by = pmvy & ~3, bc = 0x7fffffff;
	int cand[2][2] = { { pmvx & ~3, pmvy & ~3 }, { 0, 0 } };
	for (int i = 0; i < 2; ++i) {
		ch_mc_luma_block(&e->pic, px, py, bw, bh, cand[i][0], cand[i][1], pred, 16);
		int cost = ch_sad(sy, e->luma_stride, pred, 16, bw, bh)
		         + lm * (ch_se_bits(cand[i][0] - pmvx) + ch_se_bits(cand[i][1] - pmvy));
		if (cost < bc) { bc = cost; bx = cand[i][0]; by = cand[i][1]; }
	}
	// A whole-macroblock search has to find the ballpark; a sub-partition inherits a predictor
	// from the partitions beside it and only needs refining, so it skips the coarse steps.
	for (int step = (bw4 == 4 && bh4 == 4) ? 16 : 4; step >= 1; step >>= 1) {
		int improved = 1;
		while (improved) {
			improved = 0;
			static const int dx[8] = { -1,1,0,0,-1,-1,1,1 }, dy[8] = { 0,0,-1,1,-1,1,-1,1 };
			for (int k = 0; k < 8; ++k) {
				int mx = bx + dx[k] * step, my = by + dy[k] * step;
				if (mx < -2048 || mx > 2047 || my < -512 || my > 511) continue;
				ch_mc_luma_block(&e->pic, px, py, bw, bh, mx, my, pred, 16);
				int cost = ch_sad(sy, e->luma_stride, pred, 16, bw, bh)
				         + lm * (ch_se_bits(mx - pmvx) + ch_se_bits(my - pmvy));
				if (cost < bc) { bc = cost; bx = mx; by = my; improved = 1; }
			}
		}
	}
	*best_x = bx;
	*best_y = by;
	return bc;
}

// Encodes one I_16x16 macroblock: predict from the reconstructed neighbours, transform and
// quantize the residual, write it, and reconstruct so the NEXT macroblock predicts from exactly
// what a decoder will have. Predicting from the source instead is the classic drift bug -- it
// looks fine in the encoder and diverges in the decoder.
static int64_t ch_encode_mb_i16(ch_encoder_t* e, int mbx, int mby)
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

	uint8_t cpred[128];
	for (int c = 0; c < 2; ++c) {
		const uint8_t* rc = (c ? e->rec_cr : e->rec_cb) + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
		ch_pred8(rc, e->chroma_stride, 0, have_l, have_t, cpred + c * 64);
	}
	ch_chroma_t chroma;
	ch_encode_chroma(e, mbx, mby, cpred, &chroma);
	int cbp_chroma = chroma.cbp;

	// mb_type packs the prediction mode and both CBPs into one number for I_16x16.
	ch_ue(w, (uint32_t)(e->mb_type_offset + 1 + pred_mode + 4 * cbp_chroma + 12 * (cbp_luma ? 1 : 0)));
	ch_ue(w, 0);              // intra_chroma_pred_mode: DC
	ch_se(w, 0);              // mb_qp_delta: constant QP for now

	int lstride = e->mb_w * 4;
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
	ch_write_chroma(e, mbx, mby, have_l, have_t, &chroma);
	// A macroblock that is not I_NxN still has to leave a prediction mode behind, because the
	// next macroblock's modes are coded relative to it. The spec's answer is DC.
	uint8_t* mmap = e->i4_mode + (size_t)(mby * 4) * lstride + mbx * 4;
	for (int by = 0; by < 4; ++by)
		for (int bx = 0; bx < 4; ++bx) mmap[by * lstride + bx] = 2;
	ch_mark_intra_motion(&e->pic, mbx, mby);
	return ch_mb_ssd(e, mbx, mby);
}

// Encodes one I_NxN macroblock: sixteen independent 4x4 predictions, each from the reconstruction
// of the blocks already coded, including blocks earlier in this same macroblock. Returns the
// squared error so the caller can compare it against I_16x16.
static int64_t ch_encode_mb_i4(ch_encoder_t* e, int mbx, int mby)
{
	ch_bits_t* w = &e->bits;
	int have_l = mbx > 0, have_t = mby > 0;
	int lstride = e->mb_w * 4;
	uint8_t* lmap = e->nz_luma + (size_t)(mby * 4) * lstride + mbx * 4;
	uint8_t* mmap = e->i4_mode + (size_t)(mby * 4) * lstride + mbx * 4;

	int zz[16][16], mode_of[16], pred_of[16];
	int cbp_luma = 0;

	for (int b = 0; b < 16; ++b) {
		int bx = ch_blk_x[b], by = ch_blk_y[b];
		uint8_t* ry = e->rec_y + (size_t)(mby * 16 + by * 4) * e->luma_stride + mbx * 16 + bx * 4;
		const uint8_t* sy = e->y + (size_t)(mby * 16 + by * 4) * e->luma_stride + mbx * 16 + bx * 4;

		int av_l = bx > 0 || have_l;
		int av_t = by > 0 || have_t;
		// The upper-right neighbour is the awkward one. On the top row of the macroblock it comes
		// from the macroblock above, or above-right at the far edge; below that it has to already
		// be reconstructed, which for the right-hand column it never is.
		int av_tr = by == 0 ? (av_t && (bx < 3 || mbx + 1 < e->mb_w))
		                    : (bx < 3 && ch_blk_order[by - 1][bx + 1] < b);
		int avail = (av_l ? 1 : 0) | (av_t ? 2 : 0) | ((av_l && av_t) ? 4 : 0) | (av_tr ? 8 : 0);

		// predIntra4x4PredMode, clause 8.3.1.1. Note this collapses to DC when either neighbouring
		// MACROBLOCK is missing -- not the same as falling back to whichever neighbour does exist.
		int pm;
		if ((bx == 0 && !have_l) || (by == 0 && !have_t)) pm = 2;
		else {
			int ma = mmap[by * lstride + (bx - 1)];
			int mb = mmap[(by - 1) * lstride + bx];
			pm = ma < mb ? ma : mb;
		}

		uint8_t pred[16], best_pred[16];
		int best = 2, best_cost = 0x7fffffff;
		for (int m = 0; m < 9; ++m) {
			if (!ch_pred4x4_legal(m, avail)) continue;
			ch_pred4x4(ry, e->luma_stride, m, avail, pred);
			int sad = 0;
			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x) {
					int d = sy[y * (size_t)e->luma_stride + x] - pred[y * 4 + x];
					sad += d < 0 ? -d : d;
				}
			}
			// Any mode other than the predicted one costs three extra bits to signal, so it has to
			// be better by more than nothing to be worth taking.
			int cost = sad + (m == pm ? 0 : 4);
			if (cost < best_cost) { best_cost = cost; best = m; CUTE_H264_MEMCPY(best_pred, pred, 16); }
		}

		int16_t blk[16], coef[16];
		int32_t deq[16];
		for (int y = 0; y < 4; ++y)
			for (int x = 0; x < 4; ++x)
				blk[y * 4 + x] = (int16_t)(sy[y * (size_t)e->luma_stride + x] - best_pred[y * 4 + x]);
		ch_fdct4x4(blk, coef);
		CUTE_H264_MEMSET(deq, 0, sizeof(deq));
		// Unlike I_16x16 there is no separate DC block, so nothing is held back from the quantizer.
		if (ch_quant_block(coef, e->qp, 0, zz[b], deq)) cbp_luma |= 1 << (b >> 2);
		ch_reconstruct_block(deq, best_pred, 4, ry, e->luma_stride);
		mode_of[b] = best;
		pred_of[b] = pm;
		mmap[by * lstride + bx] = (uint8_t)best;
	}

	uint8_t cpred[128];
	for (int c = 0; c < 2; ++c) {
		const uint8_t* rc = (c ? e->rec_cr : e->rec_cb) + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
		ch_pred8(rc, e->chroma_stride, 0, have_l, have_t, cpred + c * 64);
	}
	ch_chroma_t chroma;
	ch_encode_chroma(e, mbx, mby, cpred, &chroma);
	int cbp = cbp_luma | (chroma.cbp << 4);

	ch_ue(w, (uint32_t)e->mb_type_offset); // mb_type 0 in an I slice is I_NxN
	for (int b = 0; b < 16; ++b) {
		if (mode_of[b] == pred_of[b]) ch_put_bit(w, 1);
		else {
			ch_put_bit(w, 0);
			// The predicted mode is removed from the alphabet rather than being sent as a ninth
			// value, which is what keeps this to three bits instead of four.
			int rem = mode_of[b] < pred_of[b] ? mode_of[b] : mode_of[b] - 1;
			ch_put_bits(w, (uint32_t)rem, 3);
		}
	}
	ch_ue(w, 0);                      // intra_chroma_pred_mode: DC
	ch_ue(w, ch_cbp_intra_codenum[cbp]);
	// Unlike I_16x16, where a DC block is always present, mb_qp_delta is only sent when something
	// is actually coded.
	if (cbp) ch_se(w, 0);

	for (int b = 0; b < 16; ++b) {
		int bx = ch_blk_x[b], by = ch_blk_y[b];
		int n = 0;
		if (cbp_luma & (1 << (b >> 2))) {
			int nc = ch_nc(lmap, lstride, bx, by, have_l, have_t);
			n = ch_write_residual(w, zz[b], 16, nc);
		}
		lmap[by * lstride + bx] = (uint8_t)n;
	}
	ch_write_chroma(e, mbx, mby, have_l, have_t, &chroma);
	ch_mark_intra_motion(&e->pic, mbx, mby);
	return ch_mb_ssd(e, mbx, mby);
}

// The vector a skipped macroblock is understood to have, spec clause 8.4.1.1. It is usually the
// ordinary prediction, but collapses to zero at the picture edge or next to a neighbour that is
// itself stationary -- which is what makes a still background cost almost nothing.
static void ch_skip_mv(const ch_pic_t* p, int mbx, int mby, int* mx, int* my)
{
	int stride = p->mb_w * 4;
	int bx = mbx * 4, by = mby * 4;
	*mx = 0; *my = 0;
	if (mbx == 0 || mby == 0) return;
	int ia = by * stride + bx - 1;
	int ib = (by - 1) * stride + bx;
	if (p->ref_idx[ia] == 0 && p->mv[ia * 2] == 0 && p->mv[ia * 2 + 1] == 0) return;
	if (p->ref_idx[ib] == 0 && p->mv[ib * 2] == 0 && p->mv[ib * 2 + 1] == 0) return;
	ch_mv_predict(p, mbx, mby, mx, my);
}

// A skipped macroblock carries no residual at all, so its reconstruction IS the prediction. The
// coefficient counts have to be cleared rather than left alone: a later neighbour reading them
// would otherwise pick a CAVLC table on the strength of a macroblock that coded nothing.
static void ch_commit_skip(ch_encoder_t* e, int mbx, int mby, int mvx, int mvy,
                           const uint8_t* pred, const uint8_t* cpred)
{
	int lstride = e->mb_w * 4, cstride = e->mb_w * 2;
	uint8_t* ry = e->rec_y + (size_t)(mby * 16) * e->luma_stride + mbx * 16;
	for (int y = 0; y < 16; ++y) CUTE_H264_MEMCPY(ry + (size_t)y * e->luma_stride, pred + y * 16, 16);
	for (int c = 0; c < 2; ++c) {
		uint8_t* rc = (c ? e->rec_cr : e->rec_cb) + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
		for (int y = 0; y < 8; ++y)
			CUTE_H264_MEMCPY(rc + (size_t)y * e->chroma_stride, cpred + c * 64 + y * 8, 8);
	}
	uint8_t* lmap = e->nz_luma + (size_t)(mby * 4) * lstride + mbx * 4;
	uint8_t* mmap = e->i4_mode + (size_t)(mby * 4) * lstride + mbx * 4;
	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) { lmap[y * lstride + x] = 0; mmap[y * lstride + x] = 2; }
	}
	for (int c = 0; c < 2; ++c) {
		uint8_t* cmap = (c ? e->nz_cr : e->nz_cb) + (size_t)(mby * 2) * cstride + mbx * 2;
		for (int y = 0; y < 2; ++y)
			for (int x = 0; x < 2; ++x) cmap[y * cstride + x] = 0;
	}
	ch_set_motion(&e->pic, mbx, mby, mvx, mvy);
}

// Encodes one P macroblock. Returns the squared error so the caller can weigh it against coding
// the macroblock intra instead, which is what has to happen wherever the picture is new rather
// than moved.
static int64_t ch_encode_mb_p(ch_encoder_t* e, int mbx, int mby)
{
	ch_bits_t* w = &e->bits;
	int have_l = mbx > 0, have_t = mby > 0;
	int lstride = e->mb_w * 4;
	uint8_t* lmap = e->nz_luma + (size_t)(mby * 4) * lstride + mbx * 4;
	const uint8_t* sy = e->y + (size_t)(mby * 16) * e->luma_stride + mbx * 16;
	uint8_t* ry = e->rec_y + (size_t)(mby * 16) * e->luma_stride + mbx * 16;

	// Each shape is searched against a clean motion field, because a partition's predictor reads
	// the partitions decided before it and a leftover from another shape would poison it.
	int16_t saved_mv[32];
	int8_t saved_ref[16];
	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			int i = (mby * 4 + y) * lstride + mbx * 4 + x;
			saved_mv[(y * 4 + x) * 2] = e->pic.mv[i * 2];
			saved_mv[(y * 4 + x) * 2 + 1] = e->pic.mv[i * 2 + 1];
			saved_ref[y * 4 + x] = e->pic.ref_idx[i];
		}
	}

	int mv[4][2], best_mv[4][2];
	int best_shape = 0, best_cost = 0x7fffffff;
	int lm = ch_lambda_mv[e->qp];
	// Splitting costs a search per partition, and a block the single vector already predicts
	// well has nothing to gain from it. The threshold rises with the quantizer because a coarse
	// quantizer throws away the small residual a finer split would have saved.
	int split_worth_it = 256 * (e->qp / 6 + 2);
	for (int shape = 0; shape < 4; ++shape) {
		if (shape == 1 && best_cost < split_worth_it) break;
		for (int y = 0; y < 4; ++y) {
			for (int x = 0; x < 4; ++x) {
				int i = (mby * 4 + y) * lstride + mbx * 4 + x;
				e->pic.mv[i * 2] = saved_mv[(y * 4 + x) * 2];
				e->pic.mv[i * 2 + 1] = saved_mv[(y * 4 + x) * 2 + 1];
				e->pic.ref_idx[i] = saved_ref[y * 4 + x];
			}
		}
		unsigned done = 0;
		int cost = lm * ch_part_bits[shape];
		for (int k = 0; k < ch_part_count[shape]; ++k) {
			// The cost only grows as partitions are added, so a shape that is already behind
			// cannot come back. Abandoning it here changes no decision, only the time to reach it.
			if (cost >= best_cost) { cost = 0x7fffffff; break; }
			const int8_t* s = ch_part_shape[shape][k];
			cost += ch_search_part(e, mbx, mby, s[0], s[1], s[2], s[3], done,
			                       ch_part_dir[shape][k], &mv[k][0], &mv[k][1]);
			ch_set_motion_part(&e->pic, mbx * 4 + s[0], mby * 4 + s[1], s[2], s[3], mv[k][0], mv[k][1]);
			done |= ch_rect_bits(s[0], s[1], s[2], s[3]);
		}
		if (cost < best_cost) {
			best_cost = cost;
			best_shape = shape;
			for (int k = 0; k < 4; ++k) { best_mv[k][0] = mv[k][0]; best_mv[k][1] = mv[k][1]; }
		}
	}

	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			int i = (mby * 4 + y) * lstride + mbx * 4 + x;
			e->pic.mv[i * 2] = saved_mv[(y * 4 + x) * 2];
			e->pic.mv[i * 2 + 1] = saved_mv[(y * 4 + x) * 2 + 1];
			e->pic.ref_idx[i] = saved_ref[y * 4 + x];
		}
	}

	// Build the prediction with the chosen shape, recording each partition's vector as it goes so
	// the next partition predicts from it exactly as a decoder will.
	uint8_t pred[256], cpred[128];
	int pmv[4][2];
	unsigned done = 0;
	int nparts = ch_part_count[best_shape];
	for (int k = 0; k < nparts; ++k) {
		const int8_t* s = ch_part_shape[best_shape][k];
		ch_mv_predict_part(&e->pic, mbx, mby, mbx * 4 + s[0], mby * 4 + s[1], s[2], done,
		                   ch_part_dir[best_shape][k], &pmv[k][0], &pmv[k][1]);
		ch_mc_luma_block(&e->pic, mbx * 16 + s[0] * 4, mby * 16 + s[1] * 4, s[2] * 4, s[3] * 4,
		                 best_mv[k][0], best_mv[k][1], pred + (s[1] * 4) * 16 + s[0] * 4, 16);
		ch_mc_chroma_block(&e->pic, mbx * 8 + s[0] * 2, mby * 8 + s[1] * 2, s[2] * 2, s[3] * 2,
		                   best_mv[k][0], best_mv[k][1], cpred + (s[1] * 2) * 8 + s[0] * 2, 8);
		ch_set_motion_part(&e->pic, mbx * 4 + s[0], mby * 4 + s[1], s[2], s[3], best_mv[k][0], best_mv[k][1]);
		done |= ch_rect_bits(s[0], s[1], s[2], s[3]);
	}

	// Residual. Inter macroblocks have no separate DC block, so each 4x4 carries all sixteen of
	// its coefficients, exactly as in I_4x4.
	int zz[16][16];
	int cbp_luma = 0;
	for (int b = 0; b < 16; ++b) {
		int bx = ch_blk_x[b], by = ch_blk_y[b];
		int16_t blk[16], coef[16];
		int32_t deq[16];
		for (int y = 0; y < 4; ++y)
			for (int x = 0; x < 4; ++x)
				blk[y * 4 + x] = (int16_t)(sy[(size_t)(by * 4 + y) * e->luma_stride + bx * 4 + x]
				                         - pred[(by * 4 + y) * 16 + bx * 4 + x]);
		ch_fdct4x4(blk, coef);
		CUTE_H264_MEMSET(deq, 0, sizeof(deq));
		if (ch_quant_block(coef, e->qp, 0, zz[b], deq)) cbp_luma |= 1 << (b >> 2);
		ch_reconstruct_block(deq, pred + (by * 4) * 16 + bx * 4, 16,
			ry + (size_t)(by * 4) * e->luma_stride + bx * 4, e->luma_stride);
	}

	ch_chroma_t chroma;
	ch_encode_chroma(e, mbx, mby, cpred, &chroma);
	int cbp = cbp_luma | (chroma.cbp << 4);

	ch_ue(w, (uint32_t)best_shape);   // mb_type: 0 is 16x16, 1 is 16x8, 2 is 8x16, 3 is 8x8
	if (best_shape == 3) {
		// Each 8x8 could itself be split further; this encoder does not, so all four say so.
		for (int k = 0; k < 4; ++k) ch_ue(w, 0);   // sub_mb_type P_L0_8x8
	}
	// ref_idx_l0 is absent: there is exactly one reference frame, so there is nothing to choose.
	for (int k = 0; k < nparts; ++k) {
		ch_se(w, best_mv[k][0] - pmv[k][0]);
		ch_se(w, best_mv[k][1] - pmv[k][1]);
	}
	ch_ue(w, ch_cbp_inter_codenum[cbp]);
	if (cbp) ch_se(w, 0);             // mb_qp_delta

	for (int b = 0; b < 16; ++b) {
		int bx = ch_blk_x[b], by = ch_blk_y[b];
		int n = 0;
		if (cbp_luma & (1 << (b >> 2))) {
			int nc = ch_nc(lmap, lstride, bx, by, have_l, have_t);
			n = ch_write_residual(w, zz[b], 16, nc);
		}
		lmap[by * lstride + bx] = (uint8_t)n;
	}
	ch_write_chroma(e, mbx, mby, have_l, have_t, &chroma);

	// An inter macroblock still has to leave an intra prediction mode behind for its neighbours.
	uint8_t* mmap = e->i4_mode + (size_t)(mby * 4) * lstride + mbx * 4;
	for (int by = 0; by < 4; ++by)
		for (int bx = 0; bx < 4; ++bx) mmap[by * lstride + bx] = 2;
	return ch_mb_ssd(e, mbx, mby);
}

// Picks between the macroblock types by encoding both and keeping the cheaper one.
// Comparing bits alone would be wrong: at a fixed QP the two types do not produce the same
// quality, so the choice has to weigh distortion against rate rather than just counting bits.
static int ch_encode_mb(ch_encoder_t* e, int mbx, int mby, int* skip_run)
{
	ch_mb_state_t before;
	ch_bits_mark_t mark = ch_bits_mark(&e->bits);
	int base = ch_bits_count(&e->bits);
	uint32_t lambda = ch_lambda16[e->qp];
	ch_mb_copy(e, mbx, mby, &before, 1);

	// Skipping is evaluated first because it is the one option whose cost is not a number of bits
	// in this macroblock: it lengthens a run counter instead, which is close enough to free that
	// pricing it at a bit or so is fair.
	int64_t cost_skip = 0;
	int smvx = 0, smvy = 0;
	uint8_t spred[256], scpred[128];
	if (e->mb_type_offset) {
		ch_skip_mv(&e->pic, mbx, mby, &smvx, &smvy);
		ch_mc_luma(&e->pic, mbx, mby, smvx, smvy, spred);
		ch_mc_chroma(&e->pic, mbx, mby, smvx, smvy, scpred);
		const uint8_t* sy = e->y + (size_t)(mby * 16) * e->luma_stride + mbx * 16;
		int64_t ssd = 0;
		for (int y = 0; y < 16; ++y) {
			for (int x = 0; x < 16; ++x) {
				int d = sy[y * (size_t)e->luma_stride + x] - spred[y * 16 + x];
				ssd += d * d;
			}
		}
		for (int c = 0; c < 2; ++c) {
			const uint8_t* sc = (c ? e->cr : e->cb) + (size_t)(mby * 8) * e->chroma_stride + mbx * 8;
			for (int y = 0; y < 8; ++y) {
				for (int x = 0; x < 8; ++x) {
					int d = sc[y * (size_t)e->chroma_stride + x] - scpred[c * 64 + y * 8 + x];
					ssd += d * d;
				}
			}
		}
		cost_skip = (ssd << 4) + (int64_t)lambda;
	}

	int64_t best = 0;
	int winner = 0;
	for (int pass = 0; pass < 3; ++pass) {
		// In an I slice there is no reference frame to predict from, so the inter pass does not
		// exist. Elsewhere all three compete and the cheapest wins.
		if (pass == 2 && !e->mb_type_offset) continue;
		if (pass) { ch_bits_rewind(&e->bits, mark); ch_mb_copy(e, mbx, mby, &before, 0); }
		// The run counter is written ahead of the macroblock, so it belongs inside the trial for
		// the rewind to be able to take it back if skipping turns out to win.
		if (e->mb_type_offset) ch_ue(&e->bits, (uint32_t)*skip_run);
		int64_t ssd = pass == 0 ? ch_encode_mb_i16(e, mbx, mby)
		            : pass == 1 ? ch_encode_mb_i4(e, mbx, mby)
		                        : ch_encode_mb_p(e, mbx, mby);
		int64_t cost = (ssd << 4) + (int64_t)lambda * (ch_bits_count(&e->bits) - base);
		if (!pass || cost < best) { best = cost; winner = pass; }
	}
	if (e->mb_type_offset && cost_skip <= best) {
		ch_bits_rewind(&e->bits, mark);
		ch_mb_copy(e, mbx, mby, &before, 0);
		ch_commit_skip(e, mbx, mby, smvx, smvy, spred, scpred);
		++*skip_run;
		return 1;
	}
	int last_pass = e->mb_type_offset ? 2 : 1;
	if (winner != last_pass) {
		// Every pass is deterministic, so re-running the winner reproduces its bits exactly.
		ch_bits_rewind(&e->bits, mark);
		ch_mb_copy(e, mbx, mby, &before, 0);
		if (e->mb_type_offset) ch_ue(&e->bits, (uint32_t)*skip_run);
		if (winner == 0) ch_encode_mb_i16(e, mbx, mby);
		else if (winner == 1) ch_encode_mb_i4(e, mbx, mby);
		else ch_encode_mb_p(e, mbx, mby);
	}
	*skip_run = 0;
	return 0;
}


//--------------------------------------------------------------------------------------------------
// Deblocking. Block-based coding leaves visible seams on block boundaries at low bitrates, and the
// filter smooths them -- but only where the step across the edge looks like a coding artifact
// rather than a real edge in the picture, which is what the alpha and beta thresholds decide.
//
// This is in the decoding loop, not a post-process. The filtered picture is what the next frame
// predicts from, so encoder and decoder have to produce identical output here or they drift apart
// frame by frame. Intra prediction is the exception and deliberately reads the UNFILTERED
// reconstruction, which is why both pictures are kept.

// Spec Tables 8-16 and 8-17, indexed by the quantizer. Both thresholds are zero below QP 16: at
// high quality there is no blocking to remove and filtering would only lose detail.
static const uint8_t ch_alpha_tab[52] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,4,4,5,6,7,8,9,10,12,13,
	15,17,20,22,25,28,32,36,40,45,50,56,63,
	71,80,90,101,113,127,144,162,182,203,226,255,255,
};
static const uint8_t ch_beta_tab[52] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,2,2,2,3,3,3,3,4,4,4,
	6,6,7,7,8,8,9,9,10,10,11,11,12,
	12,13,13,14,14,15,15,16,16,17,17,18,18,
};
static const uint8_t ch_tc0_tab[3][52] = {
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
	  1,1,1,1,1,1,1,2,2,2,2,3,3,3,4,4,4,5,6,6,7,8,9,10,11,13 },
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,
	  1,1,1,1,1,2,2,2,2,3,3,3,4,4,5,5,6,7,8,8,10,11,12,13,15,17 },
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,
	  1,2,2,2,2,3,3,3,4,4,4,5,6,6,7,8,9,10,11,13,14,16,18,20,23,25 },
};

static int ch_abs(int v) { return v < 0 ? -v : v; }

// The boundary strength for one edge, spec clause 8.7.2.1. It is how hard to filter, and it rises
// with how likely the edge is to be an artifact: an intra macroblock boundary is the worst case,
// while two blocks that moved together and coded nothing cannot have gained a seam at all.
static int ch_bs(const ch_pic_t* p, int pbx, int pby, int qbx, int qby, int mb_edge)
{
	int stride = p->mb_w * 4;
	int ip = pby * stride + pbx, iq = qby * stride + qbx;
	if (p->ref_idx[ip] < 0 || p->ref_idx[iq] < 0) return mb_edge ? 4 : 3;
	if (p->nz_luma[ip] || p->nz_luma[iq]) return 2;
	// With a single reference picture the vectors are the only thing left that can differ. A whole
	// sample of disagreement is the threshold, and vectors are in quarter samples.
	int dx = ch_abs(p->mv[ip * 2] - p->mv[iq * 2]);
	int dy = ch_abs(p->mv[ip * 2 + 1] - p->mv[iq * 2 + 1]);
	return (dx >= 4 || dy >= 4) ? 1 : 0;
}

// Filters one line of samples across an edge, spec clauses 8.7.2.3 and 8.7.2.4. `s` points at q0
// and `step` is the distance to the next sample across the edge, so the same code serves vertical
// and horizontal edges. chroma_style suppresses the outer taps: chroma has half the resolution and
// filtering three samples deep into an 8-sample block would reach most of the way across it.
static void ch_filter_line(uint8_t* s, int step, int bS, int alpha, int beta, int tc0, int chroma_style)
{
	int p0 = s[-step], p1 = s[-2 * step], p2 = s[-3 * step], p3 = s[-4 * step];
	int q0 = s[0], q1 = s[step], q2 = s[2 * step], q3 = s[3 * step];
	// The thresholds are the whole point: a step this large is a real edge in the picture, and
	// smoothing it would be destroying detail rather than removing an artifact.
	if (!(ch_abs(p0 - q0) < alpha && ch_abs(p1 - p0) < beta && ch_abs(q1 - q0) < beta)) return;
	int ap = ch_abs(p2 - p0), aq = ch_abs(q2 - q0);
	if (bS < 4) {
		int tc = chroma_style ? tc0 + 1 : tc0 + (ap < beta) + (aq < beta);
		int d = ch_clamp((((q0 - p0) << 2) + (p1 - q1) + 4) >> 3, -tc, tc);
		s[-step] = (uint8_t)ch_clip255(p0 + d);
		s[0] = (uint8_t)ch_clip255(q0 - d);
		if (!chroma_style && ap < beta)
			s[-2 * step] = (uint8_t)(p1 + ch_clamp((p2 + ((p0 + q0 + 1) >> 1) - (p1 << 1)) >> 1, -tc0, tc0));
		if (!chroma_style && aq < beta)
			s[step] = (uint8_t)(q1 + ch_clamp((q2 + ((p0 + q0 + 1) >> 1) - (q1 << 1)) >> 1, -tc0, tc0));
	} else {
		// bS 4 only happens on an intra macroblock edge, where the seam is worst and a much
		// wider filter is allowed -- but only if the surroundings are flat enough to justify it.
		int flat = ch_abs(p0 - q0) < ((alpha >> 2) + 2);
		if (!chroma_style && ap < beta && flat) {
			s[-step]     = (uint8_t)((p2 + 2 * p1 + 2 * p0 + 2 * q0 + q1 + 4) >> 3);
			s[-2 * step] = (uint8_t)((p2 + p1 + p0 + q0 + 2) >> 2);
			s[-3 * step] = (uint8_t)((2 * p3 + 3 * p2 + p1 + p0 + q0 + 4) >> 3);
		} else {
			s[-step] = (uint8_t)((2 * p1 + p0 + q1 + 2) >> 2);
		}
		if (!chroma_style && aq < beta && flat) {
			s[0]        = (uint8_t)((p1 + 2 * p0 + 2 * q0 + 2 * q1 + q2 + 4) >> 3);
			s[step]     = (uint8_t)((p0 + q0 + q1 + q2 + 2) >> 2);
			s[2 * step] = (uint8_t)((2 * q3 + 3 * q2 + q1 + q0 + p0 + 4) >> 3);
		} else {
			s[0] = (uint8_t)((2 * q1 + q0 + p1 + 2) >> 2);
		}
	}
}

// Filters the whole picture in place. Macroblocks are walked in raster order, and within each one
// every vertical edge is filtered left to right before any horizontal edge is filtered top to
// bottom. The order is not incidental: each edge reads samples the previous edge already changed,
// so a decoder that ran them in a different order would get different pixels.
static void ch_deblock(ch_pic_t* p, int qp)
{
	int ia_y = ch_clamp(qp, 0, 51);
	int ia_c = ch_clamp(ch_chroma_qp(qp), 0, 51);
	int alpha_y = ch_alpha_tab[ia_y], beta_y = ch_beta_tab[ia_y];
	int alpha_c = ch_alpha_tab[ia_c], beta_c = ch_beta_tab[ia_c];
	for (int mby = 0; mby < p->mb_h; ++mby) {
		for (int mbx = 0; mbx < p->mb_w; ++mbx) {
			for (int dir = 0; dir < 2; ++dir) {
				int vertical = dir == 0;
				for (int edge = 0; edge < 16; edge += 4) {
					// Picture boundaries have nothing on the other side to filter against.
					if (edge == 0 && (vertical ? mbx == 0 : mby == 0)) continue;
					int mb_edge = edge == 0;
					for (int g = 0; g < 16; g += 4) {
						int qbx = mbx * 4 + (vertical ? edge / 4 : g / 4);
						int qby = mby * 4 + (vertical ? g / 4 : edge / 4);
						int bs = ch_bs(p, qbx - (vertical ? 1 : 0), qby - (vertical ? 0 : 1),
						               qbx, qby, mb_edge);
						if (!bs) continue;
						int tc0 = bs < 4 ? ch_tc0_tab[bs - 1][ia_y] : 0;
						for (int k = 0; k < 4; ++k) {
							int x = mbx * 16 + (vertical ? edge : g + k);
							int y = mby * 16 + (vertical ? g + k : edge);
							uint8_t* s = p->ref_y + (size_t)y * p->luma_stride + x;
							ch_filter_line(s, vertical ? 1 : p->luma_stride,
								bs, alpha_y, beta_y, tc0, 0);
						}
					}
					// In 4:2:0 only every other luma edge has a chroma edge on it, and the
					// strength is the one already worked out for the luma edge it sits on.
					if (edge != 0 && edge != 8) continue;
					// Two chroma samples per luma block, not four: chroma is half resolution, so a
					// run of four chroma samples along the edge spans TWO luma blocks and can
					// straddle a change in strength. Filtering four at a time reads the wrong
					// block for half of them, which an all-intra picture hides completely because
					// its strength is uniform.
					for (int g = 0; g < 8; g += 2) {
						int qbx = mbx * 4 + (vertical ? edge / 4 : g / 2);
						int qby = mby * 4 + (vertical ? g / 2 : edge / 4);
						int bs = ch_bs(p, qbx - (vertical ? 1 : 0), qby - (vertical ? 0 : 1),
						               qbx, qby, mb_edge);
						if (!bs) continue;
						int tc0 = bs < 4 ? ch_tc0_tab[bs - 1][ia_c] : 0;
						for (int c = 0; c < 2; ++c) {
							uint8_t* plane = c ? p->ref_cr : p->ref_cb;
							for (int k = 0; k < 2; ++k) {
								int x = mbx * 8 + (vertical ? edge / 2 : g + k);
								int y = mby * 8 + (vertical ? g + k : edge / 2);
								uint8_t* s = plane + (size_t)y * p->chroma_stride + x;
								ch_filter_line(s, vertical ? 1 : p->chroma_stride,
									bs, alpha_c, beta_c, tc0, 1);
							}
						}
					}
				}
			}
		}
	}
}

static void ch_write_slice(ch_encoder_t* e, int idr)
{
	ch_bits_t* w = &e->bits;
	ch_bits_reset(w);
	ch_ue(w, 0);                      // first_mb_in_slice
	ch_ue(w, idr ? 7 : 5);            // slice_type: I or P, "all slices in this picture are alike"
	ch_ue(w, 0);                      // pic_parameter_set_id
	ch_put_bits(w, (uint32_t)(e->frame_num & 15), 4); // frame_num, log2_max_frame_num = 4 bits
	if (idr) {
		ch_ue(w, (uint32_t)e->idr_pic_id);
	} else {
		ch_put_bit(w, 0);             // num_ref_idx_active_override_flag: the PPS default of 1 ref
		ch_put_bit(w, 0);             // ref_pic_list_modification_flag_l0: default order
	}
	// dec_ref_pic_marking
	if (idr) {
		ch_put_bit(w, 0);             // no_output_of_prior_pics_flag
		ch_put_bit(w, 0);             // long_term_reference_flag
	} else {
		ch_put_bit(w, 0);             // adaptive_ref_pic_marking_mode_flag: sliding window
	}
	ch_se(w, e->qp < 0 ? 0 : e->qp - 26); // slice_qp_delta, against the PPS's QP 26
	// The lossless path leaves the filter off, and has to: I_PCM samples are exact by definition
	// and a filter running across their edges would smear samples the encoder never touched,
	// costing the round trip its bit-exactness for no benefit.
	if (e->qp < 0) {
		ch_ue(w, 1);                  // disable_deblocking_filter_idc: 1 = off
	} else {
		ch_ue(w, 0);                  // disable_deblocking_filter_idc: 0 = on
		ch_se(w, 0);                  // slice_alpha_c0_offset_div2
		ch_se(w, 0);                  // slice_beta_offset_div2
	}

	int skip_run = 0;
	for (int mby = 0; mby < e->mb_h; ++mby) {
		for (int mbx = 0; mbx < e->mb_w; ++mbx) {
			// In a P slice each coded macroblock is preceded by a count of the skipped macroblocks
			// before it, which ch_encode_mb writes itself so that it can be taken back if skipping
			// this one turns out to be cheaper than coding it.
			if (e->qp >= 0) { ch_encode_mb(e, mbx, mby, &skip_run); continue; }
			ch_ue(w, 25);             // mb_type 25 in an I slice is I_PCM
			ch_align_zero(w);         // pcm_alignment_zero_bit
			// I_PCM samples ARE their own reconstruction. Writing them through keeps one rule for
			// where a decoded picture lives instead of a special case for the lossless path.
			for (int y = 0; y < 16; ++y)
				CUTE_H264_MEMCPY(e->rec_y + (size_t)(mby * 16 + y) * e->luma_stride + mbx * 16,
					e->y + (size_t)(mby * 16 + y) * e->luma_stride + mbx * 16, 16);
			for (int c = 0; c < 2; ++c) {
				const uint8_t* sp = c ? e->cr : e->cb;
				uint8_t* rp = c ? e->rec_cr : e->rec_cb;
				for (int y = 0; y < 8; ++y)
					CUTE_H264_MEMCPY(rp + (size_t)(mby * 8 + y) * e->chroma_stride + mbx * 8,
						sp + (size_t)(mby * 8 + y) * e->chroma_stride + mbx * 8, 8);
			}
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
	// A slice that ends on a run of skipped macroblocks still has to say so. The decoder stops on
	// running out of data, not on a macroblock count, so the trailing run goes out and then the
	// stop bit tells it there is nothing further.
	if (!idr && skip_run) ch_ue(w, (uint32_t)skip_run);
	ch_rbsp_trailing(w);
	ch_emit_nal(&e->out, 3, idr ? CH_NAL_SLICE_IDR : CH_NAL_SLICE, w->bytes.data, w->bytes.len);
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
	e->y = (uint8_t*)CUTE_H264_ALLOC((luma + chroma * 2) * 3 + nzl * 2 + nzc * 2);
	if (!e->y) { CUTE_H264_FREE(e); ch_error_reason = "Out of memory."; return NULL; }
	// The motion field is allocated on its own only because it is not bytes, and folding a short
	// array into the middle of a byte block is a needless alignment question to have to answer.
	e->mv = (int16_t*)CUTE_H264_ALLOC(nzl * (sizeof(int16_t) * 2 + 1));
	if (!e->mv) { CUTE_H264_FREE(e->y); CUTE_H264_FREE(e); ch_error_reason = "Out of memory."; return NULL; }
	e->ref_idx = (int8_t*)(e->mv + nzl * 2);
	e->cb = e->y + luma;
	e->cr = e->cb + chroma;
	e->rec_y = e->cr + chroma;
	e->rec_cb = e->rec_y + luma;
	e->rec_cr = e->rec_cb + chroma;
	e->nz_luma = e->rec_cr + chroma;
	e->nz_cb = e->nz_luma + nzl;
	e->nz_cr = e->nz_cb + nzc;
	e->i4_mode = e->nz_cr + nzc;
	e->ref_y = e->i4_mode + nzl;
	e->ref_cb = e->ref_y + luma;
	e->ref_cr = e->ref_cb + chroma;
	e->pic.mb_w = e->mb_w;
	e->pic.mb_h = e->mb_h;
	e->pic.luma_stride = e->luma_stride;
	e->pic.chroma_stride = e->chroma_stride;
	e->pic.rec_y = e->rec_y;   e->pic.rec_cb = e->rec_cb;  e->pic.rec_cr = e->rec_cr;
	e->pic.ref_y = e->ref_y;   e->pic.ref_cb = e->ref_cb;  e->pic.ref_cr = e->ref_cr;
	e->pic.nz_luma = e->nz_luma;
	e->pic.mv = e->mv;
	e->pic.ref_idx = e->ref_idx;
	return e;
}

void ch_encoder_destroy(ch_encoder_t* e)
{
	if (!e) return;
	CUTE_H264_FREE(e->y);
	CUTE_H264_FREE(e->mv);
	CUTE_H264_FREE(e->bits.bytes.data);
	CUTE_H264_FREE(e->out.data);
	CUTE_H264_FREE(e);
}

// Copies planar 4:2:0 straight in, replicating the edges into the macroblock padding the same way
// the RGBA path does. Anything that already has YUV -- a camera, a hardware capture, a decoder --
// would otherwise pay for a conversion to RGB and back for nothing.
static void ch_copy_yuv420(ch_encoder_t* e, const uint8_t* y, const uint8_t* cb, const uint8_t* cr,
                           int y_stride, int c_stride)
{
	int pw = e->mb_w * 16, ph = e->mb_h * 16;
	for (int j = 0; j < ph; ++j) {
		int sj = j < e->h ? j : e->h - 1;
		for (int i = 0; i < pw; ++i) {
			int si = i < e->w ? i : e->w - 1;
			e->y[(size_t)j * e->luma_stride + i] = y[(size_t)sj * y_stride + si];
		}
	}
	for (int j = 0; j < ph / 2; ++j) {
		int sj = j < e->h / 2 ? j : e->h / 2 - 1;
		for (int i = 0; i < pw / 2; ++i) {
			int si = i < e->w / 2 ? i : e->w / 2 - 1;
			e->cb[(size_t)j * e->chroma_stride + i] = cb[(size_t)sj * c_stride + si];
			e->cr[(size_t)j * e->chroma_stride + i] = cr[(size_t)sj * c_stride + si];
		}
	}
}

static int ch_encoder_picture(ch_encoder_t* e);

int ch_encoder_frame_yuv(ch_encoder_t* e, const void* y, const void* cb, const void* cr,
                         int y_stride, int chroma_stride)
{
	if (!e || !y || !cb || !cr) { ch_error_reason = "Null encoder or planes."; return 0; }
	ch_copy_yuv420(e, (const uint8_t*)y, (const uint8_t*)cb, (const uint8_t*)cr, y_stride, chroma_stride);
	return ch_encoder_picture(e);
}

int ch_encoder_frame(ch_encoder_t* e, const void* rgba)
{
	if (!e || !rgba) { ch_error_reason = "Null encoder or pixels."; return 0; }
	ch_rgba_to_yuv420(e, (const uint8_t*)rgba);
	return ch_encoder_picture(e);
}

static int ch_encoder_picture(ch_encoder_t* e)
{
	// A keyframe every couple of seconds, so that a capture can be seeked and can be joined late,
	// rather than one at the start and never again. The lossless path stays all-keyframe: I_PCM
	// has nothing to gain from a reference frame it is not allowed to be approximate about.
	int idr = e->frame_count == 0 || e->qp < 0 || e->frame_count % (e->fps * 2) == 0;
	// Contexts do not survive a picture boundary: start them clean or the first macroblock row
	// inherits contexts from the previous picture and decodes as noise.
	CUTE_H264_MEMSET(e->nz_luma, 0, (size_t)(e->mb_w * 4) * (e->mb_h * 4));
	CUTE_H264_MEMSET(e->nz_cb, 0, (size_t)(e->mb_w * 2) * (e->mb_h * 2));
	CUTE_H264_MEMSET(e->nz_cr, 0, (size_t)(e->mb_w * 2) * (e->mb_h * 2));
	CUTE_H264_MEMSET(e->i4_mode, 2, (size_t)(e->mb_w * 4) * (e->mb_h * 4));
	if (idr) {
		e->frame_num = 0;
		CUTE_H264_MEMSET(e->ref_idx, -1, (size_t)(e->mb_w * 4) * (e->mb_h * 4));
		CUTE_H264_MEMSET(e->mv, 0, (size_t)(e->mb_w * 4) * (e->mb_h * 4) * sizeof(int16_t) * 2);
		// Parameter sets are repeated on every keyframe rather than written once. A capture that
		// gets cut, or that a player joins late, is then still decodable from any keyframe in it.
		ch_write_sps(e);
		ch_write_pps(e);
	}
	e->mb_type_offset = idr ? 0 : 5;
	ch_write_slice(e, idr);
	if (idr) e->idr_pic_id ^= 1;
	// The reference planes hold the DEBLOCKED picture, which is both what a decoder outputs and
	// what the next frame predicts from. The unfiltered reconstruction stays where it is, because
	// intra prediction within a picture is specified to read it rather than the filtered one.
	{
		size_t luma = (size_t)e->luma_stride * (e->mb_h * 16);
		size_t chroma = (size_t)e->chroma_stride * (e->mb_h * 8);
		CUTE_H264_MEMCPY(e->ref_y, e->rec_y, luma);
		CUTE_H264_MEMCPY(e->ref_cb, e->rec_cb, chroma);
		CUTE_H264_MEMCPY(e->ref_cr, e->rec_cr, chroma);
	}
	if (e->qp >= 0) ch_deblock(&e->pic, e->qp);
	e->frame_num = (e->frame_num + 1) & 15;
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

//--------------------------------------------------------------------------------------------------
// CAVLC decoding. Every table is matched by scanning it for the codeword that the next bits begin
// with. A decoder built for speed would build lookup trees instead, but scanning means the encoder
// and decoder read the SAME table rather than two representations of it that can disagree -- and
// a table that disagrees with itself is exactly the bug class that is hardest to find here.

static uint32_t ch_peek_bits(const ch_rbits_t* r, int n)
{
	uint32_t v = 0;
	for (int i = 0; i < n; ++i) {
		int p = r->pos + i;
		int b = (p < r->len * 8) ? ((r->data[p >> 3] >> (7 - (p & 7))) & 1) : 0;
		v = (v << 1) | (uint32_t)b;
	}
	return v;
}

static int ch_read_coeff_token(ch_rbits_t* r, int nC, int* t1_out)
{
	*t1_out = 0;
	if (nC >= 8) {
		// At this density the counts are near uniform and the spec drops the VLC for a flat code.
		uint32_t v = ch_get_bits(r, 6);
		if (v == 3) return 0;
		*t1_out = (int)(v & 3);
		return (int)(v >> 2) + 1;
	}
	const uint8_t* lens; const uint8_t* codes; int stride, cols;
	if (nC < 0)       { lens = &ch_ct_lenC[0][0]; codes = &ch_ct_codeC[0][0]; stride = 5; cols = 5; }
	else if (nC < 2)  { lens = &ch_ct_len0[0][0]; codes = &ch_ct_code0[0][0]; stride = 17; cols = 17; }
	else if (nC < 4)  { lens = &ch_ct_len1[0][0]; codes = &ch_ct_code1[0][0]; stride = 17; cols = 17; }
	else              { lens = &ch_ct_len2[0][0]; codes = &ch_ct_code2[0][0]; stride = 17; cols = 17; }
	uint32_t look = ch_peek_bits(r, 16);
	for (int t1 = 0; t1 < 4; ++t1) {
		for (int tc = 0; tc < cols; ++tc) {
			int l = lens[t1 * stride + tc];
			if (l && (look >> (16 - l)) == codes[t1 * stride + tc]) {
				r->pos += l;
				*t1_out = t1;
				return tc;
			}
		}
	}
	r->error = 1;
	return 0;
}

static int ch_read_total_zeros(ch_rbits_t* r, int total, int chroma_dc)
{
	const uint8_t* lens = chroma_dc ? &ch_tzc_len[total - 1][0] : &ch_tz_len[total - 1][0];
	const uint8_t* codes = chroma_dc ? &ch_tzc_code[total - 1][0] : &ch_tz_code[total - 1][0];
	int n = chroma_dc ? 4 : 16;
	uint32_t look = ch_peek_bits(r, 9);
	for (int tz = 0; tz < n; ++tz) {
		int l = lens[tz];
		if (l && (look >> (9 - l)) == codes[tz]) { r->pos += l; return tz; }
	}
	r->error = 1;
	return 0;
}

static int ch_read_run_before(ch_rbits_t* r, int zeros_left)
{
	int idx = (zeros_left > 7 ? 7 : zeros_left) - 1;
	uint32_t look = ch_peek_bits(r, 11);
	for (int run = 0; run < 16; ++run) {
		int l = ch_rb_len[idx][run];
		if (l && (look >> (11 - l)) == ch_rb_code[idx][run]) { r->pos += l; return run; }
	}
	r->error = 1;
	return 0;
}

// One coefficient level. The prefix is a run of zeros; past 15 of them the suffix grows a bit per
// extra zero, which is the same escape the writer needed for QP 0.
static int ch_read_level(ch_rbits_t* r, int suffix_len, int first_without_three_t1)
{
	int prefix = 0;
	while (!ch_get_bit(r)) {
		if (r->error || ++prefix > 31) { r->error = 1; return 0; }
	}
	int suffix_size = (prefix == 14 && suffix_len == 0) ? 4
	                : (prefix >= 15) ? prefix - 3
	                : suffix_len;
	int suffix = suffix_size ? (int)ch_get_bits(r, suffix_size) : 0;
	int code = ((prefix < 15 ? prefix : 15) << suffix_len) + suffix;
	if (prefix >= 15 && suffix_len == 0) code += 15;
	if (prefix >= 16) code += (1 << (prefix - 3)) - 4096;
	// The first level after fewer than three trailing ones cannot be +-1, so the writer shifted
	// its range down by two to buy a bit back.
	if (first_without_three_t1) code += 2;
	return (code & 1) ? ((-code - 1) >> 1) : ((code + 2) >> 1);
}

// Reads one block of coefficients into zig-zag order. Mirrors ch_write_residual exactly, including
// that levels arrive in reverse scan order and that the run before the FIRST coefficient is never
// sent -- it is whatever is left over once the others are accounted for.
static int ch_read_residual(ch_rbits_t* r, int* coeffs, int count, int nC)
{
	for (int i = 0; i < count; ++i) coeffs[i] = 0;
	int t1 = 0;
	int total = ch_read_coeff_token(r, nC, &t1);
	if (!total || total > count) { if (total > count) r->error = 1; return 0; }

	int levels[16], runs[16];
	for (int i = 0; i < 16; ++i) runs[i] = 0;
	for (int i = 0; i < t1; ++i) levels[total - 1 - i] = ch_get_bit(r) ? -1 : 1;

	int suffix_len = (total > 10 && t1 < 3) ? 1 : 0;
	for (int i = total - 1 - t1; i >= 0; --i) {
		int level = ch_read_level(r, suffix_len, i == total - 1 - t1 && t1 < 3);
		levels[i] = level;
		if (suffix_len == 0) suffix_len = 1;
		int mag = level < 0 ? -level : level;
		if (mag > (3 << (suffix_len - 1)) && suffix_len < 6) ++suffix_len;
	}

	int total_zeros = (total < count) ? ch_read_total_zeros(r, total, nC < 0) : 0;
	int zeros_left = total_zeros;
	for (int i = total - 1; i > 0 && zeros_left > 0; --i) {
		runs[i] = ch_read_run_before(r, zeros_left);
		zeros_left -= runs[i];
	}
	runs[0] = zeros_left;

	int pos = 0;
	for (int k = 0; k < total; ++k) {
		pos += runs[k];
		if (pos >= count) { r->error = 1; return 0; }
		coeffs[pos] = levels[k];
		++pos;
	}
	return total;
}

// The inverse of ch_quant_block: scales the decoded levels back up, still in raster order ready
// for the inverse transform.
static void ch_dequant_block(const int* zz, int qp, int skip_dc, int32_t* deq)
{
	int qp_per = qp / 6, qp_rem = qp % 6;
	for (int i = 0; i < 16; ++i) deq[i] = 0;
	for (int i = 0; i < 16; ++i) {
		int pos = ch_zigzag[i];
		if (skip_dc && pos == 0) continue;
		deq[pos] = (int32_t)(zz[i] * ch_dequant_v[qp_rem][ch_pos_class[pos]]) << qp_per;
	}
}

//--------------------------------------------------------------------------------------------------

// Reads and reconstructs the chroma of one macroblock. cpred is the prediction, which the caller
// supplies because intra and inter arrive at it completely differently.
static void ch_decode_chroma(ch_decoder_t* d, ch_rbits_t* r, int mbx, int mby,
                             const uint8_t* cpred, int cbp_chroma)
{
	int have_l = mbx > 0, have_t = mby > 0;
	int qpc = ch_chroma_qp(d->qp);
	int cqp_per = qpc / 6, cqp_rem = qpc % 6;
	int cstride = d->mb_w * 2;
	int dc[2][4];
	for (int c = 0; c < 2; ++c) for (int i = 0; i < 4; ++i) dc[c][i] = 0;
	if (cbp_chroma & 3) {
		for (int c = 0; c < 2; ++c) ch_read_residual(r, dc[c], 4, -1);
	}
	for (int c = 0; c < 2; ++c) {
		uint8_t* rc = (c ? d->pic.rec_cr : d->pic.rec_cb)
		            + (size_t)(mby * 8) * d->pic.chroma_stride + mbx * 8;
		uint8_t* cmap = (c ? d->nz_cr : d->nz_cb) + (size_t)(mby * 2) * cstride + mbx * 2;
		int gg[4];
		gg[0] = dc[c][0] + dc[c][1] + dc[c][2] + dc[c][3];
		gg[1] = dc[c][0] - dc[c][1] + dc[c][2] - dc[c][3];
		gg[2] = dc[c][0] + dc[c][1] - dc[c][2] - dc[c][3];
		gg[3] = dc[c][0] - dc[c][1] - dc[c][2] + dc[c][3];
		for (int b = 0; b < 4; ++b) {
			int bx = b & 1, by = b >> 1;
			int ac[16];
			int32_t deq[16];
			int n = 0;
			for (int i = 0; i < 16; ++i) ac[i] = 0;
			if (cbp_chroma & 2) {
				int nc = ch_nc(cmap, cstride, bx, by, have_l, have_t);
				n = ch_read_residual(r, ac + 1, 15, nc);
			}
			cmap[by * cstride + bx] = (uint8_t)n;
			ch_dequant_block(ac, qpc, 1, deq);
			deq[0] = ((int32_t)(gg[b] * ch_dequant_v[cqp_rem][0]) << (cqp_per + 4)) >> 5;
			ch_reconstruct_block(deq, cpred + c * 64 + (by * 4) * 8 + bx * 4, 8,
				rc + (size_t)(by * 4) * d->pic.chroma_stride + bx * 4, d->pic.chroma_stride);
		}
	}
}

static void ch_decode_mb(ch_decoder_t* d, ch_rbits_t* r, int mbx, int mby)
{
	int have_l = mbx > 0, have_t = mby > 0;
	int lstride = d->mb_w * 4, cstride = d->mb_w * 2;
	uint8_t* lmap = d->pic.nz_luma + (size_t)(mby * 4) * lstride + mbx * 4;
	uint8_t* mmap = d->i4_mode + (size_t)(mby * 4) * lstride + mbx * 4;
	uint8_t* ry = d->pic.rec_y + (size_t)(mby * 16) * d->pic.luma_stride + mbx * 16;

	int mb_type = (int)ch_get_ue(r);
	if (r->error) return;
	int is_inter = 0;
	if (d->slice_is_p) {
		if (mb_type < 5) {
			// P_8x8ref0 differs from P_8x8 only in that ref_idx is inferred rather than sent,
			// and with a single reference frame it is never sent either way -- so the two parse
			// identically here.
			if (mb_type == 4) mb_type = 3;
			is_inter = 1;
		} else {
			mb_type -= 5;
		}
	}

	uint8_t pred[256], cpred[128];
	int cbp_luma = 0, cbp_chroma = 0;
	int i16 = 0, i16_mode = 0;

	if (!is_inter && mb_type == 25) {
		// I_PCM: raw samples, byte aligned.
		while (r->pos & 7) ch_get_bit(r);
		for (int y = 0; y < 16; ++y)
			for (int x = 0; x < 16; ++x)
				ry[(size_t)y * d->pic.luma_stride + x] = (uint8_t)ch_get_bits(r, 8);
		for (int c = 0; c < 2; ++c) {
			uint8_t* rc = (c ? d->pic.rec_cr : d->pic.rec_cb)
			            + (size_t)(mby * 8) * d->pic.chroma_stride + mbx * 8;
			for (int y = 0; y < 8; ++y)
				for (int x = 0; x < 8; ++x)
					rc[(size_t)y * d->pic.chroma_stride + x] = (uint8_t)ch_get_bits(r, 8);
		}
		for (int y = 0; y < 4; ++y)
			for (int x = 0; x < 4; ++x) { lmap[y * lstride + x] = 16; mmap[y * lstride + x] = 2; }
		for (int c = 0; c < 2; ++c) {
			uint8_t* cmap = (c ? d->nz_cr : d->nz_cb) + (size_t)(mby * 2) * cstride + mbx * 2;
			for (int y = 0; y < 2; ++y) for (int x = 0; x < 2; ++x) cmap[y * cstride + x] = 16;
		}
		ch_mark_intra_motion(&d->pic, mbx, mby);
		return;
	}

	int modes[16];
	if (is_inter) {
		// mb_type says how the macroblock is split; an 8x8 can then be split again, so the
		// sub types are read up front and the vectors after them.
		int nparts = ch_part_count[mb_type];
		int sub[4] = { 0, 0, 0, 0 };
		if (mb_type == 3) {
			for (int k = 0; k < 4; ++k) {
				sub[k] = (int)ch_get_ue(r);
				if (sub[k] > 3) { ch_decoder_error = "Invalid sub_mb_type."; r->error = 1; return; }
			}
		}
		unsigned done = 0;
		for (int k = 0; k < nparts; ++k) {
			const int8_t* s = ch_part_shape[mb_type][k];
			// An 8x8 splits into one 8x8, two 8x4, two 4x8 or four 4x4.
			static const int8_t sub_shape[4][4][4] = {
				{ { 0,0,2,2 } },
				{ { 0,0,2,1 }, { 0,1,2,1 } },
				{ { 0,0,1,2 }, { 1,0,1,2 } },
				{ { 0,0,1,1 }, { 1,0,1,1 }, { 0,1,1,1 }, { 1,1,1,1 } },
			};
			static const int sub_count[4] = { 1, 2, 2, 4 };
			int nsub = mb_type == 3 ? sub_count[sub[k]] : 1;
			for (int j = 0; j < nsub; ++j) {
				int bx4 = s[0], by4 = s[1], bw4 = s[2], bh4 = s[3];
				if (mb_type == 3) {
					const int8_t* q = sub_shape[sub[k]][j];
					bx4 = s[0] + q[0]; by4 = s[1] + q[1]; bw4 = q[2]; bh4 = q[3];
				}
				int pmvx, pmvy;
				ch_mv_predict_part(&d->pic, mbx, mby, mbx * 4 + bx4, mby * 4 + by4, bw4, done,
				                   mb_type == 3 ? 0 : ch_part_dir[mb_type][k], &pmvx, &pmvy);
				int px = pmvx + (int)ch_get_se(r);
				int py = pmvy + (int)ch_get_se(r);
				ch_mc_luma_block(&d->pic, mbx * 16 + bx4 * 4, mby * 16 + by4 * 4, bw4 * 4, bh4 * 4,
				                 px, py, pred + (by4 * 4) * 16 + bx4 * 4, 16);
				ch_mc_chroma_block(&d->pic, mbx * 8 + bx4 * 2, mby * 8 + by4 * 2, bw4 * 2, bh4 * 2,
				                   px, py, cpred + (by4 * 2) * 8 + bx4 * 2, 8);
				ch_set_motion_part(&d->pic, mbx * 4 + bx4, mby * 4 + by4, bw4, bh4, px, py);
				done |= ch_rect_bits(bx4, by4, bw4, bh4);
			}
		}
		cbp_luma = 0;
	} else if (mb_type == 0) {
		// I_NxN: sixteen prediction modes, each coded against a prediction of its own.
		for (int b = 0; b < 16; ++b) {
			int bx = ch_blk_x[b], by = ch_blk_y[b];
			int pm;
			if ((bx == 0 && !have_l) || (by == 0 && !have_t)) pm = 2;
			else {
				int ma = mmap[by * lstride + (bx - 1)];
				int mb2 = mmap[(by - 1) * lstride + bx];
				pm = ma < mb2 ? ma : mb2;
			}
			if (ch_get_bit(r)) modes[b] = pm;
			else {
				int rem = (int)ch_get_bits(r, 3);
				modes[b] = rem < pm ? rem : rem + 1;
			}
			mmap[by * lstride + bx] = (uint8_t)modes[b];
		}
	} else {
		i16 = 1;
		int mt = mb_type - 1;
		i16_mode = mt % 4;
		cbp_chroma = (mt % 12) / 4;
		cbp_luma = (mt / 12) ? 15 : 0;
	}

	int chroma_mode = 0;
	if (!is_inter) {
		chroma_mode = (int)ch_get_ue(r);      // intra_chroma_pred_mode
		if (chroma_mode > 3) { ch_decoder_error = "Invalid intra_chroma_pred_mode."; r->error = 1; return; }
	}

	if (i16) {
		// The prediction mode came out of mb_type, so it is applied rather than searched for.
		// Plane is here even though this encoder never picks it, because a stream from any other
		// encoder will, and a decoder that only handles what its own encoder emits is not one.
		int stride = d->pic.luma_stride;
		if (i16_mode == 0) {
			for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x)
				pred[y * 16 + x] = ry[-(ptrdiff_t)stride + x];
		} else if (i16_mode == 1) {
			for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x)
				pred[y * 16 + x] = ry[(size_t)y * stride - 1];
		} else if (i16_mode == 2) {
			int sum = 0, cnt = 0;
			if (have_t) { for (int x = 0; x < 16; ++x) sum += ry[-(ptrdiff_t)stride + x]; cnt += 16; }
			if (have_l) { for (int y = 0; y < 16; ++y) sum += ry[(size_t)y * stride - 1]; cnt += 16; }
			int dc = cnt ? (sum + (cnt >> 1)) / cnt : 128;
			for (int i = 0; i < 256; ++i) pred[i] = (uint8_t)dc;
		} else {
			// Plane, clause 8.3.3.4: a linear ramp fitted to the top row and left column.
			int hh = 0, vv = 0;
			for (int i = 0; i < 8; ++i) {
				hh += (i + 1) * (ry[-(ptrdiff_t)stride + 8 + i] - ry[-(ptrdiff_t)stride + 6 - i]);
				vv += (i + 1) * (ry[(size_t)(8 + i) * stride - 1] - ry[(ptrdiff_t)(6 - i) * (ptrdiff_t)stride - 1]);
			}
			int a = 16 * (ry[(size_t)15 * stride - 1] + ry[-(ptrdiff_t)stride + 15]);
			int b = (5 * hh + 32) >> 6, c = (5 * vv + 32) >> 6;
			for (int y = 0; y < 16; ++y)
				for (int x = 0; x < 16; ++x)
					pred[y * 16 + x] = (uint8_t)ch_clip255((a + b * (x - 7) + c * (y - 7) + 16) >> 5);
		}
	}

	if (!i16) {
		uint32_t code = ch_get_ue(r);
		if (code > 47) { ch_decoder_error = "Invalid coded_block_pattern."; r->error = 1; return; }
		const uint8_t* map = is_inter ? ch_cbp_inter_codenum : ch_cbp_intra_codenum;
		int cbp = -1;
		for (int i = 0; i < 48; ++i) if (map[i] == (uint8_t)code) { cbp = i; break; }
		if (cbp < 0) { ch_decoder_error = "Invalid coded_block_pattern."; r->error = 1; return; }
		cbp_luma = cbp & 15;
		cbp_chroma = cbp >> 4;
	}

	// mb_qp_delta is cumulative across the slice, not absolute, and it wraps rather than clamps.
	// This encoder always sends zero, but a stream from anywhere else will vary the quantizer per
	// macroblock and every residual after the first would be scaled wrongly without this.
	if (i16 || cbp_luma || cbp_chroma) {
		int delta = (int)ch_get_se(r);
		if (delta < -26 || delta > 25) { ch_decoder_error = "Invalid mb_qp_delta."; r->error = 1; return; }
		d->qp = (d->qp + delta + 52) % 52;
	}

	// Inter chroma already came from motion compensation; every intra type predicts it the same
	// way, so this is one branch rather than one per macroblock type.
	if (!is_inter) {
		for (int c = 0; c < 2; ++c) {
			uint8_t* rc = (c ? d->pic.rec_cr : d->pic.rec_cb)
			            + (size_t)(mby * 8) * d->pic.chroma_stride + mbx * 8;
			ch_pred8(rc, d->pic.chroma_stride, chroma_mode, have_l, have_t, cpred + c * 64);
		}
	}

	if (i16) {
		int dc_zz[16];
		int nc0 = ch_nc(lmap, lstride, 0, 0, have_l, have_t);
		ch_read_residual(r, dc_zz, 16, nc0);
		int32_t dc_lev[16], dc_deq[16];
		for (int i = 0; i < 16; ++i) dc_lev[ch_zigzag[i]] = dc_zz[i];
		ch_ihadamard4x4(dc_lev, dc_deq);
		int qp_per = d->qp / 6, qp_rem = d->qp % 6;
		for (int i = 0; i < 16; ++i) {
			int t = dc_deq[i] * ch_dequant_v[qp_rem][0];
			dc_deq[i] = qp_per >= 2 ? (t << (qp_per - 2)) : ((t + (1 << (1 - qp_per))) >> (2 - qp_per));
		}
		for (int b = 0; b < 16; ++b) {
			int bx = ch_blk_x[b], by = ch_blk_y[b];
			int ac[16];
			int32_t deq[16];
			int n = 0;
			for (int i = 0; i < 16; ++i) ac[i] = 0;
			if (cbp_luma) {
				int nc = ch_nc(lmap, lstride, bx, by, have_l, have_t);
				n = ch_read_residual(r, ac + 1, 15, nc);
			}
			lmap[by * lstride + bx] = (uint8_t)n;
			ch_dequant_block(ac, d->qp, 1, deq);
			deq[0] = dc_deq[by * 4 + bx];
			ch_reconstruct_block(deq, pred + (by * 4) * 16 + bx * 4, 16,
				ry + (size_t)(by * 4) * d->pic.luma_stride + bx * 4, d->pic.luma_stride);
		}
		for (int y = 0; y < 4; ++y)
			for (int x = 0; x < 4; ++x) mmap[y * lstride + x] = 2;
	} else {
		// I_NxN and inter both carry sixteen full 4x4 blocks, gated per 8x8 by the pattern.
		for (int b = 0; b < 16; ++b) {
			int bx = ch_blk_x[b], by = ch_blk_y[b];
			uint8_t* blk_rec = ry + (size_t)(by * 4) * d->pic.luma_stride + bx * 4;
			uint8_t* bpred;
			uint8_t p4[16];
			if (is_inter) {
				bpred = pred + (by * 4) * 16 + bx * 4;
			} else {
				// Each block predicts from the reconstruction of the ones before it, including
				// blocks inside this same macroblock, so this has to happen in coding order.
				int av_l = bx > 0 || have_l;
				int av_t = by > 0 || have_t;
				int av_tr = by == 0 ? (av_t && (bx < 3 || mbx + 1 < d->mb_w))
				                    : (bx < 3 && ch_blk_order[by - 1][bx + 1] < b);
				int avail = (av_l ? 1 : 0) | (av_t ? 2 : 0) | ((av_l && av_t) ? 4 : 0) | (av_tr ? 8 : 0);
				ch_pred4x4(blk_rec, d->pic.luma_stride, modes[b], avail, p4);
				bpred = p4;
			}
			int zz[16];
			int32_t deq[16];
			int n = 0;
			for (int i = 0; i < 16; ++i) zz[i] = 0;
			if (cbp_luma & (1 << (b >> 2))) {
				int nc = ch_nc(lmap, lstride, bx, by, have_l, have_t);
				n = ch_read_residual(r, zz, 16, nc);
			}
			lmap[by * lstride + bx] = (uint8_t)n;
			ch_dequant_block(zz, d->qp, 0, deq);
			ch_reconstruct_block(deq, bpred, is_inter ? 16 : 4, blk_rec, d->pic.luma_stride);
		}
		if (is_inter)
			for (int y = 0; y < 4; ++y)
				for (int x = 0; x < 4; ++x) mmap[y * lstride + x] = 2;
	}

	ch_decode_chroma(d, r, mbx, mby, cpred, cbp_chroma);
	if (!is_inter) ch_mark_intra_motion(&d->pic, mbx, mby);
}

// A skipped macroblock: no data at all, so it is entirely the prediction at a vector the decoder
// works out for itself.
static void ch_decode_skip(ch_decoder_t* d, int mbx, int mby)
{
	int lstride = d->mb_w * 4, cstride = d->mb_w * 2;
	int mvx, mvy;
	uint8_t pred[256], cpred[128];
	ch_skip_mv(&d->pic, mbx, mby, &mvx, &mvy);
	ch_mc_luma(&d->pic, mbx, mby, mvx, mvy, pred);
	ch_mc_chroma(&d->pic, mbx, mby, mvx, mvy, cpred);
	uint8_t* ry = d->pic.rec_y + (size_t)(mby * 16) * d->pic.luma_stride + mbx * 16;
	for (int y = 0; y < 16; ++y)
		CUTE_H264_MEMCPY(ry + (size_t)y * d->pic.luma_stride, pred + y * 16, 16);
	for (int c = 0; c < 2; ++c) {
		uint8_t* rc = (c ? d->pic.rec_cr : d->pic.rec_cb)
		            + (size_t)(mby * 8) * d->pic.chroma_stride + mbx * 8;
		for (int y = 0; y < 8; ++y)
			CUTE_H264_MEMCPY(rc + (size_t)y * d->pic.chroma_stride, cpred + c * 64 + y * 8, 8);
	}
	uint8_t* lmap = d->pic.nz_luma + (size_t)(mby * 4) * lstride + mbx * 4;
	uint8_t* mmap = d->i4_mode + (size_t)(mby * 4) * lstride + mbx * 4;
	for (int y = 0; y < 4; ++y)
		for (int x = 0; x < 4; ++x) { lmap[y * lstride + x] = 0; mmap[y * lstride + x] = 2; }
	for (int c = 0; c < 2; ++c) {
		uint8_t* cmap = (c ? d->nz_cr : d->nz_cb) + (size_t)(mby * 2) * cstride + mbx * 2;
		for (int y = 0; y < 2; ++y) for (int x = 0; x < 2; ++x) cmap[y * cstride + x] = 0;
	}
	ch_set_motion(&d->pic, mbx, mby, mvx, mvy);
}

static int ch_decode_slice(ch_decoder_t* d, ch_rbits_t* r, int idr)
{
	ch_get_ue(r);                                  // first_mb_in_slice
	int slice_type = (int)ch_get_ue(r);
	if (slice_type >= 5) slice_type -= 5;
	if (slice_type != 2 && slice_type != 0) { ch_decoder_error = "Only I and P slices are supported."; return 0; }
	d->slice_is_p = slice_type == 0;
	d->mb_type_offset = d->slice_is_p ? 5 : 0;
	ch_get_ue(r);                                  // pic_parameter_set_id
	ch_get_bits(r, d->log2_max_frame_num);         // frame_num
	if (idr) ch_get_ue(r);                         // idr_pic_id
	if (d->pic_order_cnt_type == 0) ch_get_bits(r, d->log2_max_poc_lsb); // pic_order_cnt_lsb
	if (d->slice_is_p) {
		int refs = d->num_ref_idx_l0;
		if (ch_get_bit(r)) refs = (int)ch_get_ue(r) + 1;   // num_ref_idx_active_override_flag
		// With more than one reference in the list every partition carries a ref_idx, so this is
		// not something that can be ignored and decoded approximately -- it shifts the bitstream.
		if (refs != 1) { ch_decoder_error = "Multiple reference frames are not supported."; return 0; }
		if (ch_get_bit(r)) { ch_decoder_error = "Reference list reordering is not supported."; return 0; }
	}
	if (idr) { ch_get_bit(r); ch_get_bit(r); }
	else ch_get_bit(r);                            // adaptive_ref_pic_marking_mode_flag
	d->qp = d->pps_qp + ch_get_se(r);
	int deblock = 1;
	if (d->deblock_control) {
		int idc = (int)ch_get_ue(r);
		deblock = idc != 1;
		if (idc != 1) { ch_get_se(r); ch_get_se(r); }
	}
	if (r->error || d->qp < 0 || d->qp > 51) { ch_decoder_error = "Bad slice header."; return 0; }

	CUTE_H264_MEMSET(d->pic.nz_luma, 0, (size_t)(d->mb_w * 4) * (d->mb_h * 4));
	CUTE_H264_MEMSET(d->nz_cb, 0, (size_t)(d->mb_w * 2) * (d->mb_h * 2));
	CUTE_H264_MEMSET(d->nz_cr, 0, (size_t)(d->mb_w * 2) * (d->mb_h * 2));
	CUTE_H264_MEMSET(d->i4_mode, 2, (size_t)(d->mb_w * 4) * (d->mb_h * 4));
	if (idr) {
		CUTE_H264_MEMSET(d->pic.ref_idx, -1, (size_t)(d->mb_w * 4) * (d->mb_h * 4));
		CUTE_H264_MEMSET(d->pic.mv, 0, (size_t)(d->mb_w * 4) * (d->mb_h * 4) * sizeof(int16_t) * 2);
	}

	// The slice ends when the data runs out, not at a macroblock count -- which is what lets a
	// trailing run of skipped macroblocks terminate a picture with nothing after it.
	int total = d->mb_w * d->mb_h;
	int mb = 0, more = 1;
	while (mb < total && more) {
		if (d->slice_is_p) {
			int run = (int)ch_get_ue(r);
			for (int i = 0; i < run && mb < total; ++i, ++mb)
				ch_decode_skip(d, mb % d->mb_w, mb / d->mb_w);
			if (run > 0) more = ch_more_rbsp(r);
		}
		if (!more || mb >= total) break;
		ch_decode_mb(d, r, mb % d->mb_w, mb / d->mb_w);
		if (r->error) { if (!ch_decoder_error) ch_decoder_error = "Corrupt macroblock data."; return 0; }
		++mb;
	}
	if (mb != total) { ch_decoder_error = "Slice ended before the picture was complete."; return 0; }

	size_t luma = (size_t)d->pic.luma_stride * (d->mb_h * 16);
	size_t chroma = (size_t)d->pic.chroma_stride * (d->mb_h * 8);
	CUTE_H264_MEMCPY(d->pic.ref_y, d->pic.rec_y, luma);
	CUTE_H264_MEMCPY(d->pic.ref_cb, d->pic.rec_cb, chroma);
	CUTE_H264_MEMCPY(d->pic.ref_cr, d->pic.rec_cr, chroma);
	if (deblock) ch_deblock(&d->pic, d->qp);
	d->has_picture = 1;
	return 1;
}

//--------------------------------------------------------------------------------------------------
// Decoder, public entry points.

ch_decoder_t* ch_decoder_make(const void* annexb, int size)
{
	ch_decoder_error = NULL;
	if (!annexb || size <= 0) { ch_decoder_error = "Null or empty stream."; return NULL; }
	ch_decoder_t* d = (ch_decoder_t*)CUTE_H264_ALLOC(sizeof(ch_decoder_t));
	if (!d) { ch_decoder_error = "Out of memory."; return NULL; }
	CUTE_H264_MEMSET(d, 0, sizeof(*d));
	d->stream = (const uint8_t*)annexb;
	d->stream_len = size;
	return d;
}

void ch_decoder_destroy(ch_decoder_t* d)
{
	if (!d) return;
	CUTE_H264_FREE(d->mem);
	CUTE_H264_FREE(d->pic.mv);
	CUTE_H264_FREE(d->rbsp.data);
	CUTE_H264_FREE(d->rgba.data);
	CUTE_H264_FREE(d);
}

int ch_decoder_next(ch_decoder_t* d)
{
	if (!d) { ch_decoder_error = "Null decoder."; return 0; }
	// Cleared per call so that a zero return can be told apart from a failure by testing it.
	ch_decoder_error = NULL;
	d->has_picture = 0;
	int nal_type = 0;
	while (ch_next_nal(d, &nal_type)) {
		if (d->rbsp.oom) { ch_decoder_error = "Out of memory."; return 0; }
		ch_rbits_t r;
		r.data = d->rbsp.data;
		r.len = d->rbsp.len;
		r.pos = 0;
		r.error = 0;
		if (nal_type == CH_NAL_SPS) {
			if (!ch_parse_sps(d, &r)) return 0;
		} else if (nal_type == CH_NAL_PPS) {
			if (!ch_parse_pps(d, &r)) return 0;
		} else if (nal_type == CH_NAL_SLICE || nal_type == CH_NAL_SLICE_IDR) {
			if (!d->have_sps || !d->have_pps) { ch_decoder_error = "Slice before its parameter sets."; return 0; }
			if (!ch_decoder_alloc(d)) return 0;
			if (!ch_decode_slice(d, &r, nal_type == CH_NAL_SLICE_IDR)) return 0;
			return 1;
		}
		// Anything else -- SEI, access unit delimiters, filler -- carries nothing this decoder
		// needs, and skipping it is what lets a stream from another encoder come in unedited.
	}
	return 0;
}

int ch_decoder_size(ch_decoder_t* d, int* w, int* h)
{
	if (w) *w = d ? d->w : 0;
	if (h) *h = d ? d->h : 0;
	return d && d->have_sps;
}

const void* ch_decoder_rgba(ch_decoder_t* d)
{
	if (!d || !d->has_picture) { ch_decoder_error = "No picture has been decoded."; return NULL; }
	d->rgba.len = 0;
	ch_bytes_reserve(&d->rgba, d->w * d->h * 4);
	if (d->rgba.oom) { ch_decoder_error = "Out of memory."; return NULL; }
	uint8_t* out = d->rgba.data;
	for (int y = 0; y < d->h; ++y) {
		const uint8_t* py = d->pic.ref_y + (size_t)y * d->pic.luma_stride;
		const uint8_t* pb = d->pic.ref_cb + (size_t)(y / 2) * d->pic.chroma_stride;
		const uint8_t* pr = d->pic.ref_cr + (size_t)(y / 2) * d->pic.chroma_stride;
		for (int x = 0; x < d->w; ++x) {
			// BT.601 studio swing, the inverse of what the encoder applied. Chroma is repeated
			// rather than interpolated, which is the plain reading of 4:2:0 and matches the box
			// filter the encoder used to make it.
			int c = py[x] - 16, dd = pb[x / 2] - 128, ee = pr[x / 2] - 128;
			out[0] = ch_clamp_u8((298 * c + 409 * ee + 128) >> 8);
			out[1] = ch_clamp_u8((298 * c - 100 * dd - 208 * ee + 128) >> 8);
			out[2] = ch_clamp_u8((298 * c + 516 * dd + 128) >> 8);
			out[3] = 255;
			out += 4;
		}
	}
	return d->rgba.data;
}

const void* ch_decoder_yuv(ch_decoder_t* d, int* luma_stride, int* chroma_stride, const void** cb, const void** cr)
{
	if (!d || !d->has_picture) { ch_decoder_error = "No picture has been decoded."; return NULL; }
	if (luma_stride) *luma_stride = d->pic.luma_stride;
	if (chroma_stride) *chroma_stride = d->pic.chroma_stride;
	if (cb) *cb = d->pic.ref_cb;
	if (cr) *cr = d->pic.ref_cr;
	return d->pic.ref_y;
}

//--------------------------------------------------------------------------------------------------
// MP4. The codec produces a bare stream of pictures; almost nothing outside a media player will
// open one of those. An MP4 wraps the same bytes with what everything else needs to know -- how
// long it is, what frame rate, which frames can be seeked to, and where a sound track would go.
// Nothing is re-encoded: the picture data goes in unchanged, and this is packaging.

static void ch_be16(ch_bytes_t* b, unsigned v)
{
	ch_bytes_push(b, (uint8_t)(v >> 8)); ch_bytes_push(b, (uint8_t)v);
}

static void ch_be32(ch_bytes_t* b, uint32_t v)
{
	ch_bytes_push(b, (uint8_t)(v >> 24)); ch_bytes_push(b, (uint8_t)(v >> 16));
	ch_bytes_push(b, (uint8_t)(v >> 8));  ch_bytes_push(b, (uint8_t)v);
}

static void ch_tag(ch_bytes_t* b, const char* t)
{
	for (int i = 0; i < 4; ++i) ch_bytes_push(b, (uint8_t)t[i]);
}

// Every box is a size followed by a four character type. The size is not known until the contents
// have been written, so it goes in as a placeholder and is patched afterwards.
static int ch_box(ch_bytes_t* b, const char* type)
{
	int at = b->len;
	ch_be32(b, 0);
	ch_tag(b, type);
	return at;
}

static void ch_box_end(ch_bytes_t* b, int at)
{
	if (b->oom) return;
	uint32_t size = (uint32_t)(b->len - at);
	b->data[at] = (uint8_t)(size >> 24); b->data[at + 1] = (uint8_t)(size >> 16);
	b->data[at + 2] = (uint8_t)(size >> 8); b->data[at + 3] = (uint8_t)size;
}

static void ch_full_box(ch_bytes_t* b, uint32_t version_flags)
{
	ch_be32(b, version_flags);
}

typedef struct ch_mp4_sample_t { int offset, size, key; } ch_mp4_sample_t;

// Walks the Annex-B stream, copying each picture into the media data as a length-prefixed unit --
// which is how MP4 stores them, instead of the start codes a raw stream uses. The parameter sets
// are pulled out rather than copied, because in MP4 they belong in the sample description.
static int ch_mp4_collect(const uint8_t* s, int n, ch_bytes_t* mdat, ch_mp4_sample_t* samples,
                          int max_samples, int* out_count, const uint8_t** sps, int* sps_len,
                          const uint8_t** pps, int* pps_len)
{
	int i = 0, count = 0;
	*sps = *pps = NULL;
	*sps_len = *pps_len = 0;
	while (i + 2 < n) {
		while (i + 2 < n && !(s[i] == 0 && s[i + 1] == 0 && s[i + 2] == 1)) ++i;
		if (i + 2 >= n) break;
		i += 3;
		int start = i;
		while (i + 2 < n && !(s[i] == 0 && s[i + 1] == 0 && s[i + 2] == 1)) ++i;
		int end = (i + 2 >= n) ? n : i;
		while (end > start && s[end - 1] == 0) --end;
		if (end <= start) continue;
		int type = s[start] & 0x1f;
		if (type == CH_NAL_SPS) { *sps = s + start; *sps_len = end - start; continue; }
		if (type == CH_NAL_PPS) { *pps = s + start; *pps_len = end - start; continue; }
		if (type != CH_NAL_SLICE && type != CH_NAL_SLICE_IDR) continue;
		if (count >= max_samples) return 0;
		samples[count].offset = mdat->len;
		samples[count].size = (end - start) + 4;
		samples[count].key = type == CH_NAL_SLICE_IDR;
		ch_be32(mdat, (uint32_t)(end - start));
		for (int k = start; k < end; ++k) ch_bytes_push(mdat, s[k]);
		++count;
	}
	*out_count = count;
	return count > 0;
}

static void ch_mp4_stbl(ch_bytes_t* b, const ch_mp4_sample_t* samples, int count, int mdat_base,
                        int w, int h, int delta, const uint8_t* sps, int sps_len,
                        const uint8_t* pps, int pps_len)
{
	int stbl = ch_box(b, "stbl");
	{
		int stsd = ch_box(b, "stsd");
		ch_full_box(b, 0);
		ch_be32(b, 1);                       // one sample description
		int avc1 = ch_box(b, "avc1");
		for (int i = 0; i < 6; ++i) ch_bytes_push(b, 0);
		ch_be16(b, 1);                       // data_reference_index
		ch_be16(b, 0); ch_be16(b, 0);        // pre_defined, reserved
		for (int i = 0; i < 12; ++i) ch_bytes_push(b, 0);
		ch_be16(b, (unsigned)w);
		ch_be16(b, (unsigned)h);
		ch_be32(b, 0x00480000);              // 72 dpi horizontal
		ch_be32(b, 0x00480000);              // 72 dpi vertical
		ch_be32(b, 0);
		ch_be16(b, 1);                       // frame_count
		for (int i = 0; i < 32; ++i) ch_bytes_push(b, 0);   // compressor name
		ch_be16(b, 0x0018);                  // depth
		ch_be16(b, 0xffff);                  // pre_defined
		{
			// The decoder configuration: the parameter sets, plus a note that each picture is
			// prefixed by a four byte length.
			int avcc = ch_box(b, "avcC");
			ch_bytes_push(b, 1);
			ch_bytes_push(b, sps_len > 1 ? sps[1] : 66);   // profile
			ch_bytes_push(b, sps_len > 2 ? sps[2] : 0);    // profile compatibility
			ch_bytes_push(b, sps_len > 3 ? sps[3] : 51);   // level
			ch_bytes_push(b, 0xff);          // 6 bits set, then lengthSizeMinusOne = 3
			ch_bytes_push(b, 0xe1);          // 3 bits set, then one sequence parameter set
			ch_be16(b, (unsigned)sps_len);
			for (int i = 0; i < sps_len; ++i) ch_bytes_push(b, sps[i]);
			ch_bytes_push(b, 1);
			ch_be16(b, (unsigned)pps_len);
			for (int i = 0; i < pps_len; ++i) ch_bytes_push(b, pps[i]);
			ch_box_end(b, avcc);
		}
		ch_box_end(b, avc1);
		ch_box_end(b, stsd);
	}
	{
		int stts = ch_box(b, "stts");        // every picture lasts the same time
		ch_full_box(b, 0);
		ch_be32(b, 1);
		ch_be32(b, (uint32_t)count);
		ch_be32(b, (uint32_t)delta);
		ch_box_end(b, stts);
	}
	{
		// Which pictures can be jumped to. Without this a player has to decode from the start,
		// which is what makes a raw stream feel unseekable.
		int keys = 0;
		for (int i = 0; i < count; ++i) keys += samples[i].key;
		int stss = ch_box(b, "stss");
		ch_full_box(b, 0);
		ch_be32(b, (uint32_t)keys);
		for (int i = 0; i < count; ++i) if (samples[i].key) ch_be32(b, (uint32_t)(i + 1));
		ch_box_end(b, stss);
	}
	{
		int stsc = ch_box(b, "stsc");        // one chunk holding everything
		ch_full_box(b, 0);
		ch_be32(b, 1);
		ch_be32(b, 1);
		ch_be32(b, (uint32_t)count);
		ch_be32(b, 1);
		ch_box_end(b, stsc);
	}
	{
		int stsz = ch_box(b, "stsz");
		ch_full_box(b, 0);
		ch_be32(b, 0);                       // sizes vary, so they are listed
		ch_be32(b, (uint32_t)count);
		for (int i = 0; i < count; ++i) ch_be32(b, (uint32_t)samples[i].size);
		ch_box_end(b, stsz);
	}
	{
		int stco = ch_box(b, "stco");
		ch_full_box(b, 0);
		ch_be32(b, 1);
		ch_be32(b, (uint32_t)mdat_base);
		ch_box_end(b, stco);
	}
	ch_box_end(b, stbl);
}

const void* ch_mp4_wrap(const void* annexb, int size, int w, int h, int fps, int* out_size)
{
	ch_error_reason = NULL;
	if (out_size) *out_size = 0;
	if (!annexb || size <= 0 || w <= 0 || h <= 0 || fps <= 0) {
		ch_error_reason = "Bad arguments."; return NULL;
	}
	const uint8_t* s = (const uint8_t*)annexb;
	// One sample per coded picture; count the start codes to bound it.
	int max_samples = 1;
	for (int i = 0; i + 2 < size; ++i) if (s[i] == 0 && s[i + 1] == 0 && s[i + 2] == 1) ++max_samples;
	ch_mp4_sample_t* samples = (ch_mp4_sample_t*)CUTE_H264_ALLOC(sizeof(ch_mp4_sample_t) * (size_t)max_samples);
	if (!samples) { ch_error_reason = "Out of memory."; return NULL; }

	ch_bytes_t mdat; CUTE_H264_MEMSET(&mdat, 0, sizeof(mdat));
	const uint8_t* sps; const uint8_t* pps;
	int sps_len, pps_len, count = 0;
	if (!ch_mp4_collect(s, size, &mdat, samples, max_samples, &count, &sps, &sps_len, &pps, &pps_len)
	    || !sps || !pps) {
		CUTE_H264_FREE(samples); CUTE_H264_FREE(mdat.data);
		ch_error_reason = "The stream carries no complete picture."; return NULL;
	}

	// Timescale is a thousand ticks per frame so that any whole frame rate is exact rather than
	// rounded, which is what makes long recordings drift out of sync with audio.
	int timescale = fps * 1000, delta = 1000;
	uint32_t duration = (uint32_t)count * (uint32_t)delta;

	ch_bytes_t out; CUTE_H264_MEMSET(&out, 0, sizeof(out));
	int ftyp = ch_box(&out, "ftyp");
	ch_tag(&out, "isom");
	ch_be32(&out, 512);
	ch_tag(&out, "isom"); ch_tag(&out, "iso2"); ch_tag(&out, "avc1"); ch_tag(&out, "mp41");
	ch_box_end(&out, ftyp);

	// Media data first, so the sample offsets are known before the index that points at them.
	int mdat_at = ch_box(&out, "mdat");
	int mdat_base = out.len;
	ch_bytes_reserve(&out, mdat.len);
	if (!out.oom) { CUTE_H264_MEMCPY(out.data + out.len, mdat.data, (size_t)mdat.len); out.len += mdat.len; }
	ch_box_end(&out, mdat_at);
	CUTE_H264_FREE(mdat.data);

	int moov = ch_box(&out, "moov");
	{
		int mvhd = ch_box(&out, "mvhd");
		ch_full_box(&out, 0);
		ch_be32(&out, 0); ch_be32(&out, 0);          // creation, modification time
		ch_be32(&out, (uint32_t)timescale);
		ch_be32(&out, duration);
		ch_be32(&out, 0x00010000);                   // rate 1.0
		ch_be16(&out, 0x0100);                       // volume 1.0
		ch_be16(&out, 0);
		ch_be32(&out, 0); ch_be32(&out, 0);
		// The unity transformation matrix. Players apply it, so zeros here would collapse the
		// picture to nothing.
		ch_be32(&out, 0x00010000); ch_be32(&out, 0); ch_be32(&out, 0);
		ch_be32(&out, 0); ch_be32(&out, 0x00010000); ch_be32(&out, 0);
		ch_be32(&out, 0); ch_be32(&out, 0); ch_be32(&out, 0x40000000);
		for (int i = 0; i < 6; ++i) ch_be32(&out, 0);
		ch_be32(&out, 2);                            // next track id
		ch_box_end(&out, mvhd);
	}
	{
		int trak = ch_box(&out, "trak");
		{
			int tkhd = ch_box(&out, "tkhd");
			ch_full_box(&out, 7);                    // enabled, in movie, in preview
			ch_be32(&out, 0); ch_be32(&out, 0);
			ch_be32(&out, 1);                        // track id
			ch_be32(&out, 0);
			ch_be32(&out, duration);
			ch_be32(&out, 0); ch_be32(&out, 0);
			ch_be16(&out, 0);                        // layer
			ch_be16(&out, 0);                        // alternate group
			ch_be16(&out, 0);                        // volume, zero for video
			ch_be16(&out, 0);
			ch_be32(&out, 0x00010000); ch_be32(&out, 0); ch_be32(&out, 0);
			ch_be32(&out, 0); ch_be32(&out, 0x00010000); ch_be32(&out, 0);
			ch_be32(&out, 0); ch_be32(&out, 0); ch_be32(&out, 0x40000000);
			ch_be32(&out, (uint32_t)w << 16);        // display size, 16.16 fixed point
			ch_be32(&out, (uint32_t)h << 16);
			ch_box_end(&out, tkhd);
		}
		{
			int mdia = ch_box(&out, "mdia");
			{
				int mdhd = ch_box(&out, "mdhd");
				ch_full_box(&out, 0);
				ch_be32(&out, 0); ch_be32(&out, 0);
				ch_be32(&out, (uint32_t)timescale);
				ch_be32(&out, duration);
				ch_be16(&out, 0x55c4);               // language "und"
				ch_be16(&out, 0);
				ch_box_end(&out, mdhd);
			}
			{
				int hdlr = ch_box(&out, "hdlr");
				ch_full_box(&out, 0);
				ch_be32(&out, 0);
				ch_tag(&out, "vide");
				ch_be32(&out, 0); ch_be32(&out, 0); ch_be32(&out, 0);
				const char* name = "cute_h264";
				for (const char* c = name; *c; ++c) ch_bytes_push(&out, (uint8_t)*c);
				ch_bytes_push(&out, 0);
				ch_box_end(&out, hdlr);
			}
			{
				int minf = ch_box(&out, "minf");
				{
					int vmhd = ch_box(&out, "vmhd");
					ch_full_box(&out, 1);
					ch_be16(&out, 0);
					ch_be16(&out, 0); ch_be16(&out, 0); ch_be16(&out, 0);
					ch_box_end(&out, vmhd);
				}
				{
					int dinf = ch_box(&out, "dinf");
					int dref = ch_box(&out, "dref");
					ch_full_box(&out, 0);
					ch_be32(&out, 1);
					int url = ch_box(&out, "url ");
					ch_full_box(&out, 1);            // the data is in this same file
					ch_box_end(&out, url);
					ch_box_end(&out, dref);
					ch_box_end(&out, dinf);
				}
				ch_mp4_stbl(&out, samples, count, mdat_base, w, h, delta, sps, sps_len, pps, pps_len);
				ch_box_end(&out, minf);
			}
			ch_box_end(&out, mdia);
		}
		ch_box_end(&out, trak);
	}
	ch_box_end(&out, moov);
	CUTE_H264_FREE(samples);

	if (out.oom) { CUTE_H264_FREE(out.data); ch_error_reason = "Out of memory."; return NULL; }
	if (out_size) *out_size = out.len;
	return out.data;
}

int ch_encoder_save_mp4(ch_encoder_t* e, const char* file_name)
{
	if (!e) { ch_error_reason = "Null encoder."; return 0; }
	int size = 0;
	const void* mp4 = ch_mp4_wrap(e->out.data, e->out.len, e->w, e->h, e->fps, &size);
	if (!mp4) return 0;
#if !defined(CUTE_H264_NO_STDIO)
	FILE* fp = fopen(file_name, "wb");
	if (!fp) { CUTE_H264_FREE((void*)mp4); ch_error_reason = "Unable to open the output file."; return 0; }
	int ok = fwrite(mp4, 1, (size_t)size, fp) == (size_t)size;
	fclose(fp);
	CUTE_H264_FREE((void*)mp4);
	if (!ok) { ch_error_reason = "Unable to write the output file."; return 0; }
	return 1;
#else
	(void)file_name;
	CUTE_H264_FREE((void*)mp4);
	ch_error_reason = "Compiled without stdio.";
	return 0;
#endif
}

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
