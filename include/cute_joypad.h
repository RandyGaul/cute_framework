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

#include "cute_defines.h"
#include "cute_error.h"

struct cf_app_t;
struct cf_joypad_t;
enum cf_joypad_power_level_t;// : int;
enum cf_joypad_button_t;// : int;
enum cf_joypad_axis_t;// : int;

/**
 * Call this once before calling `joypad_open`.
 */
CUTE_API void CUTE_CALL cf_joypad_system_init();

/**
 * Adds an SDL2 mapping to the joypad system. This means for each valid mapping string added, another
 * kind of joypad is supported.
 * 
 * The function `joypad_system_init` initializes many mappings from the community organized mapping
 * database on GitHub (https://github.com/gabomdq/SDL_GameControllerDB), so you probably don't need
 * to ever call this function.
 */
CUTE_API cf_error_t CUTE_CALL cf_joypad_add_mapping(const char* mapping);

/**
 * Returns the number of joypads currently connected to the system.
 */
CUTE_API int CUTE_CALL cf_joypad_count();

/**
 * Opens a joypad on the system.
 * `index` is a number from 0 to `joypad_count`. The first joypad connected to the system is 0,
 * the second is 1, and so on.
 */
CUTE_API cf_joypad_t* CUTE_CALL cf_joypad_open(int index);

/**
 * Destroys a joypad previously opened by `joypad_open`.
 */
CUTE_API void CUTE_CALL cf_joypad_close(cf_joypad_t* joypad);

/**
 * Tests to see if the joypad is still connected to the system (returns true if it is).
 */
CUTE_API bool CUTE_CALL cf_joypad_is_connected(cf_joypad_t* joypad);

/**
 * Returns the power level of the joypad.
 */
CUTE_API cf_joypad_power_level_t CUTE_CALL cf_joypad_power_level(cf_joypad_t* joypad);

/**
 * Returns the name of the joypad.
 */
CUTE_API const char* CUTE_CALL cf_joypad_name(cf_joypad_t* joypad);

CUTE_API bool CUTE_CALL cf_joypad_button_is_down(cf_joypad_t* joypad, cf_joypad_button_t button);
CUTE_API bool CUTE_CALL cf_joypad_button_is_up(cf_joypad_t* joypad, cf_joypad_button_t button);
CUTE_API bool CUTE_CALL cf_joypad_button_was_pressed(cf_joypad_t* joypad, cf_joypad_button_t button);
CUTE_API bool CUTE_CALL cf_joypad_button_was_released(cf_joypad_t* joypad, cf_joypad_button_t button);
CUTE_API int16_t CUTE_CALL cf_joypad_axis(cf_joypad_t* joypad, cf_joypad_axis_t axis);

enum cf_joypad_power_level_t// : int
{
	CF_JOYPAD_POWER_LEVEL_UNKNOWN,
	CF_JOYPAD_POWER_LEVEL_EMPTY,
	CF_JOYPAD_POWER_LEVEL_LOW,
	CF_JOYPAD_POWER_LEVEL_MEDIUM,
	CF_JOYPAD_POWER_LEVEL_FULL,
	CF_JOYPAD_POWER_LEVEL_WIRED,

	CF_JOYPAD_POWER_LEVEL_COUNT,
};

enum cf_joypad_button_t// : int
{
	CF_JOYPAD_BUTTON_INVALID = -1,
	CF_JOYPAD_BUTTON_A,
	CF_JOYPAD_BUTTON_B,
	CF_JOYPAD_BUTTON_X,
	CF_JOYPAD_BUTTON_Y,
	CF_JOYPAD_BUTTON_BACK,
	CF_JOYPAD_BUTTON_GUIDE,
	CF_JOYPAD_BUTTON_START,
	CF_JOYPAD_BUTTON_LEFTSTICK,
	CF_JOYPAD_BUTTON_RIGHTSTICK,
	CF_JOYPAD_BUTTON_LEFTSHOULDER,
	CF_JOYPAD_BUTTON_RIGHTSHOULDER,
	CF_JOYPAD_BUTTON_DPAD_UP,
	CF_JOYPAD_BUTTON_DPAD_DOWN,
	CF_JOYPAD_BUTTON_DPAD_LEFT,
	CF_JOYPAD_BUTTON_DPAD_RIGHT,

	CF_JOYPAD_BUTTON_COUNT,
};

enum cf_joypad_axis_t// : int
{
	CF_JOYPAD_AXIS_INVALID = -1,
	CF_JOYPAD_AXIS_LEFTX,
	CF_JOYPAD_AXIS_LEFTY,
	CF_JOYPAD_AXIS_RIGHTX,
	CF_JOYPAD_AXIS_RIGHTY,
	CF_JOYPAD_AXIS_TRIGGERLEFT,
	CF_JOYPAD_AXIS_TRIGGERRIGHT,

	CF_JOYPAD_AXIS_COUNT,
};

#ifdef CUTE_CPP

namespace cute
{

}

#endif // CUTE_CPP

#endif // CUTE_JOYPAD_H
