/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// libcute compiles the atlas cache into cute_draw.cpp, where its symbols carry no CF_API
// export decoration -- invisible to anything linking against the DLL on Windows, even though
// they resolve fine against an ELF shared object. Compile a private copy into this test with
// internal linkage instead. The cache keeps all of its state in the atlas_cache_t the caller
// owns, so a second copy shares nothing with libcute's and cannot interfere with it.
#define ATLAS_CACHE_API static inline
#define ATLAS_CACHE_IMPLEMENTATION
#include <cute/cute_atlas_cache.h>

#include "test_harness.h"

#include <cute.h>

#include <math.h>
#include <string.h>

// The atlas cache pads each image with a border ring (1px by default, wider when pages carry
// mipmaps) so a filter tap at an image's edge cannot reach into the neighboring image. That
// ring is an implementation detail:
// reported uvs must span the image's *content*, and local uvs pushed on an entry are
// fractions of the image. Renderers rely on this to map one texel per sprite pixel without
// inflating their geometry, and 9-slice relies on it to cut patches where it says it does.
//
// Pure CPU -- no app, no GPU, so this runs headless.

#define UV_ATLAS_W 64
#define UV_ATLAS_H 64
#define UV_IMG_W   16
#define UV_IMG_H   8

struct UvHarness
{
	atlas_cache_t cache;
	atlas_cache_entry_t reported[8];
	int reported_count;
	uint64_t next_texture_id;
	// Captured from the most recent generate_texture callback (padded slot or whole page).
	int generated_w, generated_h;
	unsigned char generated_pixels[64 * 64 * 4];
	// texture_assembled_callback log.
	uint64_t assembled[8];
	int assembled_count;
};

static UvHarness* s_uv;

static ATLAS_CACHE_U64 s_uv_generate_texture(void* pixels, int w, int h, void* udata)
{
	CF_UNUSED(udata);
	s_uv->generated_w = w;
	s_uv->generated_h = h;
	int size = w * h * 4;
	if (size <= (int)sizeof(s_uv->generated_pixels)) CF_MEMCPY(s_uv->generated_pixels, pixels, size);
	return s_uv->next_texture_id++;
}

static void s_uv_texture_assembled(ATLAS_CACHE_U64 texture_id, void* udata)
{
	CF_UNUSED(udata);
	if (s_uv->assembled_count < 8) s_uv->assembled[s_uv->assembled_count] = texture_id;
	s_uv->assembled_count++;
}

static void s_uv_destroy_texture(ATLAS_CACHE_U64 texture_id, void* udata)
{
	CF_UNUSED(texture_id); CF_UNUSED(udata);
}

static void s_uv_get_pixels(ATLAS_CACHE_U64 image_id, void* buffer, int bytes, void* udata)
{
	CF_UNUSED(image_id); CF_UNUSED(udata);
	CF_MEMSET(buffer, 0xFF, bytes);
}

static void s_uv_report(atlas_cache_entry_t* entries, int count, int texture_w, int texture_h, void* udata)
{
	CF_UNUSED(texture_w); CF_UNUSED(texture_h); CF_UNUSED(udata);
	for (int i = 0; i < count && s_uv->reported_count < 8; ++i) {
		s_uv->reported[s_uv->reported_count++] = entries[i];
	}
}

static void s_uv_init_border(UvHarness* h, int border)
{
	CF_MEMSET(h, 0, sizeof(*h));
	h->next_texture_id = 1;
	s_uv = h;

	atlas_cache_config_t config;
	atlas_cache_set_default_config(&config);
	config.atlas_use_border_pixels = border;
	config.atlas_width_in_pixels = UV_ATLAS_W;
	config.atlas_height_in_pixels = UV_ATLAS_H;
	config.lonely_buffer_count_till_flush = 0;
	config.ticks_to_decay_texture = 100000;
	config.batch_callback = s_uv_report;
	config.get_pixels_callback = s_uv_get_pixels;
	config.generate_texture_callback = s_uv_generate_texture;
	config.delete_texture_callback = s_uv_destroy_texture;
	config.texture_assembled_callback = s_uv_texture_assembled;
	atlas_cache_init(&h->cache, &config, NULL);
}

static void s_uv_init(UvHarness* h)
{
	s_uv_init_border(h, 1);
}

// Push one entry carrying a local uv sub-rect, flush, and hand back what was reported.
static atlas_cache_entry_t s_uv_submit(UvHarness* h, uint64_t image_id, float u0, float v0, float u1, float v1)
{
	atlas_cache_entry_t e;
	CF_MEMSET(&e, 0, sizeof(e));
	e.image_id = image_id;
	e.w = UV_IMG_W;
	e.h = UV_IMG_H;
	e.minx = u0; e.miny = v0;
	e.maxx = u1; e.maxy = v1;
	h->reported_count = 0;
	atlas_cache_push(&h->cache, e);
	atlas_cache_flush(&h->cache);
	CF_MEMSET(&e, 0, sizeof(e));
	return h->reported_count == 1 ? h->reported[0] : e;
}

static bool s_uv_near(float a, float b) { return fabsf(a - b) < 1e-5f; }

TEST_CASE(test_atlas_uvs_exclude_border_ring)
{
	UvHarness h;
	s_uv_init(&h);

	// Before any defrag the image is "lonely": it owns a texture that is exactly one
	// padded slot, so the content rect is the texture minus one texel per edge.
	float iw = 1.0f / (float)(UV_IMG_W + 2);
	float ih = 1.0f / (float)(UV_IMG_H + 2);
	atlas_cache_entry_t lonely = s_uv_submit(&h, 100, 0, 0, 1, 1);
	REQUIRE(s_uv_near(lonely.minx, iw));
	REQUIRE(s_uv_near(lonely.maxx, 1.0f - iw));
	// v is reported flipped, so miny holds the larger coordinate.
	REQUIRE(s_uv_near(lonely.miny, 1.0f - ih));
	REQUIRE(s_uv_near(lonely.maxy, ih));
	// Dimensions describe the image, not its padded slot.
	REQUIRE(lonely.w == UV_IMG_W);
	REQUIRE(lonely.h == UV_IMG_H);

	// Once packed into a page, the rect must still be exactly the image: a whole number of
	// texels wide and tall, landing on texel edges, with the ring between it and the page.
	atlas_cache_defrag(&h.cache);
	atlas_cache_entry_t atlased = s_uv_submit(&h, 100, 0, 0, 1, 1);
	float x0 = atlased.minx * UV_ATLAS_W;
	float y0 = atlased.maxy * UV_ATLAS_H;
	REQUIRE(s_uv_near((atlased.maxx - atlased.minx) * UV_ATLAS_W, (float)UV_IMG_W));
	REQUIRE(s_uv_near((atlased.miny - atlased.maxy) * UV_ATLAS_H, (float)UV_IMG_H));
	REQUIRE(s_uv_near(x0, floorf(x0)));
	REQUIRE(s_uv_near(y0, floorf(y0)));
	REQUIRE(x0 >= 1.0f); // Ring sits between the content and the page edge.
	REQUIRE(y0 >= 1.0f);
	REQUIRE(atlased.w == UV_IMG_W);
	REQUIRE(atlased.h == UV_IMG_H);

	// atlas_cache_fetch reports the same rect as a flush does.
	atlas_cache_entry_t fetched = atlas_cache_fetch(&h.cache, 100, UV_IMG_W, UV_IMG_H);
	REQUIRE(s_uv_near(fetched.minx, atlased.minx));
	REQUIRE(s_uv_near(fetched.maxx, atlased.maxx));
	REQUIRE(s_uv_near(fetched.miny, atlased.miny));
	REQUIRE(s_uv_near(fetched.maxy, atlased.maxy));

	atlas_cache_term(&h.cache);
	s_uv = NULL;
	return true;
}

TEST_CASE(test_atlas_uvs_partial_rect_matches_residency)
{
	UvHarness h;
	s_uv_init(&h);

	// The 9-slice case: a local sub-rect must select that same fraction of the image, and
	// must not shift or flip when the image moves from its own texture into an atlas page.
	atlas_cache_entry_t lonely_full = s_uv_submit(&h, 100, 0, 0, 1, 1);
	atlas_cache_entry_t lonely_half = s_uv_submit(&h, 100, 0, 0, 0.5f, 0.5f);

	atlas_cache_defrag(&h.cache);
	atlas_cache_entry_t atlas_full = s_uv_submit(&h, 100, 0, 0, 1, 1);
	atlas_cache_entry_t atlas_half = s_uv_submit(&h, 100, 0, 0, 0.5f, 0.5f);

	// Both halves start at the image's origin.
	REQUIRE(s_uv_near(lonely_half.minx, lonely_full.minx));
	REQUIRE(s_uv_near(lonely_half.miny, lonely_full.miny));
	REQUIRE(s_uv_near(atlas_half.minx, atlas_full.minx));
	REQUIRE(s_uv_near(atlas_half.miny, atlas_full.miny));

	// ...and cover half of it, in the same direction, in both residencies.
	REQUIRE(s_uv_near((lonely_half.maxx - lonely_full.minx) / (lonely_full.maxx - lonely_full.minx), 0.5f));
	REQUIRE(s_uv_near((lonely_half.maxy - lonely_full.miny) / (lonely_full.maxy - lonely_full.miny), 0.5f));
	REQUIRE(s_uv_near((atlas_half.maxx - atlas_full.minx) / (atlas_full.maxx - atlas_full.minx), 0.5f));
	REQUIRE(s_uv_near((atlas_half.maxy - atlas_full.miny) / (atlas_full.maxy - atlas_full.miny), 0.5f));

	atlas_cache_term(&h.cache);
	s_uv = NULL;
	return true;
}

// A ring wider than one pixel is what mipmapped pages need (see cf_draw3d_mips): a mip-level-k
// texel spans 2^k base pixels, so adjacent images must sit 2*ring >= 2^k apart or coarse levels
// blend them together. The uv contract must hold at any width.
TEST_CASE(test_atlas_uvs_wide_border_ring)
{
	const int B = 4; // cf_draw3d_mips(4)'s ring.
	UvHarness h;
	s_uv_init_border(&h, B);

	// Lonely: one padded slot, content inset by B texels per edge.
	float bu = (float)B / (float)(UV_IMG_W + B * 2);
	float bv = (float)B / (float)(UV_IMG_H + B * 2);
	atlas_cache_entry_t lonely = s_uv_submit(&h, 100, 0, 0, 1, 1);
	REQUIRE(s_uv_near(lonely.minx, bu));
	REQUIRE(s_uv_near(lonely.maxx, 1.0f - bu));
	REQUIRE(s_uv_near(lonely.miny, 1.0f - bv));
	REQUIRE(s_uv_near(lonely.maxy, bv));
	REQUIRE(lonely.w == UV_IMG_W);
	REQUIRE(lonely.h == UV_IMG_H);

	// The lonely texture's pixels: content offset by (B, B) into the padded slot, ring zeroed.
	// This exercises the in-place expansion in atlas_cache_internal_get_pixels at width B.
	REQUIRE(s_uv->generated_w == UV_IMG_W + B * 2);
	REQUIRE(s_uv->generated_h == UV_IMG_H + B * 2);
	for (int y = 0; y < s_uv->generated_h; ++y) {
		for (int x = 0; x < s_uv->generated_w; ++x) {
			bool in_content = x >= B && x < B + UV_IMG_W && y >= B && y < B + UV_IMG_H;
			unsigned char expected = in_content ? 0xFF : 0x00;
			for (int c = 0; c < 4; ++c) {
				REQUIRE(s_uv->generated_pixels[(y * s_uv->generated_w + x) * 4 + c] == expected);
			}
		}
	}

	// Packed into a page: content rect still exactly the image, on texel edges, ring-first.
	atlas_cache_defrag(&h.cache);
	atlas_cache_entry_t a = s_uv_submit(&h, 100, 0, 0, 1, 1);
	float x0 = a.minx * UV_ATLAS_W;
	float y0 = a.maxy * UV_ATLAS_H;
	REQUIRE(s_uv_near((a.maxx - a.minx) * UV_ATLAS_W, (float)UV_IMG_W));
	REQUIRE(s_uv_near((a.miny - a.maxy) * UV_ATLAS_H, (float)UV_IMG_H));
	REQUIRE(s_uv_near(x0, floorf(x0)));
	REQUIRE(s_uv_near(y0, floorf(y0)));
	REQUIRE(x0 >= (float)B);
	REQUIRE(y0 >= (float)B);

	// Two images on one page keep 2*B pixels of ring between their content rects, the
	// spacing that keeps every allotted mip level from blending them together. Fresh cache
	// so both are lonely when defrag packs a page (a defrag above already packed image 100).
	atlas_cache_term(&h.cache);
	s_uv_init_border(&h, B);
	s_uv_submit(&h, 100, 0, 0, 1, 1);
	s_uv_submit(&h, 200, 0, 0, 1, 1);
	atlas_cache_defrag(&h.cache);
	atlas_cache_entry_t e0 = s_uv_submit(&h, 100, 0, 0, 1, 1);
	atlas_cache_entry_t e1 = s_uv_submit(&h, 200, 0, 0, 1, 1);
	REQUIRE(e0.texture_id == e1.texture_id);
	float ax0 = e0.minx * UV_ATLAS_W, ax1 = e0.maxx * UV_ATLAS_W;
	float bx0 = e1.minx * UV_ATLAS_W, bx1 = e1.maxx * UV_ATLAS_W;
	float ay0 = e0.maxy * UV_ATLAS_H, ay1 = e0.miny * UV_ATLAS_H;
	float by0 = e1.maxy * UV_ATLAS_H, by1 = e1.miny * UV_ATLAS_H;
	bool separated_x = bx0 >= ax1 + 2.0f * B - 1e-3f || ax0 >= bx1 + 2.0f * B - 1e-3f;
	bool separated_y = by0 >= ay1 + 2.0f * B - 1e-3f || ay0 >= by1 + 2.0f * B - 1e-3f;
	REQUIRE(separated_x || separated_y);

	atlas_cache_term(&h.cache);
	s_uv = NULL;
	return true;
}

// texture_assembled_callback: exactly once per produced texture, after its contents exist --
// the hook cute_draw.cpp uses to generate atlas mipmaps.
TEST_CASE(test_atlas_uvs_texture_assembled_callback)
{
	UvHarness h;
	s_uv_init(&h);

	// First flush generates one lonely texture -> one assembled notification for it.
	atlas_cache_entry_t lonely = s_uv_submit(&h, 100, 0, 0, 1, 1);
	REQUIRE(h.assembled_count == 1);
	REQUIRE(h.assembled[0] == lonely.texture_id);

	// Re-drawing an already-resident image assembles nothing new.
	s_uv_submit(&h, 100, 0, 0, 1, 1);
	REQUIRE(h.assembled_count == 1);

	// Defrag packs the image into a page (a new texture): one more notification, for the page.
	atlas_cache_defrag(&h.cache);
	atlas_cache_entry_t atlased = s_uv_submit(&h, 100, 0, 0, 1, 1);
	REQUIRE(h.assembled_count == 2);
	REQUIRE(h.assembled[1] == atlased.texture_id);

	atlas_cache_term(&h.cache);
	s_uv = NULL;
	return true;
}

TEST_SUITE(test_atlas_uvs)
{
	RUN_TEST_CASE(test_atlas_uvs_exclude_border_ring);
	RUN_TEST_CASE(test_atlas_uvs_partial_rect_matches_residency);
	RUN_TEST_CASE(test_atlas_uvs_wide_border_ring);
	RUN_TEST_CASE(test_atlas_uvs_texture_assembled_callback);
}
