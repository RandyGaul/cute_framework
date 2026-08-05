/*
    Cute Framework
    Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// cute_jpg.h against libjpeg ground truth: every vector in jpg_test_vectors.h was encoded
// AND reference-decoded by Pillow/libjpeg (see the generator note in that header), so the
// checks below compare CF's decoder against the canonical decoder within a small tolerance
// (JPEG decoders legitimately differ by IDCT rounding and upsample filters). Also covers
// the CF_Image entry points and hostile input: truncations and bit flips must fail or
// degrade cleanly, never crash. Pure CPU -- no app, no GPU.

#include "test_harness.h"

#include <cute.h>
#include <stdlib.h>

#include "jpg_test_vectors.h"

using namespace Cute;

TEST_CASE(test_jpg_reference_decodes)
{
	int nvec = (int)(sizeof(s_vectors) / sizeof(s_vectors[0]));
	for (int i = 0; i < nvec; ++i) {
		const jpg_vector_t* v = &s_vectors[i];
		CF_Image img;
		CF_Result result = cf_image_load_jpg_from_memory(v->jpg, v->jpg_len, &img);
		REQUIRE(!cf_is_error(result));
		REQUIRE(img.w == v->w && img.h == v->h);
		int max_diff = 0;
		int64_t sum_diff = 0;
		int64_t n = (int64_t)v->w * v->h;
		for (int64_t p = 0; p < n; ++p) {
			int dr = cf_abs((int)img.pix[p].colors.r - (int)v->ref[p * 3 + 0]);
			int dg = cf_abs((int)img.pix[p].colors.g - (int)v->ref[p * 3 + 1]);
			int db = cf_abs((int)img.pix[p].colors.b - (int)v->ref[p * 3 + 2]);
			int d = cf_max(dr, cf_max(dg, db));
			if (d > max_diff) max_diff = d;
			sum_diff += dr + dg + db;
			REQUIRE(img.pix[p].colors.a == 255);
		}
		double mean = (double)sum_diff / (double)(n * 3);
		// The max bounds localized catastrophes (chroma edges differ most between upsample
		// filters); the mean bounds gross decode errors.
		REQUIRE(max_diff <= 40);
		REQUIRE(mean <= 2.0);
		cf_image_free(&img);
	}
	return true;
}

TEST_CASE(test_jpg_wh_probe)
{
	int w = 0, h = 0;
	CF_Result result = cf_image_load_jpg_wh(s_vectors[0].jpg, s_vectors[0].jpg_len, &w, &h);
	REQUIRE(!cf_is_error(result));
	REQUIRE(w == s_vectors[0].w && h == s_vectors[0].h);
	return true;
}

TEST_CASE(test_jpg_hostile_input)
{
	// Non-JPEG data fails with an error, not a crash.
	const char* garbage = "definitely not a jpeg file at all";
	CF_Image img;
	REQUIRE(cf_is_error(cf_image_load_jpg_from_memory(garbage, (int)CF_STRLEN(garbage), &img)));
	REQUIRE(cf_is_error(cf_image_load_jpg_from_memory(NULL, 0, &img)));

	// Every truncation point of one baseline and one progressive vector: clean failure or
	// a partial image, never a crash or hang.
	for (int which = 0; which < 2; ++which) {
		const jpg_vector_t* v = &s_vectors[which == 0 ? 0 : 4];
		for (int len = 0; len < v->jpg_len; ++len) {
			if (!cf_is_error(cf_image_load_jpg_from_memory(v->jpg, len, &img))) {
				cf_image_free(&img);
			}
		}
	}

	// Deterministic single-bit corruption sweep.
	{
		const jpg_vector_t* v = &s_vectors[0];
		unsigned char* buf = (unsigned char*)cf_alloc(v->jpg_len);
		uint32_t rng = 0x12345u;
		for (int iter = 0; iter < 500; ++iter) {
			CF_MEMCPY(buf, v->jpg, (size_t)v->jpg_len);
			rng = rng * 1664525u + 1013904223u;
			int byte = (int)(rng % (uint32_t)v->jpg_len);
			rng = rng * 1664525u + 1013904223u;
			buf[byte] ^= (unsigned char)(1u << (rng % 8));
			if (!cf_is_error(cf_image_load_jpg_from_memory(buf, v->jpg_len, &img))) {
				cf_image_free(&img);
			}
		}
		cf_free(buf);
	}
	return true;
}

TEST_SUITE(test_jpg)
{
	RUN_TEST_CASE(test_jpg_reference_decodes);
	RUN_TEST_CASE(test_jpg_wh_probe);
	RUN_TEST_CASE(test_jpg_hostile_input);
}
