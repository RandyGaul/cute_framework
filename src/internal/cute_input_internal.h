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

#ifndef CUTE_INPUT_INTERNAL_H
#define CUTE_INPUT_INTERNAL_H

#include <cute_input.h>
#include <cute_joypad.h>
#include <cute_doubly_list.h>

#include <SDL.h>

namespace cute
{

enum mouse_click_t : int
{
	MOUSE_CLICK_SINGLE,
	MOUSE_CLICK_DOUBLE,
};

struct haptic_t;

struct joypad_t
{
	list_node_t node;
	SDL_GameController* controller = NULL;
	haptic_t* haptic = NULL;
	SDL_JoystickID id = -1;
	int buttons[JOYPAD_BUTTON_COUNT] = { 0 };
	int buttons_prev[JOYPAD_BUTTON_COUNT] = { 0 };
	int axes[JOYPAD_AXIS_COUNT] = { 0 };
};

struct app_t;

void pump_input_msgs(app_t* app);

}

#endif // CUTE_INPUT_INTERNAL_H
