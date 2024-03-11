/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_JOYPAD_H
#define CF_JOYPAD_H

#include "cute_defines.h"
#include "cute_result.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Joypad
 * @category input
 * @brief    An opaque pointer representing a joypad.
 * @related  CF_Joypad cf_joypad_open cf_joypad_button_down cf_joypad_axis
 */
typedef struct CF_Joypad CF_Joypad;
// @end

/**
 * @enum     CF_JoypadPowerLevel
 * @category input
 * @brief    The states of power for a `CF_Joypad`.
 * @related  CF_JoypadPowerLevel cf_joypad_power_level_to_string cf_joypad_power_level CF_Joypad
 */
#define CF_JOYPAD_POWER_LEVEL_DEFS \
	/* @entry */ \
	CF_ENUM(JOYPAD_POWER_LEVEL_UNKNOWN, 0) \
	/* @entry */ \
	CF_ENUM(JOYPAD_POWER_LEVEL_EMPTY, 1) \
	/* @entry */ \
	CF_ENUM(JOYPAD_POWER_LEVEL_LOW, 2) \
	/* @entry */ \
	CF_ENUM(JOYPAD_POWER_LEVEL_MEDIUM, 3) \
	/* @entry */ \
	CF_ENUM(JOYPAD_POWER_LEVEL_FULL, 4) \
	/* @entry */ \
	CF_ENUM(JOYPAD_POWER_LEVEL_WIRED, 5) \
	/* @entry */ \
	CF_ENUM(JOYPAD_POWER_LEVEL_COUNT, 6) \
	/* @end */

typedef enum CF_JoypadPowerLevel
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_JOYPAD_POWER_LEVEL_DEFS
	#undef CF_ENUM
} CF_JoypadPowerLevel;

/**
 * @function cf_joypad_power_level_to_string
 * @category input
 * @brief    Convert an enum `CF_JoypadPowerLevel` to a c-style string.
 * @param    state        The state to convert to a string.
 * @related  CF_JoypadPowerLevel cf_joypad_power_level_to_string cf_joypad_power_level CF_Joypad
 */
CF_INLINE const char* cf_joypad_power_level_to_string(CF_JoypadPowerLevel level)
{
	switch (level) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_JOYPAD_POWER_LEVEL_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @enum     CF_JoypadButton
 * @category input
 * @brief    Various buttons on a `CF_Joypad`.
 * @related  CF_JoypadButton cf_joypad_button_to_string CF_Joypad cf_joypad_button_down
 */
#define CF_JOYPAD_BUTTON_DEFS \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_INVALID, -1) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_A, 0) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_B, 1) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_X, 2) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_Y, 3) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_BACK, 4) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_GUIDE, 5) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_START, 6) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_LEFTSTICK, 7) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_RIGHTSTICK, 8) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_LEFTSHOULDER, 9) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_RIGHTSHOULDER, 10) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_DPAD_UP, 11) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_DPAD_DOWN, 12) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_DPAD_LEFT, 13) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_DPAD_RIGHT, 14) \
	/* @entry */ \
	CF_ENUM(JOYPAD_BUTTON_COUNT, 15) \
	/* @end */

typedef enum CF_JoypadButton
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_JOYPAD_BUTTON_DEFS
	#undef CF_ENUM
} CF_JoypadButton;

/**
 * @function cf_joypad_button_to_string
 * @category input
 * @brief    Convert an enum `CF_JoypadButton` to a c-style string.
 * @param    state        The state to convert to a string.
 * @related  CF_JoypadButton cf_joypad_button_to_string CF_Joypad cf_joypad_button_down
 */
CF_INLINE const char* cf_joypad_button_to_string(CF_JoypadButton button)
{
	switch (button) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_JOYPAD_BUTTON_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @enum     CF_JoypadAxis
 * @category input
 * @brief    Various axis actions on a `CF_Joypad`.
 * @related  CF_JoypadAxis cf_joypad_axis_to_string CF_Joypad cf_joypad_axis
 */
#define CF_JOYPAD_AXIS_DEFS \
	/* @entry */ \
	CF_ENUM(JOYPAD_AXIS_INVALID, -1) \
	/* @entry */ \
	CF_ENUM(JOYPAD_AXIS_LEFTX, 0) \
	/* @entry */ \
	CF_ENUM(JOYPAD_AXIS_LEFTY, 1) \
	/* @entry */ \
	CF_ENUM(JOYPAD_AXIS_RIGHTX, 2) \
	/* @entry */ \
	CF_ENUM(JOYPAD_AXIS_RIGHTY, 3) \
	/* @entry */ \
	CF_ENUM(JOYPAD_AXIS_TRIGGERLEFT, 4) \
	/* @entry */ \
	CF_ENUM(JOYPAD_AXIS_TRIGGERRIGHT, 5) \
	/* @entry */ \
	CF_ENUM(JOYPAD_AXIS_COUNT, 6) \
	/* @end */

typedef enum CF_JoypadAxis
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_JOYPAD_AXIS_DEFS
	#undef CF_ENUM
} CF_JoypadAxis;

/**
 * @function cf_joypad_axis_to_string
 * @category input
 * @brief    Convert an enum `CF_JoypadAxis` to a c-style string.
 * @param    state        The state to convert to a string.
 * @related  CF_JoypadAxis cf_joypad_axis_to_string CF_Joypad cf_joypad_axis
 */
CF_INLINE const char* cf_joypad_axis_to_string(CF_JoypadAxis axis)
{
	switch (axis) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_JOYPAD_AXIS_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_joypad_add_mapping
 * @category input
 * @brief    Adds an SDL2 mapping to the joypad system.
 * @remarks  For each valid mapping string added, another kind of joypad is supported.
 *           Cute Framework automatically initializes many mappings from the community organized mapping
 *           database on GitHub (https://github.com/gabomdq/SDL_GameControllerDB), so you probably don't need
 *           to ever call this function.
 * @related  CF_Joypad
 */
CF_API CF_Result CF_CALL cf_joypad_add_mapping(const char* mapping);

/**
 * @function cf_joypad_count
 * @category input
 * @brief    Returns the number of joypads currently connected to the system.
 * @related  CF_Joypad cf_joypad_count cf_joypad_open cf_joypad_close
 */
CF_API int CF_CALL cf_joypad_count();

/**
 * @function cf_joypad_open
 * @category input
 * @brief    Opens a joypad on the system.
 * @param    index      Which joypad to open.
 * @remarks  The first joypad connected to the system is 0, the second is 1, and so on.
 * @related  CF_Joypad cf_joypad_count cf_joypad_open cf_joypad_close
 */
CF_API CF_Joypad* CF_CALL cf_joypad_open(int index);

/**
 * @function cf_joypad_close
 * @category input
 * @brief    Destroys a joypad previously opened by `joypad_open`.
 * @param    joypad     The joypad.
 * @related  CF_Joypad cf_joypad_count cf_joypad_open cf_joypad_close
 */
CF_API void CF_CALL cf_joypad_close(CF_Joypad* joypad);

/**
 * @function cf_joypad_is_connected
 * @category input
 * @brief    Returns true if a joypad is connected.
 * @param    joypad     The joypad.
 * @related  CF_Joypad cf_joypad_count cf_joypad_open cf_joypad_close
 */
CF_API bool CF_CALL cf_joypad_is_connected(CF_Joypad* joypad);

/**
 * @function cf_joypad_power_level
 * @category input
 * @brief    Returns the power level of the joypad.
 * @param    joypad     The joypad.
 * @related  CF_JoypadPowerLevel cf_joypad_power_level_to_string cf_joypad_power_level CF_Joypad
 */
CF_API CF_JoypadPowerLevel CF_CALL cf_joypad_power_level(CF_Joypad* joypad);

/**
 * @function cf_joypad_name
 * @category input
 * @brief    Returns the name of the joypad.
 * @param    joypad     The joypad.
 * @related  CF_Joypad cf_joypad_count cf_joypad_open cf_joypad_close
 */
CF_API const char* CF_CALL cf_joypad_name(CF_Joypad* joypad);

/**
 * @function cf_joypad_button_down
 * @category input
 * @brief    Returns true if the button is currently down.
 * @param    joypad     The joypad.
 * @param    button     The button.
 * @related  CF_Joypad CF_JoypadButton cf_joypad_button_down cf_joypad_button_just_pressed cf_joypad_button_just_released cf_joypad_axis
 */
CF_API bool CF_CALL cf_joypad_button_down(CF_Joypad* joypad, CF_JoypadButton button);

/**
 * @function cf_joypad_button_just_pressed
 * @category input
 * @brief    Returns true if the button was just pressed.
 * @param    joypad     The joypad.
 * @param    button     The button.
 * @related  CF_Joypad CF_JoypadButton cf_joypad_button_down cf_joypad_button_just_pressed cf_joypad_button_just_released cf_joypad_axis
 */
CF_API bool CF_CALL cf_joypad_button_just_pressed(CF_Joypad* joypad, CF_JoypadButton button);

/**
 * @function cf_joypad_button_just_released
 * @category input
 * @brief    Returns true if the button was just released.
 * @param    joypad     The joypad.
 * @param    button     The button.
 * @related  CF_Joypad CF_JoypadButton cf_joypad_button_down cf_joypad_button_just_pressed cf_joypad_button_just_released cf_joypad_axis
 */
CF_API bool CF_CALL cf_joypad_button_just_released(CF_Joypad* joypad, CF_JoypadButton button);

/**
 * @function cf_joypad_axis
 * @category input
 * @brief    Returns a signed 16-bit integer representing how much a joypad axis is activated by.
 * @param    joypad     The joypad.
 * @param    axis       The axis.
 * @related  CF_Joypad CF_JoypadButton cf_joypad_button_down cf_joypad_button_just_pressed cf_joypad_button_just_released cf_joypad_axis
 */
CF_API int16_t CF_CALL cf_joypad_axis(CF_Joypad* joypad, CF_JoypadAxis axis);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Joypad = CF_Joypad;

using JoypadPowerLevel = CF_JoypadPowerLevel;
#define CF_ENUM(K, V) CF_INLINE constexpr JoypadPowerLevel K = CF_##K;
CF_JOYPAD_POWER_LEVEL_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(JoypadPowerLevel level)
{
	switch (level) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_JOYPAD_POWER_LEVEL_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using JoypadButton = CF_JoypadButton;
#define CF_ENUM(K, V) CF_INLINE constexpr JoypadButton K = CF_##K;
CF_JOYPAD_BUTTON_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(JoypadButton button)
{
	switch (button) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_JOYPAD_BUTTON_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using JoypadAxis = CF_JoypadAxis;
#define CF_ENUM(K, V) CF_INLINE constexpr JoypadAxis K = CF_##K;
CF_JOYPAD_AXIS_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(JoypadAxis axis)
{
	switch (axis) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_JOYPAD_AXIS_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CF_INLINE CF_Result joypad_add_mapping(const char* mapping) { return cf_joypad_add_mapping(mapping); }
CF_INLINE int joypad_count() { return cf_joypad_count(); }
CF_INLINE CF_Joypad* joypad_open(int index) { return cf_joypad_open(index); }
CF_INLINE void joypad_close(Joypad* joypad) { cf_joypad_close(joypad); }
CF_INLINE bool joypad_is_connected(Joypad* joypad) { return cf_joypad_is_connected(joypad); }
CF_INLINE JoypadPowerLevel joypad_power_level(Joypad* joypad) { return cf_joypad_power_level(joypad); }
CF_INLINE const char* joypad_name(Joypad* joypad) { return cf_joypad_name(joypad); }
CF_INLINE bool joypad_button_down(Joypad* joypad, JoypadButton button) { return cf_joypad_button_down(joypad, button); }
CF_INLINE bool joypad_button_was_pressed(Joypad* joypad, JoypadButton button) { return cf_joypad_button_just_pressed(joypad, button); }
CF_INLINE bool joypad_button_was_released(Joypad* joypad, JoypadButton button) { return cf_joypad_button_just_released(joypad, button); }
CF_INLINE int16_t joypad_axis(Joypad* joypad, JoypadAxis axis) { return cf_joypad_axis(joypad, axis); }

}

#endif // CF_CPP

#endif // CF_JOYPAD_H
