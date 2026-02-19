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

TEST_CASE(test_sprite_is_valid)
{
	// Default (unloaded) sprite is invalid.
	CF_Sprite def = cf_sprite_defaults();
	REQUIRE(!cf_sprite_is_valid(&def));

	// Need app for loading sprites.
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT | CF_APP_OPTIONS_NO_GFX_BIT, NULL)));

	// Aseprite sprite is valid.
	CF_Sprite s = cf_make_demo_sprite();
	REQUIRE(cf_sprite_is_valid(&s));

	// Easy sprite is valid.
	CF_Pixel pixels[1] = {};
	pixels[0].val = 0xFF0000FF;
	CF_Sprite easy = cf_make_easy_sprite_from_pixels(pixels, 1, 1);
	REQUIRE(cf_sprite_is_valid(&easy));

	cf_destroy_app();
	return true;
}

TEST_SUITE(test_sprite)
{
	RUN_TEST_CASE(test_make_sprite);
	RUN_TEST_CASE(test_easy_sprite_unload);
	RUN_TEST_CASE(test_sprite_is_valid);
}
