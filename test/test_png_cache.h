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

#include <internal/cute_png_cache_internal.h>

CUTE_TEST_CASE(test_png_cache, "Test all functions of the png caching API.");
int test_png_cache()
{
	cf_fs_init(NULL);
	cf_fs_mount(cf_fs_get_base_directory(), "", true);

	cf_make_png_cache();

	cf_png_t white;
	cf_png_t black;
	CF_Result err = cf_png_cache_load("test_data/white_pixel.png", &white);
	CUTE_TEST_ASSERT(!cf_is_error(err));
	err = cf_png_cache_load("test_data/black_pixel.png", &black);
	CUTE_TEST_ASSERT(!cf_is_error(err));

	cf_png_t blink_png[] = { white, black };
	float blink_delay[] = { 0.5f, 0.5f };
	
	cf_png_t white_png[] = { white };
	float white_delay[] = { 1.0f };

	cf_png_t black_png[] = { black };
	float black_delay[] = { 1.0f };

	const CF_Animation* blink_anim = cf_make_png_cache_animation("blink", blink_png, CUTE_ARRAY_SIZE(blink_png), blink_delay, CUTE_ARRAY_SIZE(blink_delay));
	const CF_Animation* white_anim = cf_make_png_cache_animation("white", white_png, CUTE_ARRAY_SIZE(white_png), white_delay, CUTE_ARRAY_SIZE(white_delay));
	const CF_Animation* black_anim = cf_make_png_cache_animation("black", black_png, CUTE_ARRAY_SIZE(black_png), black_delay, CUTE_ARRAY_SIZE(black_delay));

	const CF_Animation* anims[] = { blink_anim, white_anim, black_anim };

	const animation_t** table = cf_make_png_cache_animation_table("blink", anims, CUTE_ARRAY_SIZE(anims));
	CF_Sprite sprite = cf_make_png_cache_sprite("blink", table);

	cf_sprite_play(&sprite, "blink");
	CUTE_TEST_CHECK_POINTER(sprite.animations);
	CUTE_TEST_ASSERT(sprite.frame_index == 0);

	cf_sprite_update(&sprite, 0.5f);
	CUTE_TEST_ASSERT(sprite.frame_index == 1);

	cf_destroy_png_cache();

	cf_fs_destroy();

	return 0;
}
