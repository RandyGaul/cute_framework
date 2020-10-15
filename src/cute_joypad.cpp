/*
	Cute Framework
	Copyright (C) 2021 Randy Gaul https://randygaul.net

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

#include <cute_joypad.h>
#include <cute_c_runtime.h>
#include <cute_alloc.h>

#include <internal/cute_input_internal.h>
#include <internal/cute_app_internal.h>
#include <data/cute_joypad_mapping_db.h>

#include <SDL2/SDL.h>

namespace cute
{

// TODO - Lock on joypad list? Race condition with pumping input messages.

void joypad_system_init()
{
	int result = SDL_GameControllerAddMapping((const char*)joypad_mapping_db_data);
	CUTE_ASSERT(result != -1);
}

error_t joypad_add_mapping(const char* mapping)
{
	int result = SDL_GameControllerAddMapping(mapping);
	if (result == -1) return error_failure("Failed to add mapping.");
	else return error_success();
}

int joypad_count()
{
	return SDL_NumJoysticks();
}

joypad_t* joypad_open(app_t* app, int index)
{
	if (SDL_IsGameController(index)) {
		SDL_GameController* controller = SDL_GameControllerOpen(index);
		if (controller) {
			SDL_Joystick* joy = SDL_GameControllerGetJoystick(controller);
			joypad_t* joypad = CUTE_NEW(joypad_t, NULL);
			joypad->controller = controller;
			joypad->id = SDL_JoystickInstanceID(joy);
			list_push_front(&app->joypads, &joypad->node);
			return joypad;
		}
	}
	return NULL;
}

void joypad_close(joypad_t* joypad)
{
	list_remove(&joypad->node);
	SDL_GameControllerClose(joypad->controller);
	CUTE_FREE(joypad, NULL);
}

bool joypad_is_connected(joypad_t* joypad)
{
	return SDL_GameControllerGetAttached(joypad->controller);
}

joypad_power_level_t joypad_power_level(joypad_t* joypad)
{
	SDL_Joystick* joy = SDL_GameControllerGetJoystick(joypad->controller);
	SDL_JoystickPowerLevel level = SDL_JoystickCurrentPowerLevel(joy);
	switch (level) {
	case SDL_JOYSTICK_POWER_UNKNOWN: return JOYPAD_POWER_LEVEL_UNKNOWN;
	case SDL_JOYSTICK_POWER_EMPTY: return JOYPAD_POWER_LEVEL_EMPTY;
	case SDL_JOYSTICK_POWER_LOW: return JOYPAD_POWER_LEVEL_LOW;
	case SDL_JOYSTICK_POWER_MEDIUM: return JOYPAD_POWER_LEVEL_MEDIUM;
	case SDL_JOYSTICK_POWER_FULL: return JOYPAD_POWER_LEVEL_FULL;
	case SDL_JOYSTICK_POWER_WIRED: return JOYPAD_POWER_LEVEL_WIRED;
	case SDL_JOYSTICK_POWER_MAX: return JOYPAD_POWER_LEVEL_COUNT;
	}
	return JOYPAD_POWER_LEVEL_UNKNOWN;
}

const char* joypad_name(joypad_t* joypad)
{
	return SDL_GameControllerName(joypad->controller);
}

bool joypad_button_is_down(joypad_t* joypad, joypad_button_t button)
{
	return joypad->buttons[button];
}

bool joypad_button_is_up(joypad_t* joypad, joypad_button_t button)
{
	return !joypad->buttons[button];
}

bool joypad_button_was_pressed(joypad_t* joypad, joypad_button_t button)
{
	return joypad->buttons[button] && !joypad->buttons[button];
}

bool joypad_button_was_released(joypad_t* joypad, joypad_button_t button)
{
	return !joypad->buttons[button] && joypad->buttons[button];
}

uint16_t joypad_axis(joypad_t* joypad, joypad_axis_t axis)
{
	return joypad->axes[axis];
}

}
