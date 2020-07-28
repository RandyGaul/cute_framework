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

#include <stdio.h>

#include <cute_app.h>
#include <cute_timer.h>
#include <cute_input.h>

int main(int argc, const char** argv)
{
	cute::app_t* app = cute::app_make("Cute Snake", 0, 0, 640, 480, CUTE_APP_OPTIONS_GFX_D3D9 | CUTE_APP_OPTIONS_WINDOW_POS_CENTERED);

	while (cute::app_is_running(app)) {
		float dt = cute::calc_dt();
		cute::app_update(app, dt);

		if (cute::key_was_pressed(app, cute::KEY_SPACE)) {
			printf("space\n");
		}

		if (cute::mouse_was_pressed(app, cute::MOUSE_BUTTON_LEFT)) {
			printf("left click\n");
		}

		if (cute::mouse_double_click_was_pressed(app, cute::MOUSE_BUTTON_LEFT)) {
			printf("left double click\n");
		}
	}

	return 0;
}
