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
#include "cute_result.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct cf_app_t cf_app_t;
typedef struct cf_joypad_t cf_joypad_t;
typedef enum cf_joypad_power_level_t cf_joypad_power_level_t;
typedef enum cf_joypad_button_t cf_joypad_button_t;
typedef enum cf_joypad_axis_t cf_joypad_axis_t;

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
CUTE_API cf_result_t CUTE_CALL cf_joypad_add_mapping(const char* mapping);

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

#define CF_JOYPAD_POWER_LEVEL_DEFS \
	CF_ENUM(JOYPAD_POWER_LEVEL_UNKNOWN, 0) \
	CF_ENUM(JOYPAD_POWER_LEVEL_EMPTY, 1) \
	CF_ENUM(JOYPAD_POWER_LEVEL_LOW, 2) \
	CF_ENUM(JOYPAD_POWER_LEVEL_MEDIUM, 3) \
	CF_ENUM(JOYPAD_POWER_LEVEL_FULL, 4) \
	CF_ENUM(JOYPAD_POWER_LEVEL_WIRED, 5) \
	CF_ENUM(JOYPAD_POWER_LEVEL_COUNT, 6) \

typedef enum cf_joypad_power_level_t
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_JOYPAD_POWER_LEVEL_DEFS
	#undef CF_ENUM
} cf_joypad_power_level_t;

#define CF_JOYPAD_BUTTON_DEFS \
	CF_ENUM(JOYPAD_BUTTON_INVALID, -1) \
	CF_ENUM(JOYPAD_BUTTON_A, 0) \
	CF_ENUM(JOYPAD_BUTTON_B, 1) \
	CF_ENUM(JOYPAD_BUTTON_X, 2) \
	CF_ENUM(JOYPAD_BUTTON_Y, 3) \
	CF_ENUM(JOYPAD_BUTTON_BACK, 4) \
	CF_ENUM(JOYPAD_BUTTON_GUIDE, 5) \
	CF_ENUM(JOYPAD_BUTTON_START, 6) \
	CF_ENUM(JOYPAD_BUTTON_LEFTSTICK, 7) \
	CF_ENUM(JOYPAD_BUTTON_RIGHTSTICK, 8) \
	CF_ENUM(JOYPAD_BUTTON_LEFTSHOULDER, 9) \
	CF_ENUM(JOYPAD_BUTTON_RIGHTSHOULDER, 10) \
	CF_ENUM(JOYPAD_BUTTON_DPAD_UP, 11) \
	CF_ENUM(JOYPAD_BUTTON_DPAD_DOWN, 12) \
	CF_ENUM(JOYPAD_BUTTON_DPAD_LEFT, 13) \
	CF_ENUM(JOYPAD_BUTTON_DPAD_RIGHT, 14) \
	CF_ENUM(JOYPAD_BUTTON_COUNT, 15) \

typedef enum cf_joypad_button_t
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_JOYPAD_BUTTON_DEFS
	#undef CF_ENUM
} cf_joypad_button_t;

#define CF_JOYPAD_AXIS_DEFS \
	CF_ENUM(JOYPAD_AXIS_INVALID, -1) \
	CF_ENUM(JOYPAD_AXIS_LEFTX, 0) \
	CF_ENUM(JOYPAD_AXIS_LEFTY, 1) \
	CF_ENUM(JOYPAD_AXIS_RIGHTX, 2) \
	CF_ENUM(JOYPAD_AXIS_RIGHTY, 3) \
	CF_ENUM(JOYPAD_AXIS_TRIGGERLEFT, 4) \
	CF_ENUM(JOYPAD_AXIS_TRIGGERRIGHT, 5) \
	CF_ENUM(JOYPAD_AXIS_COUNT, 6) \

typedef enum cf_joypad_axis_t
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_JOYPAD_AXIS_DEFS
	#undef CF_ENUM
} cf_joypad_axis_t;

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using app_t = cf_app_t;
using joypad_t = cf_joypad_t;

enum joypad_power_level_t : int
{
	#define CF_ENUM(K, V) K = V,
	CF_JOYPAD_POWER_LEVEL_DEFS
	#undef CF_ENUM
};

enum joypad_button_t : int
{
	#define CF_ENUM(K, V) K = V,
	CF_JOYPAD_BUTTON_DEFS
	#undef CF_ENUM
};

enum joypad_axis_t : int
{
	#define CF_ENUM(K, V) K = V,
	CF_JOYPAD_AXIS_DEFS
	#undef CF_ENUM
};

CUTE_INLINE void joypad_system_init() { cf_joypad_system_init(); }
CUTE_INLINE cf_result_t joypad_add_mapping(const char* mapping) { return cf_joypad_add_mapping(mapping); }
CUTE_INLINE int joypad_count() { return cf_joypad_count(); }
CUTE_INLINE cf_joypad_t* joypad_open(int index) { return cf_joypad_open(index); }
CUTE_INLINE void joypad_close(joypad_t* joypad) { cf_joypad_close(joypad); }
CUTE_INLINE bool joypad_is_connected(joypad_t* joypad) { return cf_joypad_is_connected(joypad); }
CUTE_INLINE joypad_power_level_t joypad_power_level(joypad_t* joypad) { return (joypad_power_level_t)cf_joypad_power_level(joypad); }
CUTE_INLINE const char* joypad_name(joypad_t* joypad) { return cf_joypad_name(joypad); }
CUTE_INLINE bool joypad_button_is_down(joypad_t* joypad, joypad_button_t button) { return cf_joypad_button_is_down(joypad, (cf_joypad_button_t)button); }
CUTE_INLINE bool joypad_button_is_up(joypad_t* joypad, joypad_button_t button) { return cf_joypad_button_is_up(joypad, (cf_joypad_button_t)button); }
CUTE_INLINE bool joypad_button_was_pressed(joypad_t* joypad, joypad_button_t button) { return cf_joypad_button_was_pressed(joypad, (cf_joypad_button_t)button); }
CUTE_INLINE bool joypad_button_was_released(joypad_t* joypad, joypad_button_t button) { return cf_joypad_button_was_released(joypad, (cf_joypad_button_t)button); }
CUTE_INLINE int16_t joypad_axis(joypad_t* joypad, joypad_axis_t axis) { return cf_joypad_axis(joypad, (cf_joypad_axis_t)axis); }

}

#endif // CUTE_CPP

#endif // CUTE_JOYPAD_H
