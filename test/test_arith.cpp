/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute_networking.h>
#include <cute_alloc.h>
#include <string.h>

// Snapshot compression contracts: the worst-case bound is tight (raw-store escape), a
// bound-sized buffer never fails, and an undersized buffer reports the exact required
// capacity as a negative return -- `if (result >= 0)` stays the success check.

static void s_fill_random(uint8_t* p, int n, uint32_t seed)
{
	// xorshift; incompressible enough to force the raw escape.
	uint32_t x = seed ? seed : 1;
	for (int i = 0; i < n; ++i) {
		x ^= x << 13;
		x ^= x >> 17;
		x ^= x << 5;
		p[i] = (uint8_t)x;
	}
}

/* A typical sparse delta compresses far below the bound and round-trips exactly. */
TEST_CASE(test_arith_sparse_delta_roundtrip)
{
	const int n = 4096;
	uint8_t* base = (uint8_t*)cf_alloc(n);
	uint8_t* cur = (uint8_t*)cf_alloc(n);
	uint8_t* out = (uint8_t*)cf_alloc(cf_snapshot_compress_bound(n));
	uint8_t* rt = (uint8_t*)cf_alloc(n);
	s_fill_random(base, n, 7);
	memcpy(cur, base, n);
	for (int i = 0; i < n; i += 97) cur[i] ^= (uint8_t)(i * 31 + 1); // ~1% changed.

	int size = cf_snapshot_compress(base, cur, n, out, cf_snapshot_compress_bound(n));
	REQUIRE(size >= 0);
	REQUIRE(size <= cf_snapshot_compress_bound(n));
	REQUIRE(size < n / 4); // Sparse changes should compress well below raw.
	REQUIRE(cf_snapshot_decompress(base, n, out, size, rt) == 0);
	REQUIRE(!memcmp(rt, cur, n));

	// An identical frame costs nearly nothing: model warm-up plus the coder's flush tail,
	// far below one bit per byte.
	int same = cf_snapshot_compress(base, base, n, out, cf_snapshot_compress_bound(n));
	REQUIRE(same >= 0 && same < n / 64);
	REQUIRE(cf_snapshot_decompress(base, n, out, same, rt) == 0);
	REQUIRE(!memcmp(rt, base, n));

	cf_free(base);
	cf_free(cur);
	cf_free(out);
	cf_free(rt);
	return true;
}

/* Incompressible input takes the raw escape: output stays within the bound (one byte over raw
 * at worst) and still round-trips. This is the tight-worst-case guarantee. */
TEST_CASE(test_arith_incompressible_raw_escape)
{
	const int n = 2048;
	uint8_t* base = (uint8_t*)cf_alloc(n);
	uint8_t* cur = (uint8_t*)cf_alloc(n);
	uint8_t* out = (uint8_t*)cf_alloc(cf_snapshot_compress_bound(n));
	uint8_t* rt = (uint8_t*)cf_alloc(n);
	s_fill_random(base, n, 11);
	s_fill_random(cur, n, 999); // Unrelated noise: every byte differs, no structure.

	int size = cf_snapshot_compress(base, cur, n, out, cf_snapshot_compress_bound(n));
	REQUIRE(size >= 0);
	REQUIRE(size <= cf_snapshot_compress_bound(n));
	REQUIRE(cf_snapshot_decompress(base, n, out, size, rt) == 0);
	REQUIRE(!memcmp(rt, cur, n));

	// NULL baseline (first snapshot) round-trips too.
	int first = cf_snapshot_compress(NULL, cur, n, out, cf_snapshot_compress_bound(n));
	REQUIRE(first >= 0 && first <= cf_snapshot_compress_bound(n));
	REQUIRE(cf_snapshot_decompress(NULL, n, out, first, rt) == 0);
	REQUIRE(!memcmp(rt, cur, n));

	cf_free(base);
	cf_free(cur);
	cf_free(out);
	cf_free(rt);
	return true;
}

/* An undersized buffer reports the exact required capacity, negated; retrying with -result
 * succeeds and matches a full-buffer compression byte for byte. */
TEST_CASE(test_arith_negative_required)
{
	const int n = 1024;
	uint8_t* base = (uint8_t*)cf_alloc(n);
	uint8_t* cur = (uint8_t*)cf_alloc(n);
	uint8_t* full = (uint8_t*)cf_alloc(cf_snapshot_compress_bound(n));
	uint8_t* rt = (uint8_t*)cf_alloc(n);
	s_fill_random(base, n, 3);
	s_fill_random(cur, n, 12345);

	int full_size = cf_snapshot_compress(base, cur, n, full, cf_snapshot_compress_bound(n));
	REQUIRE(full_size >= 0);

	for (int cap = 0; cap < full_size; cap += cap < 8 ? 1 : 61) {
		uint8_t* small = (uint8_t*)cf_alloc(cap > 0 ? cap : 1);
		int r = cf_snapshot_compress(base, cur, n, small, cap);
		REQUIRE(r < 0);
		REQUIRE(-r <= cf_snapshot_compress_bound(n));
		uint8_t* exact = (uint8_t*)cf_alloc(-r);
		int r2 = cf_snapshot_compress(base, cur, n, exact, -r);
		REQUIRE(r2 == -r); // The reported requirement is exact, not padded.
		REQUIRE(cf_snapshot_decompress(base, n, exact, r2, rt) == 0);
		REQUIRE(!memcmp(rt, cur, n));
		cf_free(exact);
		cf_free(small);
	}

	cf_free(base);
	cf_free(cur);
	cf_free(full);
	cf_free(rt);
	return true;
}

/* Degenerate inputs behave: zero-size snapshots, and malformed compressed data is refused. */
TEST_CASE(test_arith_edges)
{
	uint8_t out[8];
	uint8_t byte = 0;
	REQUIRE(cf_snapshot_compress_bound(0) == 1);
	int size = cf_snapshot_compress(NULL, &byte, 0, out, sizeof(out));
	REQUIRE(size >= 0 && size <= cf_snapshot_compress_bound(0));
	REQUIRE(cf_snapshot_decompress(NULL, 0, out, size, &byte) == 0);

	REQUIRE(cf_snapshot_decompress(NULL, 4, out, 0, &byte) == -1); // No flag byte.
	uint8_t raw_truncated[2] = { 1, 0xAB };
	uint8_t dst[4];
	REQUIRE(cf_snapshot_decompress(NULL, 4, raw_truncated, sizeof(raw_truncated), dst) == -1);
	return true;
}

TEST_SUITE(test_arith)
{
	RUN_TEST_CASE(test_arith_sparse_delta_roundtrip);
	RUN_TEST_CASE(test_arith_incompressible_raw_escape);
	RUN_TEST_CASE(test_arith_negative_required);
	RUN_TEST_CASE(test_arith_edges);
}
