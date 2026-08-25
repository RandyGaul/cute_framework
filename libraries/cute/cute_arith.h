/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

/*
	cute_arith.h - An adaptive binary range coder, for packet compression.

	This is the entropy-coding primitive underneath CF's networking compression. It is a binary
	range coder in the LZMA lineage: everything is coded one bit at a time against an adaptive
	probability, plus an "equiprobable" path for bits that carry no model. On top of that sit a
	few conveniences: adaptive bytes (an 8-node bit tree), and variable-length integers.

	Why binary-only? A binary coder is small, has no division in the hot path, is trivial to make
	carry-correct, and composes: any alphabet or model you need is expressed as a handful of bit
	decisions with their own probabilities. Delta-encoded game snapshots are mostly zero bits, and
	an adaptive bit model drives those toward ~0 cost, which is exactly what we want.

	USAGE

		Encode:
			uint16_t model[256]; cf_arith_probs_clear(model, 256);
			cf_arith_encoder e; cf_arith_encoder_init(&e, out_buffer, out_cap);
			cf_arith_encode_byte(&e, model, some_byte);
			...
			int bytes = cf_arith_encoder_flush(&e);   // -1 if it overflowed out_cap

		Decode (mirror the exact same model updates and call sequence):
			uint16_t model[256]; cf_arith_probs_clear(model, 256);
			cf_arith_decoder d; cf_arith_decoder_init(&d, in_buffer, in_size);
			uint8_t b = cf_arith_decode_byte(&d, model);

	The decoder must replay the identical sequence of encode calls with identically-initialized
	models; the streams are otherwise self-delimiting only insofar as the caller knows how many
	symbols to read (encode a length up front if you need it).

	Build the tests with CUTE_ARITH_TESTS defined, then call cf_arith_run_tests().
*/

#ifndef CUTE_ARITH_H
#define CUTE_ARITH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// A single adaptive probability is a uint16 in [1, 2047], representing P(bit==0) as prob/2048.
#define CF_ARITH_PROB_BITS 11
#define CF_ARITH_PROB_ONE  (1 << CF_ARITH_PROB_BITS) // 2048
#define CF_ARITH_PROB_INIT (CF_ARITH_PROB_ONE >> 1)  // 1024, i.e. 0.5
#define CF_ARITH_ADAPT_SHIFT 5                        // Adaptation speed; larger = slower.
#define CF_ARITH_TOP (1u << 24)

typedef struct cf_arith_encoder
{
	uint64_t low;
	uint32_t range;
	uint8_t cache;
	uint64_t cache_size;
	uint8_t* out;
	int cap;
	int count;    // Number of bytes that *would* be written (may exceed cap).
	int overflow; // Set once a write past cap is attempted.
} cf_arith_encoder;

typedef struct cf_arith_decoder
{
	uint32_t range;
	uint32_t code;
	const uint8_t* in;
	int size;
	int pos;
} cf_arith_decoder;

// Reset a probability array to the neutral 0.5.
static inline void cf_arith_probs_clear(uint16_t* probs, int count)
{
	for (int i = 0; i < count; ++i) probs[i] = CF_ARITH_PROB_INIT;
}

//--------------------------------------------------------------------------------------------------
// Encoder.

static inline void cf_arith_encoder_init(cf_arith_encoder* e, uint8_t* out, int cap)
{
	e->low = 0;
	e->range = 0xFFFFFFFFu;
	e->cache = 0;
	e->cache_size = 1;
	e->out = out;
	e->cap = cap;
	e->count = 0;
	e->overflow = 0;
}

static inline void cf_arith_s_put(cf_arith_encoder* e, uint8_t byte)
{
	if (e->count < e->cap) e->out[e->count] = byte;
	else e->overflow = 1;
	e->count++;
}

static inline void cf_arith_s_shift_low(cf_arith_encoder* e)
{
	if ((uint32_t)(e->low >> 32) != 0 || e->low < 0xFF000000ull) {
		uint8_t temp = e->cache;
		do {
			cf_arith_s_put(e, (uint8_t)(temp + (uint8_t)(e->low >> 32)));
			temp = 0xFF;
		} while (--e->cache_size != 0);
		e->cache = (uint8_t)(e->low >> 24);
	}
	e->cache_size++;
	e->low = (uint32_t)((uint32_t)e->low << 8);
}

// Encode one bit against an adaptive probability, updating the model.
static inline void cf_arith_encode_bit(cf_arith_encoder* e, uint16_t* prob, int bit)
{
	uint32_t bound = (e->range >> CF_ARITH_PROB_BITS) * (*prob);
	if (!bit) {
		e->range = bound;
		*prob = (uint16_t)(*prob + ((CF_ARITH_PROB_ONE - *prob) >> CF_ARITH_ADAPT_SHIFT));
	} else {
		e->low += bound;
		e->range -= bound;
		*prob = (uint16_t)(*prob - (*prob >> CF_ARITH_ADAPT_SHIFT));
	}
	while (e->range < CF_ARITH_TOP) {
		cf_arith_s_shift_low(e);
		e->range <<= 8;
	}
}

// Encode nbits equiprobable bits (no model), MSB first. For incompressible values.
static inline void cf_arith_encode_bits(cf_arith_encoder* e, uint32_t value, int nbits)
{
	for (int i = nbits - 1; i >= 0; --i) {
		e->range >>= 1;
		uint32_t b = (value >> i) & 1;
		e->low += e->range & (0u - b);
		while (e->range < CF_ARITH_TOP) {
			cf_arith_s_shift_low(e);
			e->range <<= 8;
		}
	}
}

// Encode a byte through an 8-node adaptive bit tree. `probs` must hold 256 entries.
static inline void cf_arith_encode_byte(cf_arith_encoder* e, uint16_t* probs, uint8_t byte)
{
	uint32_t m = 1;
	for (int i = 7; i >= 0; --i) {
		int bit = (byte >> i) & 1;
		cf_arith_encode_bit(e, &probs[m], bit);
		m = (m << 1) | (uint32_t)bit;
	}
}

// Encode a variable-length unsigned integer: a unary-ish length prefix of continue-bits followed
// by 7 payload bits per group, each group coded with its own small model set. `probs` needs 128
// entries (one bit-tree of 8 nodes per group is overkill; we keep it simple with a shared byte
// tree per 8-bit chunk). Here we use a compact scheme: encode 7 bits + 1 continue bit per group.
static inline void cf_arith_encode_uint(cf_arith_encoder* e, uint16_t* probs, uint64_t value)
{
	// probs layout: [0]=continue model, [1..256]=byte tree for the 7-bit groups (reuse encode_bit).
	for (;;) {
		uint32_t group = (uint32_t)(value & 0x7F);
		value >>= 7;
		int more = value != 0;
		// 7 payload bits, MSB first, each with its own model index (2..128 range).
		uint32_t m = 1;
		for (int i = 6; i >= 0; --i) {
			int bit = (group >> i) & 1;
			cf_arith_encode_bit(e, &probs[1 + m], bit);
			m = (m << 1) | (uint32_t)bit;
		}
		cf_arith_encode_bit(e, &probs[0], more);
		if (!more) break;
	}
}

// Finish the stream. Returns the number of bytes written, or -1 if the output buffer overflowed.
static inline int cf_arith_encoder_flush(cf_arith_encoder* e)
{
	for (int i = 0; i < 5; ++i) cf_arith_s_shift_low(e);
	return e->overflow ? -1 : e->count;
}

//--------------------------------------------------------------------------------------------------
// Decoder.

static inline uint8_t cf_arith_s_get(cf_arith_decoder* d)
{
	return (uint8_t)(d->pos < d->size ? d->in[d->pos++] : (d->pos++, 0));
}

static inline void cf_arith_decoder_init(cf_arith_decoder* d, const uint8_t* in, int size)
{
	d->in = in;
	d->size = size;
	d->pos = 0;
	d->range = 0xFFFFFFFFu;
	d->code = 0;
	// The encoder always emits a leading 0 byte; consume 5 bytes to prime `code`.
	for (int i = 0; i < 5; ++i) d->code = (d->code << 8) | cf_arith_s_get(d);
}

static inline int cf_arith_decode_bit(cf_arith_decoder* d, uint16_t* prob)
{
	uint32_t bound = (d->range >> CF_ARITH_PROB_BITS) * (*prob);
	int bit;
	if (d->code < bound) {
		d->range = bound;
		*prob = (uint16_t)(*prob + ((CF_ARITH_PROB_ONE - *prob) >> CF_ARITH_ADAPT_SHIFT));
		bit = 0;
	} else {
		d->code -= bound;
		d->range -= bound;
		*prob = (uint16_t)(*prob - (*prob >> CF_ARITH_ADAPT_SHIFT));
		bit = 1;
	}
	while (d->range < CF_ARITH_TOP) {
		d->code = (d->code << 8) | cf_arith_s_get(d);
		d->range <<= 8;
	}
	return bit;
}

static inline uint32_t cf_arith_decode_bits(cf_arith_decoder* d, int nbits)
{
	uint32_t result = 0;
	for (int i = 0; i < nbits; ++i) {
		d->range >>= 1;
		uint32_t t = (d->code - d->range) >> 31; // 1 if code < range, else 0.
		d->code -= d->range & (t - 1);
		result = (result << 1) | (1u - t);
		while (d->range < CF_ARITH_TOP) {
			d->code = (d->code << 8) | cf_arith_s_get(d);
			d->range <<= 8;
		}
	}
	return result;
}

static inline uint8_t cf_arith_decode_byte(cf_arith_decoder* d, uint16_t* probs)
{
	uint32_t m = 1;
	for (int i = 0; i < 8; ++i) {
		int bit = cf_arith_decode_bit(d, &probs[m]);
		m = (m << 1) | (uint32_t)bit;
	}
	return (uint8_t)(m & 0xFF);
}

static inline uint64_t cf_arith_decode_uint(cf_arith_decoder* d, uint16_t* probs)
{
	uint64_t value = 0;
	int shift = 0;
	for (;;) {
		uint32_t m = 1;
		for (int i = 0; i < 7; ++i) {
			int bit = cf_arith_decode_bit(d, &probs[1 + m]);
			m = (m << 1) | (uint32_t)bit;
		}
		uint32_t group = m & 0x7F;
		value |= (uint64_t)group << shift;
		shift += 7;
		int more = cf_arith_decode_bit(d, &probs[0]);
		if (!more) break;
	}
	return value;
}

//--------------------------------------------------------------------------------------------------
// Delta compression.
//
// Compress a snapshot `cur` (n bytes) relative to a baseline `prev` (n bytes, or NULL to delta
// against all-zero). The two buffers must be the same fixed size n, known to both sides. Each byte
// is coded as a "changed" flag against an adaptive model, and only changed bytes carry a value --
// so the long runs of unchanged fields typical of frame-to-frame game state cost almost nothing.
// The decode side reproduces `cur` exactly given the identical `prev`.

// Returns the compressed size in bytes, or -1 if it overflowed out_cap.
static inline int cf_arith_delta_compress(const uint8_t* prev, const uint8_t* cur, int n, uint8_t* out, int out_cap)
{
	uint16_t changed = CF_ARITH_PROB_INIT;
	uint16_t value[256];
	cf_arith_probs_clear(value, 256);
	cf_arith_encoder e;
	cf_arith_encoder_init(&e, out, out_cap);
	for (int i = 0; i < n; ++i) {
		uint8_t d = (uint8_t)(cur[i] ^ (prev ? prev[i] : 0));
		int is_changed = d != 0;
		cf_arith_encode_bit(&e, &changed, is_changed);
		if (is_changed) cf_arith_encode_byte(&e, value, d);
	}
	return cf_arith_encoder_flush(&e);
}

// Reconstructs `cur` (n bytes) from `prev` and the compressed delta. Returns 0 on success.
static inline int cf_arith_delta_decompress(const uint8_t* prev, int n, const uint8_t* in, int in_size, uint8_t* cur)
{
	uint16_t changed = CF_ARITH_PROB_INIT;
	uint16_t value[256];
	cf_arith_probs_clear(value, 256);
	cf_arith_decoder d;
	cf_arith_decoder_init(&d, in, in_size);
	for (int i = 0; i < n; ++i) {
		uint8_t delta = 0;
		if (cf_arith_decode_bit(&d, &changed)) delta = cf_arith_decode_byte(&d, value);
		cur[i] = (uint8_t)(delta ^ (prev ? prev[i] : 0));
	}
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif // CUTE_ARITH_H

//--------------------------------------------------------------------------------------------------
// Tests.

#ifdef CUTE_ARITH_TESTS
#ifndef CUTE_ARITH_TESTS_ONCE
#define CUTE_ARITH_TESTS_ONCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cf_arith_s_test_roundtrip_bytes(const uint8_t* data, int n)
{
	// Order-0 adaptive byte model, both directions.
	uint8_t* out = (uint8_t*)malloc((size_t)n * 2 + 64);
	uint16_t model[256];
	cf_arith_probs_clear(model, 256);
	cf_arith_encoder e;
	cf_arith_encoder_init(&e, out, n * 2 + 64);
	for (int i = 0; i < n; ++i) cf_arith_encode_byte(&e, model, data[i]);
	int size = cf_arith_encoder_flush(&e);
	if (size < 0) { printf("  overflow\n"); free(out); return 0; }

	uint16_t model2[256];
	cf_arith_probs_clear(model2, 256);
	cf_arith_decoder d;
	cf_arith_decoder_init(&d, out, size);
	int ok = 1;
	for (int i = 0; i < n; ++i) {
		uint8_t b = cf_arith_decode_byte(&d, model2);
		if (b != data[i]) { ok = 0; printf("  mismatch at %d: got %d want %d\n", i, b, data[i]); break; }
	}
	free(out);
	return ok;
}

static int cf_arith_run_tests(void)
{
	int fails = 0;
	unsigned int seed = 12345;
	#define RND() (seed = seed * 1103515245u + 12345u, (seed >> 16) & 0xFF)

	// 1. Empty.
	if (!cf_arith_s_test_roundtrip_bytes(NULL, 0)) { printf("FAIL empty\n"); fails++; } else printf("PASS empty\n");

	// 2. Single byte, each value.
	{
		int ok = 1;
		for (int v = 0; v < 256; ++v) { uint8_t b = (uint8_t)v; if (!cf_arith_s_test_roundtrip_bytes(&b, 1)) { ok = 0; break; } }
		if (!ok) { printf("FAIL single-byte\n"); fails++; } else printf("PASS single-byte (all 256)\n");
	}

	// 3. All zeros (should compress hugely) and check ratio.
	{
		int n = 4096;
		uint8_t* z = (uint8_t*)calloc(n, 1);
		if (!cf_arith_s_test_roundtrip_bytes(z, n)) { printf("FAIL zeros\n"); fails++; }
		else {
			uint8_t* out = (uint8_t*)malloc(n * 2 + 64);
			uint16_t model[256]; cf_arith_probs_clear(model, 256);
			cf_arith_encoder e; cf_arith_encoder_init(&e, out, n * 2 + 64);
			for (int i = 0; i < n; ++i) cf_arith_encode_byte(&e, model, 0);
			int size = cf_arith_encoder_flush(&e);
			printf("PASS zeros (%d bytes -> %d, %.1fx)\n", n, size, (double)n / size);
			if (size > n / 10) { printf("  WARN: zeros should compress far better\n"); }
			free(out);
		}
		free(z);
	}

	// 4. Random incompressible data of many sizes (round-trip must still be exact).
	{
		int ok = 1;
		int sizes[] = { 1, 2, 3, 7, 15, 16, 17, 255, 256, 257, 1000, 4096, 65537 };
		for (int s = 0; s < (int)(sizeof(sizes)/sizeof(sizes[0])) && ok; ++s) {
			int n = sizes[s];
			uint8_t* data = (uint8_t*)malloc(n);
			for (int i = 0; i < n; ++i) data[i] = (uint8_t)RND();
			if (!cf_arith_s_test_roundtrip_bytes(data, n)) { printf("FAIL random n=%d\n", n); ok = 0; }
			free(data);
		}
		if (ok) printf("PASS random (13 sizes)\n"); else fails++;
	}

	// 5. Skewed data (mostly zeros with occasional values) -- the delta-snapshot shape.
	{
		int n = 8192;
		uint8_t* data = (uint8_t*)malloc(n);
		for (int i = 0; i < n; ++i) data[i] = (RND() < 8) ? (uint8_t)RND() : 0;
		int rt = cf_arith_s_test_roundtrip_bytes(data, n);
		uint8_t* out = (uint8_t*)malloc(n * 2 + 64);
		uint16_t model[256]; cf_arith_probs_clear(model, 256);
		cf_arith_encoder e; cf_arith_encoder_init(&e, out, n * 2 + 64);
		for (int i = 0; i < n; ++i) cf_arith_encode_byte(&e, model, data[i]);
		int size = cf_arith_encoder_flush(&e);
		if (!rt) { printf("FAIL skewed\n"); fails++; }
		else printf("PASS skewed (%d bytes -> %d, %.1fx)\n", n, size, (double)n / size);
		free(out); free(data);
	}

	// 6. Direct (equiprobable) bits and varints round-trip.
	{
		uint8_t out[4096];
		cf_arith_encoder e; cf_arith_encoder_init(&e, out, sizeof(out));
		uint16_t uprobs[257]; cf_arith_probs_clear(uprobs, 257);
		uint32_t dvals[] = { 0, 1, 2, 255, 256, 1023, 0xFFFFFFFFu, 0x12345 };
		int dbits[]     = { 1, 1, 2, 8,   9,   10,   32,          20 };
		uint64_t uvals[] = { 0, 1, 127, 128, 300, 16384, 1000000, 0xFFFFFFFFFFull };
		for (int i = 0; i < 8; ++i) cf_arith_encode_bits(&e, dvals[i] & ((dbits[i]==32)?0xFFFFFFFFu:((1u<<dbits[i])-1)), dbits[i]);
		for (int i = 0; i < 8; ++i) cf_arith_encode_uint(&e, uprobs, uvals[i]);
		int size = cf_arith_encoder_flush(&e);

		cf_arith_decoder d; cf_arith_decoder_init(&d, out, size);
		uint16_t uprobs2[257]; cf_arith_probs_clear(uprobs2, 257);
		int ok = size >= 0;
		for (int i = 0; i < 8 && ok; ++i) {
			uint32_t want = dvals[i] & ((dbits[i]==32)?0xFFFFFFFFu:((1u<<dbits[i])-1));
			uint32_t got = cf_arith_decode_bits(&d, dbits[i]);
			if (got != want) { printf("  direct mismatch %u != %u (%d bits)\n", got, want, dbits[i]); ok = 0; }
		}
		for (int i = 0; i < 8 && ok; ++i) {
			uint64_t got = cf_arith_decode_uint(&d, uprobs2);
			if (got != uvals[i]) { printf("  uint mismatch %llu != %llu\n", (unsigned long long)got, (unsigned long long)uvals[i]); ok = 0; }
		}
		if (!ok) { printf("FAIL direct/uint\n"); fails++; } else printf("PASS direct bits + varints\n");
	}

	// 7. Delta compression: a snapshot vs a mostly-identical next frame.
	{
		int n = 4096;
		uint8_t* base = (uint8_t*)malloc(n);
		uint8_t* cur = (uint8_t*)malloc(n);
		uint8_t* rt = (uint8_t*)malloc(n);
		uint8_t* out = (uint8_t*)malloc(n * 2 + 64);
		for (int i = 0; i < n; ++i) base[i] = (uint8_t)RND();
		memcpy(cur, base, n);
		// Change ~2% of bytes, like a handful of moving entities between frames.
		for (int k = 0; k < n / 50; ++k) { int idx = RND() | (RND() << 8); idx %= n; cur[idx] = (uint8_t)RND(); }

		int size = cf_arith_delta_compress(base, cur, n, out, n * 2 + 64);
		int ok = size >= 0;
		if (ok) { cf_arith_delta_decompress(base, n, out, size, rt); ok = memcmp(rt, cur, n) == 0; }
		if (!ok) { printf("FAIL delta (roundtrip)\n"); fails++; }
		else printf("PASS delta (%d bytes, ~2%% changed -> %d, %.1fx)\n", n, size, (double)n / size);

		// Identical frame -> near-zero payload.
		int same = cf_arith_delta_compress(base, base, n, out, n * 2 + 64);
		cf_arith_delta_decompress(base, n, out, same, rt);
		if (memcmp(rt, base, n) != 0) { printf("FAIL delta (identical roundtrip)\n"); fails++; }
		else printf("PASS delta identical (%d bytes -> %d, %.0fx)\n", n, same, (double)n / same);

		// NULL baseline (first snapshot) must round-trip too.
		int first = cf_arith_delta_compress(NULL, cur, n, out, n * 2 + 64);
		cf_arith_delta_decompress(NULL, n, out, first, rt);
		if (memcmp(rt, cur, n) != 0) { printf("FAIL delta (null baseline)\n"); fails++; }
		else printf("PASS delta null-baseline (%d bytes -> %d)\n", n, first);

		free(base); free(cur); free(rt); free(out);
	}

	#undef RND
	if (fails) printf("\n%d TEST GROUP(S) FAILED\n", fails);
	else printf("\nALL cute_arith TESTS PASSED\n");
	return fails ? -1 : 0;
}

#endif // CUTE_ARITH_TESTS_ONCE
#endif // CUTE_ARITH_TESTS
