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

typedef struct CF_App CF_App;
typedef struct CF_Joypad CF_Joypad;

#define CF_JOYPAD_POWER_LEVEL_DEFS \
	CF_ENUM(JOYPAD_POWER_LEVEL_UNKNOWN, 0) \
	CF_ENUM(JOYPAD_POWER_LEVEL_EMPTY, 1) \
	CF_ENUM(JOYPAD_POWER_LEVEL_LOW, 2) \
	CF_ENUM(JOYPAD_POWER_LEVEL_MEDIUM, 3) \
	CF_ENUM(JOYPAD_POWER_LEVEL_FULL, 4) \
	CF_ENUM(JOYPAD_POWER_LEVEL_WIRED, 5) \
	CF_ENUM(JOYPAD_POWER_LEVEL_COUNT, 6) \

typedef enum CF_JoypadPowerLevel
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_JOYPAD_POWER_LEVEL_DEFS
	#undef CF_ENUM
} CF_JoypadPowerLevel;

CUTE_INLINE const char* cf_joypad_power_level_to_string(CF_JoypadPowerLevel level)
{
	switch (level) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_JOYPAD_POWER_LEVEL_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

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

typedef enum CF_JoypadButton
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_JOYPAD_BUTTON_DEFS
	#undef CF_ENUM
} CF_JoypadButton;

CUTE_INLINE const char* cf_joypad_button_to_string(CF_JoypadButton button)
{
	switch (button) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_JOYPAD_BUTTON_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

#define CF_JOYPAD_AXIS_DEFS \
	CF_ENUM(JOYPAD_AXIS_INVALID, -1) \
	CF_ENUM(JOYPAD_AXIS_LEFTX, 0) \
	CF_ENUM(JOYPAD_AXIS_LEFTY, 1) \
	CF_ENUM(JOYPAD_AXIS_RIGHTX, 2) \
	CF_ENUM(JOYPAD_AXIS_RIGHTY, 3) \
	CF_ENUM(JOYPAD_AXIS_TRIGGERLEFT, 4) \
	CF_ENUM(JOYPAD_AXIS_TRIGGERRIGHT, 5) \
	CF_ENUM(JOYPAD_AXIS_COUNT, 6) \

typedef enum CF_JoypadAxis
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_JOYPAD_AXIS_DEFS
	#undef CF_ENUM
} CF_JoypadAxis;

CUTE_INLINE const char* cf_joypad_axis_to_string(CF_JoypadAxis axis)
{
	switch (axis) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_JOYPAD_AXIS_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

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
CUTE_API CF_Result CUTE_CALL cf_joypad_add_mapping(const char* mapping);

/**
 * Returns the number of joypads currently connected to the system.
 */
CUTE_API int CUTE_CALL cf_joypad_count();

/**
 * Opens a joypad on the system.
 * `index` is a number from 0 to `joypad_count`. The first joypad connected to the system is 0,
 * the second is 1, and so on.
 */
CUTE_API CF_Joypad* CUTE_CALL cf_joypad_open(int index);

/**
 * Destroys a joypad previously opened by `joypad_open`.
 */
CUTE_API void CUTE_CALL cf_joypad_close(CF_Joypad* joypad);

/**
 * Tests to see if the joypad is still connected to the system (returns true if it is).
 */
CUTE_API bool CUTE_CALL cf_joypad_is_connected(CF_Joypad* joypad);

/**
 * Returns the power level of the joypad.
 */
CUTE_API CF_JoypadPowerLevel CUTE_CALL cf_joypad_power_level(CF_Joypad* joypad);

/**
 * Returns the name of the joypad.
 */
CUTE_API const char* CUTE_CALL cf_joypad_name(CF_Joypad* joypad);

CUTE_API bool CUTE_CALL cf_joypad_button_is_down(CF_Joypad* joypad, CF_JoypadButton button);
CUTE_API bool CUTE_CALL cf_joypad_button_is_up(CF_Joypad* joypad, CF_JoypadButton button);
CUTE_API bool CUTE_CALL cf_joypad_button_was_pressed(CF_Joypad* joypad, CF_JoypadButton button);
CUTE_API bool CUTE_CALL cf_joypad_button_was_released(CF_Joypad* joypad, CF_JoypadButton button);
CUTE_API int16_t CUTE_CALL cf_joypad_axis(CF_Joypad* joypad, CF_JoypadAxis axis);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using App = CF_App;
using Joypad = CF_Joypad;

using JoypadPowerLevel = CF_JoypadPowerLevel;
#define CF_ENUM(K, V) CUTE_INLINE constexpr JoypadPowerLevel K = CF_##K;
CF_JOYPAD_POWER_LEVEL_DEFS
#undef CF_ENUM

CUTE_INLINE const char* joypad_power_level_to_string(JoypadPowerLevel level)
{
	switch (level) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_JOYPAD_POWER_LEVEL_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using JoypadButton = CF_JoypadButton;
#define CF_ENUM(K, V) CUTE_INLINE constexpr JoypadButton K = CF_##K;
CF_JOYPAD_BUTTON_DEFS
#undef CF_ENUM

CUTE_INLINE const char* joypad_button_to_string(JoypadButton button)
{
	switch (button) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_JOYPAD_BUTTON_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using JoypadAxis = CF_JoypadAxis;
#define CF_ENUM(K, V) CUTE_INLINE constexpr JoypadAxis K = CF_##K;
CF_JOYPAD_AXIS_DEFS
#undef CF_ENUM

CUTE_INLINE const char* joypad_axis_to_string(JoypadAxis axis)
{
	switch (axis) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_JOYPAD_AXIS_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CUTE_INLINE void joypad_system_init() { cf_joypad_system_init(); }
CUTE_INLINE CF_Result joypad_add_mapping(const char* mapping) { return cf_joypad_add_mapping(mapping); }
CUTE_INLINE int joypad_count() { return cf_joypad_count(); }
CUTE_INLINE CF_Joypad* joypad_open(int index) { return cf_joypad_open(index); }
CUTE_INLINE void joypad_close(Joypad* joypad) { cf_joypad_close(joypad); }
CUTE_INLINE bool joypad_is_connected(Joypad* joypad) { return cf_joypad_is_connected(joypad); }
CUTE_INLINE JoypadPowerLevel joypad_power_level(Joypad* joypad) { return cf_joypad_power_level(joypad); }
CUTE_INLINE const char* joypad_name(Joypad* joypad) { return cf_joypad_name(joypad); }
CUTE_INLINE bool joypad_button_is_down(Joypad* joypad, JoypadButton button) { return cf_joypad_button_is_down(joypad, button); }
CUTE_INLINE bool joypad_button_is_up(Joypad* joypad, JoypadButton button) { return cf_joypad_button_is_up(joypad, button); }
CUTE_INLINE bool joypad_button_was_pressed(Joypad* joypad, JoypadButton button) { return cf_joypad_button_was_pressed(joypad, button); }
CUTE_INLINE bool joypad_button_was_released(Joypad* joypad, JoypadButton button) { return cf_joypad_button_was_released(joypad, button); }
CUTE_INLINE int16_t joypad_axis(Joypad* joypad, JoypadAxis axis) { return cf_joypad_axis(joypad, axis); }

}

#endif // CUTE_CPP

#endif // CUTE_JOYPAD_H
