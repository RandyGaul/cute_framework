/*
	------------------------------------------------------------------------------
		Licensing information can be found at the end of the file.
	------------------------------------------------------------------------------

	cute_jpg.h - v1.00

	To create implementation (the function definitions)
		#define CUTE_JPG_IMPLEMENTATION
	in *one* C/CPP file (translation unit) that includes this file


	SUMMARY:

		A from-scratch JPEG decoder in a single header, the sibling of cute_png.h.
		Decodes baseline and progressive huffman-coded JFIF/EXIF files -- which is
		what cameras, Photoshop, Blender, libjpeg, and the web produce -- into a
		plain RGBA8 pixel buffer.

		Supported: baseline sequential (SOF0), extended sequential (SOF1), and
		progressive (SOF2) huffman coding; grayscale, YCbCr with 4:4:4 / 4:2:2 /
		4:4:0 / 4:2:0 (and any other legal sampling factors -- exact 2x ratios get
		a proper triangle reconstruction filter, exotic ones fall back to nearest);
		restart markers; 16-bit quantization tables; Adobe CMYK and YCCK.

		Not supported, and rejected with a clear `cj_error_reason`: arithmetic
		coding (SOF9/10, patent-era rarity), lossless and hierarchical modes,
		12-bit sample precision, and DNL-deferred heights. EXIF orientation tags
		are ignored -- pixels come back in file order.

		Decoding is fully bounds-checked and never trusts a length field: truncated
		entropy data pads with zero bits (matching libjpeg), all other corruption
		fails cleanly with an error string instead of reading out of bounds.

	EXAMPLES:

		Loading a JPEG from disk, then freeing it
			cj_image_t img = cj_load_jpg("images/pic.jpg");
			...
			CUTE_JPG_FREE(img.pix);
			CUTE_JPG_MEMSET(&img, 0, sizeof(img));

		Loading a JPEG from memory, then freeing it
			cj_image_t img = cj_load_jpg_mem(memory, size);
			if (!img.pix) printf("%s\n", cj_error_reason);
			...
			CUTE_JPG_FREE(img.pix);

		Peeking at dimensions without decoding
			int w, h;
			cj_load_jpg_wh(memory, size, &w, &h);

	CUSTOMIZATION

		There are various macros in this header you can customize by defining them before
		including cute_jpg.h. Simply define one to override the default behavior.

			CUTE_JPG_ALLOC
			CUTE_JPG_FREE
			CUTE_JPG_MEMCPY
			CUTE_JPG_MEMSET
			CUTE_JPG_ASSERT
			CUTE_JPG_NO_STDIO

	Revision history:
		1.00 (08/04/2026) initial release: baseline + progressive huffman decode
*/

#if !defined(CUTE_JPG_H)

#include <stdint.h>

typedef struct cj_image_t cj_image_t;
typedef struct cj_pixel_t cj_pixel_t;

// Read this after a load returns a NULL pixel pointer.
extern const char* cj_error_reason;

struct cj_pixel_t
{
	uint8_t r, g, b, a;
};

// The pixel buffer is one CUTE_JPG_ALLOC allocation; free it with CUTE_JPG_FREE
// (plain free by default). Alpha is always 255 -- JPEG carries no alpha.
struct cj_image_t
{
	int w;
	int h;
	cj_pixel_t* pix;
};

#if !defined(CUTE_JPG_NO_STDIO)
cj_image_t cj_load_jpg(const char* file_name);
#endif
cj_image_t cj_load_jpg_mem(const void* jpg_data, int jpg_length);

// Reads just the header (through SOF) to report dimensions; w/h get 0 on failure.
void cj_load_jpg_wh(const void* jpg_data, int jpg_length, int* w, int* h);

#define CUTE_JPG_H
#endif // CUTE_JPG_H

#if defined(CUTE_JPG_IMPLEMENTATION)
#if !defined(CUTE_JPG_IMPLEMENTATION_ONCE)
#define CUTE_JPG_IMPLEMENTATION_ONCE

#if !defined(CUTE_JPG_ALLOC)
	#include <stdlib.h>
	#define CUTE_JPG_ALLOC(size) malloc(size)
	#define CUTE_JPG_FREE(mem) free(mem)
#endif

#if !defined(CUTE_JPG_MEMCPY)
	#include <string.h>
	#define CUTE_JPG_MEMCPY memcpy
#endif

#if !defined(CUTE_JPG_MEMSET)
	#include <string.h>
	#define CUTE_JPG_MEMSET memset
#endif

#if !defined(CUTE_JPG_ASSERT)
	#include <assert.h>
	#define CUTE_JPG_ASSERT assert
#endif

#if !defined(CUTE_JPG_NO_STDIO)
	#include <stdio.h>
#endif

const char* cj_error_reason;

// Every parse helper returns 0 on failure with cj_error_reason set; decode aborts cleanly.
#define CJ_CHECK(X, Y) do { if (!(X)) { cj_error_reason = Y; return 0; } } while (0)

#define CJ_FAST_BITS 9 // Fast-path huffman lookup width; codes this short (the vast majority) decode in one table read.

// Natural order for the 64 zigzag positions. Padded with 63 so a corrupt run length that
// walks k past the end writes harmlessly into the last coefficient instead of out of bounds.
static const uint8_t cj_zigzag[64 + 15] =
{
	 0,  1,  8, 16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63,
	63, 63, 63, 63, 63, 63, 63, 63,
	63, 63, 63, 63, 63, 63, 63
};

typedef struct cj_huff_t
{
	// Canonical code tables built from the DHT counts, plus a CJ_FAST_BITS-wide
	// direct lookup for short codes.
	uint8_t fast[1 << CJ_FAST_BITS]; // 255 = take the slow path.
	uint8_t values[256];
	uint8_t size[257];
	uint32_t maxcode[18];            // Left-aligned to 16 bits; [17] is a sentinel.
	int delta[17];                   // values index offset per code length.
	int defined;
} cj_huff_t;

typedef struct cj_component_t
{
	int id;
	int h, v;       // Sampling factors, 1-4.
	int tq;         // Quant table index.
	int dc_tbl, ac_tbl;
	int x, y;                        // This component's plane dimensions in samples.
	int w_blocks, h_blocks;          // ceil(x/8), ceil(y/8): blocks that carry real samples.
	int w_blocks_pad, h_blocks_pad;  // MCU-aligned: interleaved scans encode the padding blocks too.
	int16_t* coef;                   // w_blocks_pad * h_blocks_pad * 64 coefficients.
	uint8_t* plane;                  // Component-resolution samples after IDCT; stride w_blocks_pad*8.
	uint8_t* full;                   // Upsampled to full image resolution (width*height).
	int dc_pred;
} cj_component_t;

typedef struct cj_decoder_t
{
	const uint8_t* p;
	const uint8_t* end;

	// Entropy-coded bit reader, MSB first. A marker encountered mid-entropy parks in
	// `marker` and the reader feeds zero bits from then on (truncated-stream behavior).
	uint32_t bitbuf;
	int bitcnt;
	int marker;   // 0 = none pending.
	int nomore;

	cj_huff_t huff_dc[4];
	cj_huff_t huff_ac[4];
	uint16_t dequant[4][64]; // In natural (row-major) order, ready for IDCT.
	int dequant_defined[4];

	int width, height;
	int ncomp;
	cj_component_t comp[4];
	int hmax, vmax;
	int mcus_x, mcus_y;
	int mcu_w, mcu_h;
	int progressive;
	int restart_interval;
	int adobe_transform; // -1 = no Adobe APP14 marker.
	int jfif;
	int sof_seen;

	// Current scan (SOS) state.
	int scan_ncomp;
	int scan_comp[4];    // Indices into comp[].
	int spec_start, spec_end, succ_high, succ_low;
	int eobrun;
} cj_decoder_t;

static int cj_at_least(cj_decoder_t* d, int bytes)
{
	return (int)(d->end - d->p) >= bytes;
}

static int cj_u8(cj_decoder_t* d)
{
	return d->p < d->end ? *d->p++ : 0;
}

static int cj_u16(cj_decoder_t* d)
{
	int hi = cj_u8(d);
	return (hi << 8) | cj_u8(d);
}

//--------------------------------------------------------------------------------------------------
// Entropy-coded bit reader.

static void cj_grow_bits(cj_decoder_t* d)
{
	do {
		int b = 0;
		if (!d->nomore) {
			if (d->p >= d->end) {
				// Truncated entropy data: pad with zeros, exactly what libjpeg does. The
				// image comes out gray toward the bottom instead of failing outright.
				d->nomore = 1;
			} else {
				b = *d->p++;
				if (b == 0xFF) {
					int c = d->p < d->end ? *d->p : 0xD9;
					while (c == 0xFF) { d->p++; c = d->p < d->end ? *d->p : 0xD9; } // Fill bytes.
					if (c == 0x00) {
						d->p++; // Stuffed: this really is a 0xFF data byte.
					} else {
						// A real marker terminates the entropy segment. Consume its code
						// byte, park it for the scan loop, and feed zero bits from here on.
						if (d->p < d->end) d->p++;
						d->marker = c;
						d->nomore = 1;
						b = 0;
					}
				}
			}
		}
		d->bitbuf |= (uint32_t)b << (24 - d->bitcnt);
		d->bitcnt += 8;
	} while (d->bitcnt <= 24);
}

static int cj_getbits(cj_decoder_t* d, int n)
{
	if (d->bitcnt < n) cj_grow_bits(d);
	uint32_t v = d->bitbuf >> (32 - n);
	d->bitbuf <<= n;
	d->bitcnt -= n;
	return (int)v;
}

static int cj_getbit(cj_decoder_t* d)
{
	if (d->bitcnt < 1) cj_grow_bits(d);
	uint32_t v = d->bitbuf >> 31;
	d->bitbuf <<= 1;
	d->bitcnt -= 1;
	return (int)v;
}

// Resets the reader to a byte boundary with clean state -- the restart-marker contract.
static void cj_reset_bits(cj_decoder_t* d)
{
	d->bitbuf = 0;
	d->bitcnt = 0;
	d->nomore = 0;
	d->eobrun = 0;
	for (int i = 0; i < d->ncomp; ++i) d->comp[i].dc_pred = 0;
}

//--------------------------------------------------------------------------------------------------
// Huffman tables.

static int cj_build_huffman(cj_huff_t* h, const uint8_t* counts)
{
	int k = 0;
	// Code sizes per value, in order.
	for (int i = 0; i < 16; ++i) {
		for (int j = 0; j < counts[i]; ++j) {
			CJ_CHECK(k < 256, "corrupt JPG: huffman table with more than 256 codes");
			h->size[k++] = (uint8_t)(i + 1);
		}
	}
	h->size[k] = 0;
	int total = k;

	// Canonical codes, and the per-length compare/offset tables for the slow path.
	uint16_t codes[256];
	uint32_t code = 0;
	k = 0;
	for (int len = 1; len <= 16; ++len) {
		h->delta[len] = k - (int)code;
		while (h->size[k] == len) {
			codes[k++] = (uint16_t)code++;
			CJ_CHECK(code - 1 < (1u << len), "corrupt JPG: bad huffman code lengths");
		}
		h->maxcode[len] = code << (16 - len); // Left-aligned so 16-bit prefixes compare directly.
		code <<= 1;
	}
	h->maxcode[17] = 0xFFFFFFFF;

	// Fast table: every short code stamps all its suffix-completions.
	CUTE_JPG_MEMSET(h->fast, 255, sizeof(h->fast));
	for (int i = 0; i < total; ++i) {
		int s = h->size[i];
		if (s <= CJ_FAST_BITS) {
			int c = codes[i] << (CJ_FAST_BITS - s);
			int m = 1 << (CJ_FAST_BITS - s);
			for (int j = 0; j < m; ++j) h->fast[c + j] = (uint8_t)i;
		}
	}
	h->defined = 1;
	return 1;
}

static int cj_huff_decode(cj_decoder_t* d, const cj_huff_t* h)
{
	if (d->bitcnt < 16) cj_grow_bits(d);
	int c = (int)(d->bitbuf >> (32 - CJ_FAST_BITS));
	int k = h->fast[c];
	if (k < 255) {
		int s = h->size[k];
		d->bitbuf <<= s;
		d->bitcnt -= s;
		return h->values[k];
	}

	// Slow path: compare the top 16 bits against each length's max code.
	uint32_t temp = d->bitbuf >> 16;
	int s = CJ_FAST_BITS + 1;
	for (;;) {
		if (temp < h->maxcode[s]) break;
		++s;
		if (s > 16) return -1; // Corrupt stream.
	}
	k = (int)(temp >> (16 - s)) + h->delta[s];
	if (k < 0 || k > 255) return -1;
	d->bitbuf <<= s;
	d->bitcnt -= s;
	return h->values[k];
}

// Receives an s-bit magnitude value and sign-extends it per the JPEG convention
// (a leading 0 bit means negative, offset from the most negative value).
static int cj_extend_receive(cj_decoder_t* d, int s)
{
	if (s == 0) return 0;
	int v = cj_getbits(d, s);
	if (v < (1 << (s - 1))) v += ((-1) << s) + 1;
	return v;
}

//--------------------------------------------------------------------------------------------------
// Block decoding: DC/AC for sequential scans, plus the four progressive scan flavors.

static int cj_decode_block(cj_decoder_t* d, cj_component_t* c, int16_t* coef)
{
	// DC.
	const cj_huff_t* hdc = &d->huff_dc[c->dc_tbl];
	const cj_huff_t* hac = &d->huff_ac[c->ac_tbl];
	CJ_CHECK(hdc->defined && hac->defined, "corrupt JPG: scan references an undefined huffman table");
	int t = cj_huff_decode(d, hdc);
	CJ_CHECK(t >= 0 && t <= 15, "corrupt JPG: bad DC huffman code");
	int diff = cj_extend_receive(d, t);
	c->dc_pred += diff;
	coef[0] = (int16_t)c->dc_pred;

	// AC run-length pairs.
	int k = 1;
	do {
		int rs = cj_huff_decode(d, hac);
		CJ_CHECK(rs >= 0, "corrupt JPG: bad AC huffman code");
		int s = rs & 15;
		int r = rs >> 4;
		if (s == 0) {
			if (r != 15) break; // EOB.
			k += 16;            // ZRL: sixteen zeros.
		} else {
			k += r;
			CJ_CHECK(k < 64 + 15, "corrupt JPG: AC coefficient run past end of block");
			coef[cj_zigzag[k]] = (int16_t)cj_extend_receive(d, s);
			++k;
		}
	} while (k < 64);
	return 1;
}

static int cj_decode_block_prog_dc(cj_decoder_t* d, cj_component_t* c, int16_t* coef)
{
	if (d->succ_high == 0) {
		// First DC scan: ordinary DC decode, scaled by the successive-approximation shift.
		const cj_huff_t* hdc = &d->huff_dc[c->dc_tbl];
		CJ_CHECK(hdc->defined, "corrupt JPG: scan references an undefined huffman table");
		int t = cj_huff_decode(d, hdc);
		CJ_CHECK(t >= 0 && t <= 15, "corrupt JPG: bad DC huffman code");
		int diff = cj_extend_receive(d, t);
		c->dc_pred += diff;
		coef[0] = (int16_t)(c->dc_pred * (1 << d->succ_low));
	} else {
		// Refinement: one bit per block.
		if (cj_getbit(d)) coef[0] = (int16_t)(coef[0] | (1 << d->succ_low));
	}
	return 1;
}

static int cj_decode_block_prog_ac_first(cj_decoder_t* d, cj_component_t* c, int16_t* coef)
{
	if (d->eobrun) {
		--d->eobrun;
		return 1;
	}
	const cj_huff_t* hac = &d->huff_ac[c->ac_tbl];
	CJ_CHECK(hac->defined, "corrupt JPG: scan references an undefined huffman table");
	int k = d->spec_start;
	do {
		int rs = cj_huff_decode(d, hac);
		CJ_CHECK(rs >= 0, "corrupt JPG: bad AC huffman code");
		int s = rs & 15;
		int r = rs >> 4;
		if (s == 0) {
			if (r < 15) {
				// End-of-band run: this block (and eobrun more) have no further nonzero
				// coefficients in this band.
				d->eobrun = (1 << r) - 1;
				if (r) d->eobrun += cj_getbits(d, r);
				break;
			}
			k += 16; // ZRL.
		} else {
			k += r;
			CJ_CHECK(k < 64 + 15, "corrupt JPG: AC coefficient run past end of band");
			coef[cj_zigzag[k]] = (int16_t)(cj_extend_receive(d, s) * (1 << d->succ_low));
			++k;
		}
	} while (k <= d->spec_end);
	return 1;
}

static int cj_decode_block_prog_ac_refine(cj_decoder_t* d, cj_component_t* c, int16_t* coef)
{
	int bit = 1 << d->succ_low;

	if (d->eobrun) {
		// Inside an end-of-band run, already-nonzero coefficients still receive
		// correction bits.
		--d->eobrun;
		for (int k = d->spec_start; k <= d->spec_end; ++k) {
			int16_t* p = &coef[cj_zigzag[k]];
			if (*p != 0) {
				if (cj_getbit(d)) {
					if ((*p & bit) == 0) {
						if (*p > 0) *p = (int16_t)(*p + bit);
						else        *p = (int16_t)(*p - bit);
					}
				}
			}
		}
		return 1;
	}

	const cj_huff_t* hac = &d->huff_ac[c->ac_tbl];
	CJ_CHECK(hac->defined, "corrupt JPG: scan references an undefined huffman table");
	int k = d->spec_start;
	do {
		int rs = cj_huff_decode(d, hac);
		CJ_CHECK(rs >= 0, "corrupt JPG: bad AC huffman code");
		int s = rs & 15;
		int r = rs >> 4;
		int16_t newval = 0;
		if (s == 0) {
			if (r < 15) {
				d->eobrun = (1 << r) - 1;
				if (r) d->eobrun += cj_getbits(d, r);
				// Flush correction bits through the rest of the band, then we're in the
				// run for eobrun more blocks.
				for (; k <= d->spec_end; ++k) {
					int16_t* p = &coef[cj_zigzag[k]];
					if (*p != 0) {
						if (cj_getbit(d)) {
							if ((*p & bit) == 0) {
								if (*p > 0) *p = (int16_t)(*p + bit);
								else        *p = (int16_t)(*p - bit);
							}
						}
					}
				}
				return 1;
			}
			// r == 15: skip sixteen zero-history coefficients (correcting nonzero ones on the way).
		} else {
			CJ_CHECK(s == 1, "corrupt JPG: refinement scan with magnitude > 1");
			newval = cj_getbit(d) ? (int16_t)bit : (int16_t)-bit;
		}

		// Advance past r zero-history coefficients, handing a correction bit to every
		// nonzero coefficient passed over, then place the new +-1 (if any).
		while (k <= d->spec_end) {
			int16_t* p = &coef[cj_zigzag[k]];
			++k;
			if (*p != 0) {
				if (cj_getbit(d)) {
					if ((*p & bit) == 0) {
						if (*p > 0) *p = (int16_t)(*p + bit);
						else        *p = (int16_t)(*p - bit);
					}
				}
			} else {
				if (r == 0) {
					if (s) *p = newval;
					break;
				}
				--r;
			}
		}
	} while (k <= d->spec_end);
	return 1;
}

//--------------------------------------------------------------------------------------------------
// IDCT: the standard Loeffler-Ligtenberg-Moshovitz factorization in 12-bit fixed point,
// the same one every production JPEG decoder uses (jidctint et al). Rows then columns.

#define CJ_F2F(x) ((int)((x) * 4096 + 0.5))
#define CJ_FSH(x) ((x) * 4096)

#define CJ_IDCT_1D(s0, s1, s2, s3, s4, s5, s6, s7) \
	int p2 = s2, p3 = s6; \
	int p1 = (p2 + p3) * CJ_F2F(0.5411961f); \
	int t2 = p1 + p3 * CJ_F2F(-1.847759065f); \
	int t3 = p1 + p2 * CJ_F2F(0.765366865f); \
	p2 = s0; p3 = s4; \
	int t0 = CJ_FSH(p2 + p3); \
	int t1 = CJ_FSH(p2 - p3); \
	int x0 = t0 + t3; \
	int x3 = t0 - t3; \
	int x1 = t1 + t2; \
	int x2 = t1 - t2; \
	t0 = s7; t1 = s5; p2 = s3; int t3b = s1; \
	p3 = t0 + p2; \
	int p4 = t1 + t3b; \
	p1 = t0 + t3b; \
	int p2b = t1 + p2; \
	int p5 = (p3 + p4) * CJ_F2F(1.175875602f); \
	t0 = t0 * CJ_F2F(0.298631336f); \
	t1 = t1 * CJ_F2F(2.053119869f); \
	p2 = p2 * CJ_F2F(3.072711026f); \
	t3b = t3b * CJ_F2F(1.501321110f); \
	p1 = p5 + p1 * CJ_F2F(-0.899976223f); \
	p2b = p5 + p2b * CJ_F2F(-2.562915447f); \
	p3 = p3 * CJ_F2F(-1.961570560f); \
	p4 = p4 * CJ_F2F(-0.390180644f); \
	int y3 = t3b + p1 + p4; \
	int y2 = p2 + p2b + p3; \
	int y1 = t1 + p2b + p4; \
	int y0 = t0 + p1 + p3;

static uint8_t cj_clamp(int x)
{
	if ((unsigned)x > 255) {
		if (x < 0) return 0;
		return 255;
	}
	return (uint8_t)x;
}

static void cj_idct_block(uint8_t* out, int out_stride, const int16_t* coef, const uint16_t* dq)
{
	int tmp[64];
	int* t = tmp;
	const int16_t* c = coef;

	// Columns first (input is row-major, we walk column strides of 8).
	for (int i = 0; i < 8; ++i, ++c, ++t) {
		// A column with no AC energy collapses to a constant -- very common, worth the branch.
		if (c[8] == 0 && c[16] == 0 && c[24] == 0 && c[32] == 0 && c[40] == 0 && c[48] == 0 && c[56] == 0) {
			int dc = (c[0] * dq[i]) * 4;
			t[0] = t[8] = t[16] = t[24] = t[32] = t[40] = t[48] = t[56] = dc;
		} else {
			CJ_IDCT_1D(c[0] * dq[i],      c[8] * dq[i + 8],   c[16] * dq[i + 16], c[24] * dq[i + 24],
			           c[32] * dq[i + 32], c[40] * dq[i + 40], c[48] * dq[i + 48], c[56] * dq[i + 56])
			// Keep 10 bits of fraction into the row pass, with rounding.
			x0 += 512; x1 += 512; x2 += 512; x3 += 512;
			t[0]  = (x0 + y3) >> 10;
			t[56] = (x0 - y3) >> 10;
			t[8]  = (x1 + y2) >> 10;
			t[48] = (x1 - y2) >> 10;
			t[16] = (x2 + y1) >> 10;
			t[40] = (x2 - y1) >> 10;
			t[24] = (x3 + y0) >> 10;
			t[32] = (x3 - y0) >> 10;
		}
	}

	t = tmp;
	for (int i = 0; i < 8; ++i, t += 8, out += out_stride) {
		CJ_IDCT_1D(t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7])
		// The +128 level shift rides the rounding constant: 17 fraction bits remain
		// (10 carried in + 12 from the multiplies - 5... balanced to 17 as in stb/jidctint),
		// plus 128.5 in that fixed point.
		x0 += 65536 + (128 << 17);
		x1 += 65536 + (128 << 17);
		x2 += 65536 + (128 << 17);
		x3 += 65536 + (128 << 17);
		out[0] = cj_clamp((x0 + y3) >> 17);
		out[7] = cj_clamp((x0 - y3) >> 17);
		out[1] = cj_clamp((x1 + y2) >> 17);
		out[6] = cj_clamp((x1 - y2) >> 17);
		out[2] = cj_clamp((x2 + y1) >> 17);
		out[5] = cj_clamp((x2 - y1) >> 17);
		out[3] = cj_clamp((x3 + y0) >> 17);
		out[4] = cj_clamp((x3 - y0) >> 17);
	}
}

//--------------------------------------------------------------------------------------------------
// Marker segment parsing.

static int cj_parse_dqt(cj_decoder_t* d)
{
	int len = cj_u16(d) - 2;
	CJ_CHECK(len >= 0 && cj_at_least(d, len), "corrupt JPG: DQT segment overruns the file");
	const uint8_t* seg_end = d->p + len;
	while (d->p < seg_end) {
		int pq_tq = cj_u8(d);
		int pq = pq_tq >> 4;
		int tq = pq_tq & 15;
		CJ_CHECK(pq <= 1, "corrupt JPG: bad quantization table precision");
		CJ_CHECK(tq <= 3, "corrupt JPG: bad quantization table index");
		CJ_CHECK((int)(seg_end - d->p) >= 64 * (pq + 1), "corrupt JPG: DQT table overruns its segment");
		for (int k = 0; k < 64; ++k) {
			int v = pq ? cj_u16(d) : cj_u8(d);
			// Tables arrive in zigzag order; store in natural order so the IDCT can
			// multiply element-wise.
			d->dequant[tq][cj_zigzag[k]] = (uint16_t)v;
		}
		d->dequant_defined[tq] = 1;
	}
	CJ_CHECK(d->p == seg_end, "corrupt JPG: DQT segment length mismatch");
	return 1;
}

static int cj_parse_dht(cj_decoder_t* d)
{
	int len = cj_u16(d) - 2;
	CJ_CHECK(len >= 0 && cj_at_least(d, len), "corrupt JPG: DHT segment overruns the file");
	const uint8_t* seg_end = d->p + len;
	while (d->p < seg_end) {
		CJ_CHECK((int)(seg_end - d->p) >= 17, "corrupt JPG: DHT table header overruns its segment");
		int tc_th = cj_u8(d);
		int tc = tc_th >> 4;
		int th = tc_th & 15;
		CJ_CHECK(tc <= 1, "corrupt JPG: bad huffman table class");
		CJ_CHECK(th <= 3, "corrupt JPG: bad huffman table index");
		uint8_t counts[16];
		int total = 0;
		for (int i = 0; i < 16; ++i) {
			counts[i] = (uint8_t)cj_u8(d);
			total += counts[i];
		}
		CJ_CHECK(total <= 256, "corrupt JPG: huffman table with more than 256 codes");
		CJ_CHECK((int)(seg_end - d->p) >= total, "corrupt JPG: DHT values overrun their segment");
		cj_huff_t* h = tc ? &d->huff_ac[th] : &d->huff_dc[th];
		if (!cj_build_huffman(h, counts)) return 0;
		for (int i = 0; i < total; ++i) h->values[i] = (uint8_t)cj_u8(d);
	}
	CJ_CHECK(d->p == seg_end, "corrupt JPG: DHT segment length mismatch");
	return 1;
}

static int cj_parse_sof(cj_decoder_t* d, int marker)
{
	CJ_CHECK(!d->sof_seen, "corrupt JPG: multiple SOF markers");
	d->sof_seen = 1;
	d->progressive = marker == 0xC2;
	int len = cj_u16(d) - 2;
	CJ_CHECK(len >= 6 && cj_at_least(d, len), "corrupt JPG: SOF segment overruns the file");
	int precision = cj_u8(d);
	CJ_CHECK(precision == 8, "unsupported JPG: only 8-bit sample precision is supported");
	d->height = cj_u16(d);
	d->width = cj_u16(d);
	CJ_CHECK(d->height > 0, "unsupported JPG: DNL-deferred height (height 0 in SOF)");
	CJ_CHECK(d->width > 0, "corrupt JPG: zero width");
	d->ncomp = cj_u8(d);
	CJ_CHECK(d->ncomp == 1 || d->ncomp == 3 || d->ncomp == 4, "unsupported JPG: component count is not 1, 3, or 4");
	CJ_CHECK(len == 6 + d->ncomp * 3, "corrupt JPG: SOF segment length mismatch");

	d->hmax = 1;
	d->vmax = 1;
	for (int i = 0; i < d->ncomp; ++i) {
		cj_component_t* c = &d->comp[i];
		c->id = cj_u8(d);
		int hv = cj_u8(d);
		c->h = hv >> 4;
		c->v = hv & 15;
		c->tq = cj_u8(d);
		CJ_CHECK(c->h >= 1 && c->h <= 4 && c->v >= 1 && c->v <= 4, "corrupt JPG: bad sampling factors");
		CJ_CHECK(c->tq <= 3, "corrupt JPG: bad quantization table selector");
		if (c->h > d->hmax) d->hmax = c->h;
		if (c->v > d->vmax) d->vmax = c->v;
	}

	d->mcu_w = d->hmax * 8;
	d->mcu_h = d->vmax * 8;
	d->mcus_x = (d->width + d->mcu_w - 1) / d->mcu_w;
	d->mcus_y = (d->height + d->mcu_h - 1) / d->mcu_h;

	for (int i = 0; i < d->ncomp; ++i) {
		cj_component_t* c = &d->comp[i];
		c->x = (d->width * c->h + d->hmax - 1) / d->hmax;
		c->y = (d->height * c->v + d->vmax - 1) / d->vmax;
		c->w_blocks = (c->x + 7) / 8;
		c->h_blocks = (c->y + 7) / 8;
		// Interleaved scans encode whole MCUs, padding blocks included, so the
		// coefficient plane is MCU-aligned.
		c->w_blocks_pad = d->mcus_x * c->h;
		c->h_blocks_pad = d->mcus_y * c->v;
		int64_t coef_bytes = (int64_t)c->w_blocks_pad * c->h_blocks_pad * 64 * (int64_t)sizeof(int16_t);
		int64_t plane_bytes = (int64_t)c->w_blocks_pad * 8 * (int64_t)c->h_blocks_pad * 8;
		CJ_CHECK(coef_bytes > 0 && coef_bytes < (int64_t)1 << 30, "unsupported JPG: image too large");
		CJ_CHECK(plane_bytes > 0 && plane_bytes < (int64_t)1 << 30, "unsupported JPG: image too large");
		c->coef = (int16_t*)CUTE_JPG_ALLOC((size_t)coef_bytes);
		c->plane = (uint8_t*)CUTE_JPG_ALLOC((size_t)plane_bytes);
		CJ_CHECK(c->coef && c->plane, "out of memory");
		CUTE_JPG_MEMSET(c->coef, 0, (size_t)coef_bytes);
	}
	return 1;
}

static int cj_parse_sos(cj_decoder_t* d)
{
	CJ_CHECK(d->sof_seen, "corrupt JPG: SOS before SOF");
	int len = cj_u16(d) - 2;
	CJ_CHECK(len >= 4 && cj_at_least(d, len), "corrupt JPG: SOS segment overruns the file");
	d->scan_ncomp = cj_u8(d);
	CJ_CHECK(d->scan_ncomp >= 1 && d->scan_ncomp <= 4, "corrupt JPG: bad scan component count");
	CJ_CHECK(len == 4 + d->scan_ncomp * 2, "corrupt JPG: SOS segment length mismatch");
	for (int i = 0; i < d->scan_ncomp; ++i) {
		int id = cj_u8(d);
		int tables = cj_u8(d);
		int which = -1;
		for (int j = 0; j < d->ncomp; ++j) {
			if (d->comp[j].id == id) { which = j; break; }
		}
		CJ_CHECK(which >= 0, "corrupt JPG: scan references an unknown component");
		d->comp[which].dc_tbl = tables >> 4;
		d->comp[which].ac_tbl = tables & 15;
		CJ_CHECK(d->comp[which].dc_tbl <= 3 && d->comp[which].ac_tbl <= 3, "corrupt JPG: bad huffman table selector");
		d->scan_comp[i] = which;
	}
	d->spec_start = cj_u8(d);
	d->spec_end = cj_u8(d);
	int ahal = cj_u8(d);
	d->succ_high = ahal >> 4;
	d->succ_low = ahal & 15;

	if (d->progressive) {
		CJ_CHECK(d->spec_start <= 63 && d->spec_end <= 63 && d->spec_start <= d->spec_end,
			"corrupt JPG: bad progressive spectral selection");
		CJ_CHECK(d->spec_start != 0 || d->spec_end == 0, "corrupt JPG: DC scan with a nonzero band end");
		CJ_CHECK(d->spec_start == 0 || d->scan_ncomp == 1, "corrupt JPG: interleaved progressive AC scan");
		CJ_CHECK(d->succ_high <= 13 && d->succ_low <= 13, "corrupt JPG: bad successive approximation");
	} else {
		CJ_CHECK(d->spec_start == 0, "corrupt JPG: bad sequential scan header");
		d->spec_end = 63;
		d->succ_high = 0;
		d->succ_low = 0;
	}
	return 1;
}

//--------------------------------------------------------------------------------------------------
// Entropy scan walking.

static int cj_decode_one(cj_decoder_t* d, cj_component_t* c, int16_t* coef)
{
	if (!d->progressive) return cj_decode_block(d, c, coef);
	if (d->spec_start == 0) return cj_decode_block_prog_dc(d, c, coef);
	if (d->succ_high == 0) return cj_decode_block_prog_ac_first(d, c, coef);
	return cj_decode_block_prog_ac_refine(d, c, coef);
}

// Between restart intervals: byte-align, expect an RSTn marker, and reset entropy state.
static int cj_restart(cj_decoder_t* d)
{
	// The reader may have parked the marker; otherwise it's the next two bytes.
	int m = d->marker;
	d->marker = 0;
	if (m == 0) {
		CJ_CHECK(cj_at_least(d, 2) && d->p[0] == 0xFF, "corrupt JPG: expected a restart marker");
		m = d->p[1];
		d->p += 2;
	}
	CJ_CHECK(m >= 0xD0 && m <= 0xD7, "corrupt JPG: expected a restart marker");
	cj_reset_bits(d);
	return 1;
}

static int cj_decode_scan(cj_decoder_t* d)
{
	cj_reset_bits(d);
	d->marker = 0;
	int todo = d->restart_interval ? d->restart_interval : 0x7FFFFFFF;

	if (d->scan_ncomp == 1) {
		// Non-interleaved: one block per MCU, covering only the blocks that carry real
		// samples (no MCU padding).
		cj_component_t* c = &d->comp[d->scan_comp[0]];
		for (int by = 0; by < c->h_blocks; ++by) {
			for (int bx = 0; bx < c->w_blocks; ++bx) {
				int16_t* coef = c->coef + ((int64_t)by * c->w_blocks_pad + bx) * 64;
				if (!cj_decode_one(d, c, coef)) return 0;
				if (--todo <= 0) {
					if (!(by == c->h_blocks - 1 && bx == c->w_blocks - 1)) {
						if (!cj_restart(d)) return 0;
					}
					todo = d->restart_interval ? d->restart_interval : 0x7FFFFFFF;
				}
			}
		}
	} else {
		// Interleaved: whole MCUs, padding blocks included (they consume entropy data).
		for (int my = 0; my < d->mcus_y; ++my) {
			for (int mx = 0; mx < d->mcus_x; ++mx) {
				for (int i = 0; i < d->scan_ncomp; ++i) {
					cj_component_t* c = &d->comp[d->scan_comp[i]];
					for (int v = 0; v < c->v; ++v) {
						for (int h = 0; h < c->h; ++h) {
							int bx = mx * c->h + h;
							int by = my * c->v + v;
							int16_t* coef = c->coef + ((int64_t)by * c->w_blocks_pad + bx) * 64;
							if (!cj_decode_one(d, c, coef)) return 0;
						}
					}
				}
				if (--todo <= 0) {
					if (!(my == d->mcus_y - 1 && mx == d->mcus_x - 1)) {
						if (!cj_restart(d)) return 0;
					}
					todo = d->restart_interval ? d->restart_interval : 0x7FFFFFFF;
				}
			}
		}
	}
	return 1;
}

//--------------------------------------------------------------------------------------------------
// The marker-driven main loop.

static int cj_next_marker(cj_decoder_t* d, int* out)
{
	if (d->marker) {
		*out = d->marker;
		d->marker = 0;
		return 1;
	}
	// Tolerate stray bytes between segments (some writers pad); markers are 0xFF-aligned.
	while (d->p < d->end && *d->p != 0xFF) d->p++;
	while (d->p < d->end && *d->p == 0xFF) d->p++; // Fill bytes collapse.
	CJ_CHECK(d->p < d->end, "corrupt JPG: file ended before EOI");
	*out = *d->p++;
	return 1;
}

static int cj_parse(cj_decoder_t* d, int header_only)
{
	CJ_CHECK(cj_at_least(d, 2) && d->p[0] == 0xFF && d->p[1] == 0xD8, "not a JPG: missing SOI marker");
	d->p += 2;
	d->adobe_transform = -1;

	for (;;) {
		int m;
		if (!cj_next_marker(d, &m)) return 0;
		switch (m) {
		case 0xC0: case 0xC1: case 0xC2:
			if (!cj_parse_sof(d, m)) return 0;
			if (header_only) return 1;
			break;
		case 0xC4:
			if (!cj_parse_dht(d)) return 0;
			break;
		case 0xDB:
			if (!cj_parse_dqt(d)) return 0;
			break;
		case 0xDD: {
			int len = cj_u16(d);
			CJ_CHECK(len == 4, "corrupt JPG: bad DRI segment length");
			d->restart_interval = cj_u16(d);
			break;
		}
		case 0xDA:
			if (!cj_parse_sos(d)) return 0;
			if (!cj_decode_scan(d)) return 0;
			break;
		case 0xD9:
			// EOI. Some files place it early; decode whatever arrived.
			CJ_CHECK(d->sof_seen, "corrupt JPG: EOI before any image data");
			return 1;
		case 0xC3: case 0xC5: case 0xC6: case 0xC7: case 0xCB: case 0xCD: case 0xCE: case 0xCF:
			CJ_CHECK(0, "unsupported JPG: lossless or hierarchical coding");
			break;
		case 0xC9: case 0xCA:
			CJ_CHECK(0, "unsupported JPG: arithmetic coding");
			break;
		case 0xC8: case 0x01:
			break; // JPG-reserved / TEM: no segment body.
		default: {
			if (m >= 0xD0 && m <= 0xD7) break; // Stray RST outside a scan: skip.
			CJ_CHECK(m >= 0xE0, "corrupt JPG: unexpected marker");
			// APPn / COM: sniff the two we care about, then skip.
			int len = cj_u16(d) - 2;
			CJ_CHECK(len >= 0 && cj_at_least(d, len), "corrupt JPG: marker segment overruns the file");
			if (m == 0xE0 && len >= 5 && d->p[0] == 'J' && d->p[1] == 'F' && d->p[2] == 'I' && d->p[3] == 'F' && d->p[4] == 0) {
				d->jfif = 1;
			}
			if (m == 0xEE && len >= 12 && d->p[0] == 'A' && d->p[1] == 'd' && d->p[2] == 'o' && d->p[3] == 'b' && d->p[4] == 'e') {
				d->adobe_transform = d->p[11];
			}
			d->p += len;
			break;
		}
		}
	}
}

//--------------------------------------------------------------------------------------------------
// Finishing: dequantize + IDCT every real block, upsample to full resolution, color convert.

static int cj_finish_blocks(cj_decoder_t* d)
{
	for (int i = 0; i < d->ncomp; ++i) {
		cj_component_t* c = &d->comp[i];
		CJ_CHECK(d->dequant_defined[c->tq], "corrupt JPG: component references an undefined quantization table");
		const uint16_t* dq = d->dequant[c->tq];
		int stride = c->w_blocks_pad * 8;
		for (int by = 0; by < c->h_blocks; ++by) {
			for (int bx = 0; bx < c->w_blocks; ++bx) {
				const int16_t* coef = c->coef + ((int64_t)by * c->w_blocks_pad + bx) * 64;
				cj_idct_block(c->plane + (int64_t)by * 8 * stride + bx * 8, stride, coef, dq);
			}
		}
	}
	return 1;
}

// Horizontal 2x triangle filter (the classic centered reconstruction): each output pixel is
// 3/4 the nearer sample plus 1/4 the farther one.
static void cj_upsample_h2_row(uint8_t* out, const uint8_t* in, int in_w, int out_w)
{
	for (int x = 0; x < out_w; ++x) {
		int i = x >> 1;
		int j = i + ((x & 1) ? 1 : -1);
		if (j < 0) j = 0;
		if (j >= in_w) j = in_w - 1;
		out[x] = (uint8_t)((3 * in[i] + in[j] + 2) >> 2);
	}
}

static void cj_upsample_nearest_row(uint8_t* out, const uint8_t* in, int in_w, int out_w, int h, int hmax)
{
	for (int x = 0; x < out_w; ++x) {
		int i = x * h / hmax;
		if (i >= in_w) i = in_w - 1;
		out[x] = in[i];
	}
}

static int cj_upsample(cj_decoder_t* d)
{
	int w = d->width;
	int h = d->height;
	for (int i = 0; i < d->ncomp; ++i) {
		cj_component_t* c = &d->comp[i];
		c->full = (uint8_t*)CUTE_JPG_ALLOC((size_t)w * h);
		CJ_CHECK(c->full, "out of memory");
		int stride = c->w_blocks_pad * 8;
		int rh = d->hmax / c->h; // Only meaningful when it divides exactly; checked below.
		int rv = d->vmax / c->v;
		int exact_h = c->h * rh == d->hmax && (rh == 1 || rh == 2);
		int exact_v = c->v * rv == d->vmax && (rv == 1 || rv == 2);

		if (exact_h && exact_v) {
			// The overwhelmingly common cases: 1x/2x per axis, with a triangle filter for
			// the 2x direction(s). Vertical first (into a scratch row), horizontal last.
			uint8_t* vrow = (uint8_t*)CUTE_JPG_ALLOC((size_t)c->x); // Heap: c->x can reach 65535.
			CJ_CHECK(vrow, "out of memory");
			for (int y = 0; y < h; ++y) {
				const uint8_t* src_row;
				if (rv == 1) {
					int sy = y;
					if (sy >= c->y) sy = c->y - 1;
					src_row = c->plane + (int64_t)sy * stride;
				} else {
					// Triangle between the two nearest source rows: 3/4 near + 1/4 far.
					int sy = y >> 1;
					int ty = sy + ((y & 1) ? 1 : -1);
					if (sy >= c->y) sy = c->y - 1;
					if (ty < 0) ty = 0;
					if (ty >= c->y) ty = c->y - 1;
					const uint8_t* a = c->plane + (int64_t)sy * stride;
					const uint8_t* b = c->plane + (int64_t)ty * stride;
					for (int x = 0; x < c->x; ++x) vrow[x] = (uint8_t)((3 * a[x] + b[x] + 2) >> 2);
					src_row = vrow;
				}
				if (rh == 1) {
					CUTE_JPG_MEMCPY(c->full + (int64_t)y * w, src_row, (size_t)(w < c->x ? w : c->x));
					if (w > c->x) CUTE_JPG_MEMSET(c->full + (int64_t)y * w + c->x, src_row[c->x - 1], (size_t)(w - c->x));
				} else {
					cj_upsample_h2_row(c->full + (int64_t)y * w, src_row, c->x, w);
				}
			}
			CUTE_JPG_FREE(vrow);
		} else {
			// Exotic sampling ratios (3x, 4x, 4:1:1...): nearest neighbor.
			for (int y = 0; y < h; ++y) {
				int sy = y * c->v / d->vmax;
				if (sy >= c->y) sy = c->y - 1;
				cj_upsample_nearest_row(c->full + (int64_t)y * w, c->plane + (int64_t)sy * stride, c->x, w, c->h, d->hmax);
			}
		}
	}
	return 1;
}

// Fixed-point BT.601 YCbCr -> RGB, 16 fraction bits with rounding.
#define CJ_YFIX(x) ((int)((x) * 65536.0f + 0.5f))

static void cj_ycbcr_to_rgb(uint8_t* r, uint8_t* g, uint8_t* b, int y, int cb, int cr)
{
	int yy = (y << 16) + 32768;
	cb -= 128;
	cr -= 128;
	*r = cj_clamp((yy + CJ_YFIX(1.40200f) * cr) >> 16);
	*g = cj_clamp((yy - CJ_YFIX(0.34414f) * cb - CJ_YFIX(0.71414f) * cr) >> 16);
	*b = cj_clamp((yy + CJ_YFIX(1.77200f) * cb) >> 16);
}

static int cj_color_convert(cj_decoder_t* d, cj_pixel_t* out)
{
	int64_t n = (int64_t)d->width * d->height;

	if (d->ncomp == 1) {
		const uint8_t* y = d->comp[0].full;
		for (int64_t i = 0; i < n; ++i) {
			out[i].r = out[i].g = out[i].b = y[i];
			out[i].a = 255;
		}
		return 1;
	}

	if (d->ncomp == 3) {
		// JFIF is YCbCr by definition; Adobe transform 0 or literal 'R','G','B' component
		// ids mean the samples are already RGB.
		int rgb = (!d->jfif && d->comp[0].id == 'R' && d->comp[1].id == 'G' && d->comp[2].id == 'B')
			|| d->adobe_transform == 0;
		const uint8_t* c0 = d->comp[0].full;
		const uint8_t* c1 = d->comp[1].full;
		const uint8_t* c2 = d->comp[2].full;
		if (rgb) {
			for (int64_t i = 0; i < n; ++i) {
				out[i].r = c0[i];
				out[i].g = c1[i];
				out[i].b = c2[i];
				out[i].a = 255;
			}
		} else {
			for (int64_t i = 0; i < n; ++i) {
				cj_ycbcr_to_rgb(&out[i].r, &out[i].g, &out[i].b, c0[i], c1[i], c2[i]);
				out[i].a = 255;
			}
		}
		return 1;
	}

	// Four components: Adobe CMYK (stored inverted, transform 0) or YCCK (transform 2).
	const uint8_t* c0 = d->comp[0].full;
	const uint8_t* c1 = d->comp[1].full;
	const uint8_t* c2 = d->comp[2].full;
	const uint8_t* c3 = d->comp[3].full;
	int ycck = d->adobe_transform == 2;
	for (int64_t i = 0; i < n; ++i) {
		int k = c3[i];
		uint8_t r, g, b;
		if (ycck) {
			// YCCK stores the YCbCr transform of the inverted CMY channels; undo the
			// inversion, then composite with K -- the libjpeg convention.
			cj_ycbcr_to_rgb(&r, &g, &b, c0[i], c1[i], c2[i]);
			r = (uint8_t)((255 - r) * k / 255);
			g = (uint8_t)((255 - g) * k / 255);
			b = (uint8_t)((255 - b) * k / 255);
		} else {
			// Adobe stores CMYK inverted, so the channel value IS the additive intensity.
			r = (uint8_t)(c0[i] * k / 255);
			g = (uint8_t)(c1[i] * k / 255);
			b = (uint8_t)(c2[i] * k / 255);
		}
		out[i].r = r;
		out[i].g = g;
		out[i].b = b;
		out[i].a = 255;
	}
	return 1;
}

static void cj_free_decoder(cj_decoder_t* d)
{
	for (int i = 0; i < 4; ++i) {
		if (d->comp[i].coef) CUTE_JPG_FREE(d->comp[i].coef);
		if (d->comp[i].plane) CUTE_JPG_FREE(d->comp[i].plane);
		if (d->comp[i].full) CUTE_JPG_FREE(d->comp[i].full);
		d->comp[i].coef = 0;
		d->comp[i].plane = 0;
		d->comp[i].full = 0;
	}
}

//--------------------------------------------------------------------------------------------------
// Public API.

cj_image_t cj_load_jpg_mem(const void* jpg_data, int jpg_length)
{
	cj_image_t img = { 0, 0, 0 };
	cj_decoder_t d;
	CUTE_JPG_MEMSET(&d, 0, sizeof(d));
	d.p = (const uint8_t*)jpg_data;
	d.end = d.p + (jpg_length > 0 ? jpg_length : 0);
	cj_error_reason = 0;

	int ok = jpg_data && jpg_length > 1 && cj_parse(&d, 0);
	if (ok) ok = cj_finish_blocks(&d);
	if (ok) ok = cj_upsample(&d);
	if (ok) {
		int64_t bytes = (int64_t)d.width * d.height * (int64_t)sizeof(cj_pixel_t);
		cj_pixel_t* pix = (cj_pixel_t*)CUTE_JPG_ALLOC((size_t)bytes);
		if (!pix) {
			cj_error_reason = "out of memory";
			ok = 0;
		} else {
			cj_color_convert(&d, pix);
			img.w = d.width;
			img.h = d.height;
			img.pix = pix;
		}
	}
	if (!ok && !cj_error_reason) cj_error_reason = "not a JPG: missing SOI marker";
	cj_free_decoder(&d);
	return img;
}

void cj_load_jpg_wh(const void* jpg_data, int jpg_length, int* w, int* h)
{
	if (w) *w = 0;
	if (h) *h = 0;
	cj_decoder_t d;
	CUTE_JPG_MEMSET(&d, 0, sizeof(d));
	d.p = (const uint8_t*)jpg_data;
	d.end = d.p + (jpg_length > 0 ? jpg_length : 0);
	cj_error_reason = 0;
	if (jpg_data && jpg_length > 1 && cj_parse(&d, 1)) {
		if (w) *w = d.width;
		if (h) *h = d.height;
	}
	cj_free_decoder(&d);
}

#if !defined(CUTE_JPG_NO_STDIO)
cj_image_t cj_load_jpg(const char* file_name)
{
	cj_image_t img = { 0, 0, 0 };
	FILE* fp = fopen(file_name, "rb");
	if (!fp) {
		cj_error_reason = "unable to open file";
		return img;
	}
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (size <= 0 || size > 0x7FFFFFFF) {
		fclose(fp);
		cj_error_reason = "unable to read file";
		return img;
	}
	void* data = CUTE_JPG_ALLOC((size_t)size);
	if (!data) {
		fclose(fp);
		cj_error_reason = "out of memory";
		return img;
	}
	size_t read = fread(data, 1, (size_t)size, fp);
	fclose(fp);
	if (read == (size_t)size) img = cj_load_jpg_mem(data, (int)size);
	else cj_error_reason = "unable to read file";
	CUTE_JPG_FREE(data);
	return img;
}
#endif // CUTE_JPG_NO_STDIO

#endif // CUTE_JPG_IMPLEMENTATION_ONCE
#endif // CUTE_JPG_IMPLEMENTATION

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
