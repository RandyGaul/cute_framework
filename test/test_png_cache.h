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

#include <cute.h>
using namespace cute;

CUTE_TEST_CASE(test_png_cache, "Test all functions of the png caching API.");
int test_png_cache()
{
	file_system_init(NULL);
	file_system_mount(file_system_get_base_dir(), "");

	png_cache_t* cache = png_cache_make();

	png_t white;
	png_t black;
	error_t err = png_cache_load(cache, "white_pixel.png", &white);
	CUTE_TEST_ASSERT(!err.is_error());
	err = png_cache_load(cache, "black_pixel.png", &black);
	CUTE_TEST_ASSERT(!err.is_error());

	const animation_t* blink_anim = png_cache_make_animation(cache, "blink", { white, black }, { 0.5f, 0.5f });
	const animation_t* white_anim = png_cache_make_animation(cache, "white", { white }, { 1.0f });
	const animation_t* black_anim = png_cache_make_animation(cache, "black", { black }, { 1.0f });
	png_cache_make_animation_table(cache, "blink", { blink_anim, white_anim, black_anim } );
	sprite_t sprite = png_cache_make_sprite(cache, "blink");

	sprite.play("blink");
	CUTE_TEST_CHECK_POINTER(sprite.animations);
	CUTE_TEST_ASSERT(sprite.frame_index == 0);

	sprite.update(0.5f);
	CUTE_TEST_ASSERT(sprite.frame_index == 1);

	png_cache_destroy(cache);

	file_system_destroy();

	return 0;
}

