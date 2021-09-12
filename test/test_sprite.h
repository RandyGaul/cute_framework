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

CUTE_TEST_CASE(test_sprite_make, "Load a sprite destroy it.");
int test_sprite_make()
{
	CUTE_TEST_ASSERT(!app_make("sprite test", 0, 0, 0, 0, CUTE_APP_OPTIONS_HIDDEN | CUTE_APP_OPTIONS_DEFAULT_GFX_CONTEXT).is_error());
	app_init_upscaling(UPSCALE_PIXEL_PERFECT_AT_LEAST_2X, 160, 120);

	sprite_t s = sprite_make("test_data/girl.aseprite");
	batch_t* batch = sprite_get_batch();
	batch_flush(batch);
	app_present();

	app_destroy();

	return 0;
}
