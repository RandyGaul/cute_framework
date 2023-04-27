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

#include <cute.h>
using namespace Cute;

/* Load a sprite destroy it. */
TEST_CASE(test_make_sprite)
{
	REQUIRE(!cf_is_error(cf_make_app("sprite test", 0, 0, 0, 0, APP_OPTIONS_HIDDEN | APP_OPTIONS_DEFAULT_GFX_CONTEXT, NULL)));

	CF_Sprite s = cf_make_sprite("test_data/girl.aseprite");
	cf_app_draw_onto_screen();

	cf_destroy_app();

	return true;
}

TEST_SUITE(test_sprite)
{
	RUN_TEST_CASE(test_make_sprite);
}
