/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include "internal/cute_app_internal.h"

#include <cute.h>
using namespace Cute;

#include <internal/cute_girl.h>
#include "slice_keys.h"

/* Load a sprite destroy it. */
TEST_CASE(test_make_sprite)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT | CF_APP_OPTIONS_NO_GFX_BIT, NULL)));
	CF_Sprite s = cf_make_sprite_from_memory("girl.aseprite", girl_data, girl_sz);
	REQUIRE(s.name);
	cf_destroy_app();
	return true;
}

TEST_CASE(test_easy_sprite_unload)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT | CF_APP_OPTIONS_NO_GFX_BIT, NULL)));

	CF_Pixel r, g, b;
	r.val = 0xFF0000FF;
	g.val = 0x00FF00FF;
	b.val = 0x0000FFFF;

	CF_Pixel pixels[3] = { r, g, b };

	CF_Sprite s = cf_make_easy_sprite_from_pixels(pixels, 3, 1);
	REQUIRE(s.easy_sprite_id);
	REQUIRE(app->easy_sprites.count() == 1);

	cf_easy_sprite_unload(&s);
	REQUIRE(app->easy_sprites.count() == 0);

	cf_destroy_app();
	return true;
}

/* Easy sprites store a user-defined 9-slice center patch. */
TEST_CASE(test_easy_sprite_center_patch)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT | CF_APP_OPTIONS_NO_GFX_BIT, NULL)));

	CF_Pixel pixels[16 * 16] = { 0 };
	CF_Sprite s = cf_make_easy_sprite_from_pixels(pixels, 16, 16);
	REQUIRE(s.easy_sprite_id);

	// Default is zero — 9-slice falls back to normal draw.
	CF_Aabb zero = cf_sprite_get_center_patch(&s);
	REQUIRE(zero.min.x == 0 && zero.min.y == 0 && zero.max.x == 0 && zero.max.y == 0);

	CF_Aabb patch = cf_make_aabb(cf_v2(4, 4), cf_v2(12, 12));
	cf_sprite_set_center_patch(&s, patch);
	CF_Aabb got = cf_sprite_get_center_patch(&s);
	REQUIRE(got.min.x == 4 && got.min.y == 4);
	REQUIRE(got.max.x == 12 && got.max.y == 12);

	// Easy sprites are single-frame; update must not clear the patch.
	cf_sprite_update(&s);
	got = cf_sprite_get_center_patch(&s);
	REQUIRE(got.min.x == 4 && got.max.x == 12);

	cf_easy_sprite_unload(&s);
	cf_destroy_app();
	return true;
}

static bool s_aabb_equal(CF_Aabb a, CF_Aabb b)
{
	return a.min.x == b.min.x && a.min.y == b.min.y && a.max.x == b.max.x && a.max.y == b.max.y;
}

/* A slice key holds from its frame until the slice's next key, as in Aseprite. */
TEST_CASE(test_sprite_slice_keys)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT | CF_APP_OPTIONS_NO_GFX_BIT, NULL)));
	CF_Sprite s = cf_make_sprite_from_memory("slice_keys.ase", slice_keys_data, slice_keys_sz);
	REQUIRE(s.name);

	// Aseprite's boxes in CF's space: y up, (0, 0) at the centre of the 2x2 canvas.
	CF_Aabb a0 = cf_make_aabb(cf_v2(-1, 0), cf_v2(0, 1));
	CF_Aabb a2 = cf_make_aabb(cf_v2(0, -1), cf_v2(1, 0));
	CF_Aabb b1 = cf_make_aabb(cf_v2(-1, -1), cf_v2(1, 0));
	CF_Aabb none = { 0 };
	CF_Aabb a_by_frame[] = { a0, a0, a2, a2 };
	CF_Aabb b_by_frame[] = { none, b1, b1, b1 };

	cf_sprite_play(&s, "all");
	for (int i = 0; i < 4; ++i) {
		s.frame_index = i;
		REQUIRE(s_aabb_equal(cf_sprite_get_slice(&s, "a"), a_by_frame[i]));
		REQUIRE(s_aabb_equal(cf_sprite_get_slice(&s, "b"), b_by_frame[i]));
	}

	// Keys count frames from the start of the file, not of the animation.
	cf_sprite_play(&s, "late");
	REQUIRE(s_aabb_equal(cf_sprite_get_slice(&s, "a"), a2));

	cf_destroy_app();
	return true;
}

TEST_SUITE(test_sprite)
{
	RUN_TEST_CASE(test_make_sprite);
	RUN_TEST_CASE(test_easy_sprite_unload);
	RUN_TEST_CASE(test_easy_sprite_center_patch);
	RUN_TEST_CASE(test_sprite_slice_keys);
}
