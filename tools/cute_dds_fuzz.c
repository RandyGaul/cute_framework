/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Mutation fuzzer for cute_dds.h, the sibling of cute_jpg_fuzz.c and cute_model_fuzz.c.
// DDS parsing is all offset arithmetic driven by file-controlled counts and dimensions --
// exactly the bug class (32-bit overflow, off-the-end surface walks) the other parsers'
// fuzzing found. This harness takes a seed DDS, applies deterministic random mutations
// (bit flips, byte splices, header-field damage, truncation), and hands the result to
// cd_parse_dds_mem. Every run must end in either a parse or a zeroed struct + error
// string -- never a crash, hang, or out-of-bounds read.
//
// Deterministic by design: the PRNG is seeded from the command line, so any failure
// reproduces exactly. Build with a sanitizer:
//
//     cl /TC /fsanitize=address /Zi /Od /I libraries tools\cute_dds_fuzz.c
//     cute_dds_fuzz.exe [seed.dds] [iterations] [start_seed]
//
// Without a seed file it fuzzes a built-in BC1 cube map with mips, which walks every
// branch of the parser: legacy header, FourCC dispatch, cube caps, and the surface loop.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define CUTE_DDS_IMPLEMENTATION
#include <cute/cute_dds.h>

static uint64_t s_rng_state;
static uint32_t s_rand(void)
{
	uint64_t x = s_rng_state;
	x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
	s_rng_state = x;
	return (uint32_t)((x * 0x2545F4914F6CDD1DULL) >> 32);
}
static uint32_t s_rand_below(uint32_t n) { return n ? s_rand() % n : 0; }

// Builds the built-in seed: an 8x8 BC1 cube map with 2 mips (legacy header + caps2).
static int s_build_seed(unsigned char* out)
{
	unsigned char* p = out;
	#define PUT32(v) do { uint32_t u = (uint32_t)(v); *p++ = (unsigned char)u; *p++ = (unsigned char)(u >> 8); *p++ = (unsigned char)(u >> 16); *p++ = (unsigned char)(u >> 24); } while (0)
	PUT32(0x20534444); // "DDS "
	PUT32(124);
	PUT32(0x1007 | 0x20000);
	PUT32(8); PUT32(8);
	PUT32(0); PUT32(0);
	PUT32(2); // mips
	for (int i = 0; i < 11; ++i) PUT32(0);
	PUT32(32);
	PUT32(0x4); // DDPF_FOURCC
	PUT32(0x31545844); // "DXT1"
	PUT32(0); PUT32(0); PUT32(0); PUT32(0); PUT32(0);
	PUT32(0x1008); // caps: TEXTURE | COMPLEX
	PUT32(0xfe00); // caps2: cube, all faces
	PUT32(0); PUT32(0); PUT32(0);
	// 6 faces x (4 blocks + 1 block) x 8 bytes.
	for (int i = 0; i < 6 * 5 * 8; ++i) *p++ = (unsigned char)i;
	#undef PUT32
	return (int)(p - out);
}

static void s_mutate(unsigned char* data, int* size)
{
	int n = *size;
	if (n <= 8) return;
	switch (s_rand_below(5)) {
	case 0: { // Bit flip.
		int i = (int)s_rand_below((uint32_t)n);
		data[i] ^= (unsigned char)(1u << s_rand_below(8));
		break;
	}
	case 1: { // Byte splice.
		data[s_rand_below((uint32_t)n)] = (unsigned char)s_rand_below(256);
		break;
	}
	case 2: { // Header-field bomb: overwrite a whole u32 in the first 148 bytes with an
	          // interesting value (0, 1, huge, INT_MAX-ish) -- dimensions, mips, masks, caps.
		static const uint32_t bombs[] = { 0, 1, 3, 0x7fffffff, 0x80000000u, 0xffffffffu, 65536, 124, 0x20534444 };
		int field = (int)s_rand_below(148 / 4);
		uint32_t v = bombs[s_rand_below(sizeof(bombs) / sizeof(bombs[0]))];
		if (field * 4 + 4 <= n) memcpy(data + field * 4, &v, 4);
		break;
	}
	case 3: { // Truncate.
		*size = 4 + (int)s_rand_below((uint32_t)n - 4);
		break;
	}
	case 4: { // Duplicate a random chunk over another spot.
		int len = 1 + (int)s_rand_below(32);
		if (len > n / 2) len = n / 2;
		int src = (int)s_rand_below((uint32_t)(n - len));
		int dst = (int)s_rand_below((uint32_t)(n - len));
		memmove(data + dst, data + src, (size_t)len);
		break;
	}
	}
}

int main(int argc, char* argv[])
{
	unsigned char seed[4096];
	int seed_size = 0;
	if (argc > 1) {
		FILE* fp = fopen(argv[1], "rb");
		if (!fp) { printf("unable to open %s\n", argv[1]); return -1; }
		seed_size = (int)fread(seed, 1, sizeof(seed), fp);
		fclose(fp);
	} else {
		seed_size = s_build_seed(seed);
	}

	int iterations = argc > 2 ? atoi(argv[2]) : 200000;
	uint64_t start_seed = argc > 3 ? (uint64_t)atoll(argv[3]) : 0x12345678ull;

	unsigned char* buf = (unsigned char*)malloc(sizeof(seed));
	int parsed = 0, rejected = 0;
	for (int i = 0; i < iterations; ++i) {
		s_rng_state = start_seed + (uint64_t)i * 0x9E3779B97F4A7C15ULL;
		memcpy(buf, seed, (size_t)seed_size);
		int size = seed_size;
		int mutations = 1 + (int)s_rand_below(8);
		for (int m = 0; m < mutations; ++m) s_mutate(buf, &size);

		cd_dds_t dds = cd_parse_dds_mem(buf, size);
		if (dds.slice_count) {
			// Touch every surface byte the parser claims is in bounds -- ASAN's job.
			for (int s = 0; s < dds.slice_count; ++s) {
				const unsigned char* d = (const unsigned char*)dds.slices[s].data;
				volatile unsigned char sink = 0;
				if (dds.slices[s].size) sink ^= d[0] ^ d[dds.slices[s].size - 1];
				(void)sink;
			}
			parsed++;
		} else {
			if (!cd_error_reason) { printf("FAIL: rejected without a reason at iteration %d\n", i); return -1; }
			rejected++;
		}
		cd_free_dds(&dds);
	}
	free(buf);
	printf("done: %d iterations, %d parsed, %d rejected, 0 crashes\n", iterations, parsed, rejected);
	return 0;
}
