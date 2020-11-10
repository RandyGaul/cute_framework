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

#ifndef CUTE_JOYPAD_H
#define CUTE_JOYPAD_H

#include <cute_defines.h>
#include <cute_error.h>

namespace cute
{

struct app_t;
struct joypad_t;
enum joypad_power_level_t : int;
enum joypad_button_t : int;
enum joypad_axis_t : int;

/**
 * Call this once before calling `joypad_open`.
 */
CUTE_API void CUTE_CALL joypad_system_init();

/**
 * Adds an SDL2 mapping to the joypad system. This means for each valid mapping string added, another
 * kind of joypad is supported.
 * 
 * The function `joypad_system_init` initializes many mappings from the community organized mapping
 * database on GitHub (https://github.com/gabomdq/SDL_GameControllerDB), so you probably don't need
 * to ever call this function.
 */
CUTE_API error_t CUTE_CALL joypad_add_mapping(const char* mapping);

/**
 * Returns the number of joypads currently connected to the system.
 */
CUTE_API int CUTE_CALL joypad_count();

/**
 * Opens a joypad on the system.
 * `index` is a number from 0 to `joypad_count`. The first joypad connected to the system is 0,
 * the second is 1, and so on.
 */
CUTE_API joypad_t* CUTE_CALL joypad_open(app_t* app, int index);

/**
 * Destroys a joypad previously opened by `joypad_open`.
 */
CUTE_API void CUTE_CALL joypad_close(joypad_t* joypad);

/**
 * Tests to see if the joypad is still connected to the system (returns true if it is).
 */
CUTE_API bool CUTE_CALL joypad_is_connected(joypad_t* joypad);

/**
 * Returns the power level of the joypad.
 */
CUTE_API joypad_power_level_t CUTE_CALL joypad_power_level(joypad_t* joypad);

/**
 * Returns the name of the joypad.
 */
CUTE_API const char* CUTE_CALL joypad_name(joypad_t* joypad);

CUTE_API bool CUTE_CALL joypad_button_is_down(joypad_t* joypad, joypad_button_t button);
CUTE_API bool CUTE_CALL joypad_button_is_up(joypad_t* joypad, joypad_button_t button);
CUTE_API bool CUTE_CALL joypad_button_was_pressed(joypad_t* joypad, joypad_button_t button);
CUTE_API bool CUTE_CALL joypad_button_was_released(joypad_t* joypad, joypad_button_t button);
CUTE_API int16_t CUTE_CALL joypad_axis(joypad_t* joypad, joypad_axis_t axis);

enum joypad_power_level_t : int
{
	JOYPAD_POWER_LEVEL_UNKNOWN,
	JOYPAD_POWER_LEVEL_EMPTY,
	JOYPAD_POWER_LEVEL_LOW,
	JOYPAD_POWER_LEVEL_MEDIUM,
	JOYPAD_POWER_LEVEL_FULL,
	JOYPAD_POWER_LEVEL_WIRED,

	JOYPAD_POWER_LEVEL_COUNT,
};

enum joypad_button_t : int
{
	JOYPAD_BUTTON_INVALID = -1,
	JOYPAD_BUTTON_A,
	JOYPAD_BUTTON_B,
	JOYPAD_BUTTON_X,
	JOYPAD_BUTTON_Y,
	JOYPAD_BUTTON_BACK,
	JOYPAD_BUTTON_GUIDE,
	JOYPAD_BUTTON_START,
	JOYPAD_BUTTON_LEFTSTICK,
	JOYPAD_BUTTON_RIGHTSTICK,
	JOYPAD_BUTTON_LEFTSHOULDER,
	JOYPAD_BUTTON_RIGHTSHOULDER,
	JOYPAD_BUTTON_DPAD_UP,
	JOYPAD_BUTTON_DPAD_DOWN,
	JOYPAD_BUTTON_DPAD_LEFT,
	JOYPAD_BUTTON_DPAD_RIGHT,

	JOYPAD_BUTTON_COUNT,
};

enum joypad_axis_t : int
{
	JOYPAD_AXIS_INVALID = -1,
	JOYPAD_AXIS_LEFTX,
	JOYPAD_AXIS_LEFTY,
	JOYPAD_AXIS_RIGHTX,
	JOYPAD_AXIS_RIGHTY,
	JOYPAD_AXIS_TRIGGERLEFT,
	JOYPAD_AXIS_TRIGGERRIGHT,

	JOYPAD_AXIS_COUNT,
};

}

#endif // CUTE_JOYPAD_H
