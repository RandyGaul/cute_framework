/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include "test_harness.h"
#include "internal/cute_app_internal.h"

#include <cute.h>
using namespace Cute;

#include <internal/cute_girl.h>

/* Load a sprite destroy it. */
TEST_CASE(test_make_sprite)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, APP_OPTIONS_HIDDEN | APP_OPTIONS_NO_AUDIO | APP_OPTIONS_NO_GFX, NULL)));
	CF_Sprite s = cf_make_sprite_from_memory("girl.aseprite", girl_data, girl_sz);
	REQUIRE(s.name);
	cf_destroy_app();
	return true;
}

TEST_CASE(test_easy_sprite_unload)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, APP_OPTIONS_HIDDEN | APP_OPTIONS_NO_AUDIO | APP_OPTIONS_NO_GFX, NULL)));

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

TEST_SUITE(test_sprite)
{
	RUN_TEST_CASE(test_make_sprite);
	RUN_TEST_CASE(test_easy_sprite_unload);
}
