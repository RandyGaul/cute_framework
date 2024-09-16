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

#define CF_MAX_JOYPADS 8

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
 * @related  CF_JoypadAxis cf_joypad_axis_to_string CF_Joypad cf_joypad_axis CF_JoypadType
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
 * @related  CF_JoypadAxis cf_joypad_axis_to_string CF_Joypad cf_joypad_axis CF_JoypadType
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
 * @enum     CF_JoypadType
 * @category input
 * @brief    Various types of joypads by enum name.
 * @related  CF_Joypad CF_JoypadType cf_joypad_type_to_string cf_joypad_type
 */
#define CF_JOYPAD_TYPE_DEFS \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_UNKNOWN, 0) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_XBOX360, 1) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_XBOXONE, 2) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_PS3, 3) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_PS4, 4) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_NINENTDO_SWITCH_PRO, 5) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_VIRTUAL, 6) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_PS5, 7) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_AMAZON_LUNA, 8) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_GOOGLE_STADIA, 9) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_NVIDIA_SHIELD, 10) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT, 11) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT, 12) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR, 13) \
	/* @entry */ \
	CF_ENUM(JOYPAD_TYPE_COUNT, 14) \
	/* @end */

typedef enum CF_JoypadType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_JOYPAD_TYPE_DEFS
	#undef CF_ENUM
} CF_JoypadType;

/**
 * @function cf_joypad_type_to_string
 * @category input
 * @brief    Convert an enum `CF_JoypadType` to a c-style string.
 * @param    state        The state to convert to a string.
 * @related  CF_Joypad CF_JoypadType cf_joypad_type_to_string cf_joypad_type
 */
CF_INLINE const char* cf_joypad_type_to_string(CF_JoypadType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_JOYPAD_TYPE_DEFS
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
 * @remarks  This may return a number larger than `CF_MAX_JOYPADS` (8), though, only up to 8 will
 *           be seen by Cute Framework.
 * @related  CF_Joypad cf_joypad_count cf_joypad_open cf_joypad_close
 */
CF_API int CF_CALL cf_joypad_count();

/**
 * @function cf_joypad_is_connected
 * @category input
 * @brief    Returns true if a joypad is connected.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @related  CF_Joypad cf_joypad_count cf_joypad_open cf_joypad_close
 */
CF_API bool CF_CALL cf_joypad_is_connected(int player_index);

/**
 * @function cf_joypad_power_level
 * @category input
 * @brief    Returns the power level of the joypad.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @related  CF_JoypadPowerLevel cf_joypad_power_level_to_string cf_joypad_power_level CF_Joypad
 */
CF_API CF_JoypadPowerLevel CF_CALL cf_joypad_power_level(int player_index);

/**
 * @function cf_joypad_name
 * @category input
 * @brief    Returns the name of the joypad.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @related  CF_Joypad cf_joypad_count cf_joypad_open cf_joypad_close
 */
CF_API const char* CF_CALL cf_joypad_name(int player_index);

/**
 * @function cf_joypad_type
 * @category input
 * @brief    Returns the type of the joypad.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @related  CF_Joypad CF_JoypadType
 */
CF_API CF_JoypadType CF_CALL cf_joypad_type(int player_index);

/**
 * @function cf_joypad_vendor
 * @category input
 * @brief    Returns the USB vendor ID.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @remarks  Returns 0 if not available.
 * @related  CF_Joypad CF_JoypadType
 */
CF_API uint16_t CF_CALL cf_joypad_vendor(int player_index);

/**
 * @function cf_joypad_product_id
 * @category input
 * @brief    Returns the USB product ID.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @remarks  Returns 0 if not available.
 * @related  CF_Joypad CF_JoypadType
 */
CF_API uint16_t CF_CALL cf_joypad_product_id(int player_index);

/**
 * @function cf_joypad_serial_number
 * @category input
 * @brief    Returns the serial number.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @remarks  Returns 0 if not available.
 * @related  CF_Joypad CF_JoypadType
 */
CF_API const char* CF_CALL cf_joypad_serial_number(int player_index);

/**
 * @function cf_joypad_firmware_version
 * @category input
 * @brief    Returns the firmware version.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @remarks  Returns 0 if not available.
 * @related  CF_Joypad CF_JoypadType
 */
CF_API uint16_t CF_CALL cf_joypad_firmware_version(int player_index);

/**
 * @function cf_joypad_product_version
 * @category input
 * @brief    Returns the product version.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @remarks  Returns 0 if not available.
 * @related  CF_Joypad CF_JoypadType
 */
CF_API uint16_t CF_CALL cf_joypad_product_version(int player_index);

/**
 * @function cf_joypad_button_down
 * @category input
 * @brief    Returns true if the button is currently down.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @param    button     The button.
 * @related  CF_Joypad CF_JoypadButton cf_joypad_button_down cf_joypad_button_just_pressed cf_joypad_button_just_released cf_joypad_axis
 */
CF_API bool CF_CALL cf_joypad_button_down(int player_index, CF_JoypadButton button);

/**
 * @function cf_joypad_button_just_pressed
 * @category input
 * @brief    Returns true if the button was just pressed.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @param    button     The button.
 * @related  CF_Joypad CF_JoypadButton cf_joypad_button_down cf_joypad_button_just_pressed cf_joypad_button_just_released cf_joypad_axis
 */
CF_API bool CF_CALL cf_joypad_button_just_pressed(int player_index, CF_JoypadButton button);

/**
 * @function cf_joypad_button_just_released
 * @category input
 * @brief    Returns true if the button was just released.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @param    button     The button.
 * @related  CF_Joypad CF_JoypadButton cf_joypad_button_down cf_joypad_button_just_pressed cf_joypad_button_just_released cf_joypad_axis
 */
CF_API bool CF_CALL cf_joypad_button_just_released(int player_index, CF_JoypadButton button);

/**
 * @function cf_joypad_axis
 * @category input
 * @brief    Returns a signed 16-bit integer representing how much a joypad axis is activated by.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @param    axis       The axis.
 * @related  CF_Joypad CF_JoypadButton cf_joypad_button_down cf_joypad_button_just_pressed cf_joypad_button_just_released cf_joypad_axis
 */
CF_API int16_t CF_CALL cf_joypad_axis(int player_index, CF_JoypadAxis axis);

/**
 * @function cf_joypad_rumble
 * @category input
 * @brief    Rumbles the joypad.
 * @param    player_index     An index represeting the joypad for a particular player, starting at 0.
 * @param    lo_frequency_rumble       Rumble intensity from 0 to 65535. Represents the low frequency motor (or left motor).
 * @param    hi_frequency_rumble       Rumble intensity from 0 to 65535. Represents the high frequency motor (or right motor).
 * @remarks  Calling this function cancels any previous rumbles. Sending in 0 for either low/high frequency parameters cancels the rumble.
 * @related  CF_Joypad CF_JoypadButton cf_joypad_button_down cf_joypad_button_just_pressed cf_joypad_button_just_released cf_joypad_axis
 */
CF_API void CF_CALL cf_joypad_rumble(int player_index, uint16_t lo_frequency_rumble, uint16_t hi_frequency_rumble, int duration_ms);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

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

using JoypadType = CF_JoypadType;
#define CF_ENUM(K, V) CF_INLINE constexpr JoypadType K = CF_##K;
CF_JOYPAD_TYPE_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(JoypadType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_JOYPAD_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CF_INLINE Result joypad_add_mapping(const char* mapping) { return cf_joypad_add_mapping(mapping); }
CF_INLINE int joypad_count() { return cf_joypad_count(); }
CF_INLINE bool joypad_is_connected(int player_index) { return cf_joypad_is_connected(player_index); }
CF_INLINE JoypadPowerLevel joypad_power_level(int player_index) { return cf_joypad_power_level(player_index); }
CF_INLINE const char* joypad_name(int player_index) { return cf_joypad_name(player_index); }
CF_INLINE CF_JoypadType joypad_type(int player_index) { return cf_joypad_type(player_index); }
CF_INLINE uint16_t joypad_vendor(int player_index) { return cf_joypad_vendor(player_index); }
CF_INLINE uint16_t joypad_product_id(int player_index) { return cf_joypad_product_id(player_index); }
CF_INLINE const char* joypad_serial_number(int player_index) { return cf_joypad_serial_number(player_index); }
CF_INLINE uint16_t joypad_firmware_version(int player_index) { return cf_joypad_firmware_version(player_index); }
CF_INLINE uint16_t joypad_product_version(int player_index) { return cf_joypad_product_version(player_index); }
CF_INLINE bool joypad_button_down(int player_index, JoypadButton button) { return cf_joypad_button_down(player_index, button); }
CF_INLINE bool joypad_button_just_pressed(int player_index, JoypadButton button) { return cf_joypad_button_just_pressed(player_index, button); }
CF_INLINE bool joypad_button_just_released(int player_index, JoypadButton button) { return cf_joypad_button_just_released(player_index, button); }
CF_INLINE int16_t joypad_axis(int player_index, JoypadAxis axis) { return cf_joypad_axis(player_index, axis); }
CF_INLINE void joypad_rumble(int player_index, uint16_t lo_frequency_rumble, uint16_t hi_frequency_rumble, int duration_ms) { cf_joypad_rumble(player_index, lo_frequency_rumble, hi_frequency_rumble, duration_ms); }

}

#endif // CF_CPP

#endif // CF_JOYPAD_H
