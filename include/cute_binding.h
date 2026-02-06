/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_BINDING_H
#define CF_BINDING_H

#include "cute_defines.h"
#include "cute_math.h"
#include "cute_input.h"
#include "cute_joypad.h"
#include "cute_time.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_ButtonBinding
 * @category binding
 * @brief    A handle to a button binding, which maps multiple physical inputs to a single abstract button.
 * @remarks  Supports input buffering, where presses/releases persist over a small time window for forgiving input.
 *           Create with `cf_make_button_binding`, destroy with `cf_destroy_button_binding`.
 * @related  CF_ButtonBinding CF_AxisBinding CF_StickBinding cf_make_button_binding cf_destroy_button_binding
 */
typedef struct CF_ButtonBinding { uint64_t id; } CF_ButtonBinding;
// @end

/**
 * @struct   CF_AxisBinding
 * @category binding
 * @brief    A handle to an axis binding, which maps a negative/positive button pair to a -1..1 axis.
 * @remarks  Supports conflict resolution when both directions are pressed simultaneously.
 *           Create with `cf_make_axis_binding`, destroy with `cf_destroy_axis_binding`.
 * @related  CF_ButtonBinding CF_AxisBinding CF_StickBinding cf_make_axis_binding cf_destroy_axis_binding
 */
typedef struct CF_AxisBinding { uint64_t id; } CF_AxisBinding;
// @end

/**
 * @struct   CF_StickBinding
 * @category binding
 * @brief    A handle to a stick binding, combining two axis bindings (x and y) into a 2D stick.
 * @remarks  Create with `cf_make_stick_binding`, destroy with `cf_destroy_stick_binding`.
 * @related  CF_ButtonBinding CF_AxisBinding CF_StickBinding cf_make_stick_binding cf_destroy_stick_binding
 */
typedef struct CF_StickBinding { uint64_t id; } CF_StickBinding;
// @end

/**
 * @enum     CF_AxisConflict
 * @category binding
 * @brief    How to resolve simultaneous opposing directions on an axis.
 * @related  CF_AxisConflict cf_axis_binding_set_conflict
 */
#define CF_AXIS_CONFLICT_DEFS \
	/* @entry If both directions pressed, use the newest. */ \
	CF_ENUM(AXIS_CONFLICT_NEWEST, 0) \
	/* @entry If both directions pressed, use the oldest. */ \
	CF_ENUM(AXIS_CONFLICT_OLDEST, 1) \
	/* @entry If both directions pressed, return 0. */ \
	CF_ENUM(AXIS_CONFLICT_CANCEL, 2) \
	/* @end */

typedef enum CF_AxisConflict
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_AXIS_CONFLICT_DEFS
	#undef CF_ENUM
} CF_AxisConflict;

/**
 * @function cf_axis_conflict_to_string
 * @category binding
 * @brief    Converts a `CF_AxisConflict` to a c-string.
 * @param    conflict      The conflict mode.
 * @related  CF_AxisConflict
 */
CF_INLINE const char* cf_axis_conflict_to_string(CF_AxisConflict conflict)
{
	switch (conflict) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_AXIS_CONFLICT_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

//--------------------------------------------------------------------------------------------------
// ButtonBinding API.

/**
 * @function cf_make_button_binding
 * @category binding
 * @brief    Creates a button binding for the given player.
 * @param    player_index   The joypad player index (0 for single player).
 * @param    press_buffer   Seconds to buffer press/release events for forgiving input.
 * @return   Returns a `CF_ButtonBinding` handle.
 * @related  CF_ButtonBinding cf_make_button_binding cf_destroy_button_binding
 */
CF_API CF_ButtonBinding CF_CALL cf_make_button_binding(int player_index, float press_buffer);

/**
 * @function cf_destroy_button_binding
 * @category binding
 * @brief    Destroys a button binding.
 * @param    b              The button binding handle.
 * @related  CF_ButtonBinding cf_make_button_binding cf_destroy_button_binding
 */
CF_API void CF_CALL cf_destroy_button_binding(CF_ButtonBinding b);

/**
 * @function cf_button_binding_add_key
 * @category binding
 * @brief    Binds a keyboard key to this button binding.
 * @param    b              The button binding handle.
 * @param    key            The key to bind.
 * @related  CF_ButtonBinding cf_button_binding_add_key cf_button_binding_add_mouse_button cf_button_binding_add_joypad_button cf_button_binding_add_trigger
 */
CF_API void CF_CALL cf_button_binding_add_key(CF_ButtonBinding b, CF_KeyButton key);

/**
 * @function cf_button_binding_add_mouse_button
 * @category binding
 * @brief    Binds a mouse button to this button binding.
 * @param    b              The button binding handle.
 * @param    button         The mouse button to bind.
 * @related  CF_ButtonBinding cf_button_binding_add_key cf_button_binding_add_mouse_button cf_button_binding_add_joypad_button cf_button_binding_add_trigger
 */
CF_API void CF_CALL cf_button_binding_add_mouse_button(CF_ButtonBinding b, CF_MouseButton button);

/**
 * @function cf_button_binding_add_joypad_button
 * @category binding
 * @brief    Binds a joypad button to this button binding.
 * @param    b              The button binding handle.
 * @param    button         The joypad button to bind.
 * @related  CF_ButtonBinding cf_button_binding_add_key cf_button_binding_add_mouse_button cf_button_binding_add_joypad_button cf_button_binding_add_trigger
 */
CF_API void CF_CALL cf_button_binding_add_joypad_button(CF_ButtonBinding b, CF_JoypadButton button);

/**
 * @function cf_button_binding_add_trigger
 * @category binding
 * @brief    Binds a joypad axis (as a trigger/threshold) to this button binding.
 * @param    b              The button binding handle.
 * @param    axis           The joypad axis to use.
 * @param    threshold      The threshold value (0..1) at which the axis counts as pressed.
 * @param    positive       True to trigger on positive axis values, false for negative.
 * @related  CF_ButtonBinding cf_button_binding_add_key cf_button_binding_add_mouse_button cf_button_binding_add_joypad_button cf_button_binding_add_trigger
 */
CF_API void CF_CALL cf_button_binding_add_trigger(CF_ButtonBinding b, CF_JoypadAxis axis, float threshold, bool positive);

/**
 * @function cf_button_binding_pressed
 * @category binding
 * @brief    Returns true if the button was pressed (with input buffering).
 * @param    b              The button binding handle.
 * @related  CF_ButtonBinding cf_button_binding_pressed cf_button_binding_released cf_button_binding_down cf_button_binding_value
 */
CF_API bool CF_CALL cf_button_binding_pressed(CF_ButtonBinding b);

/**
 * @function cf_button_binding_released
 * @category binding
 * @brief    Returns true if the button was released (with input buffering).
 * @param    b              The button binding handle.
 * @related  CF_ButtonBinding cf_button_binding_pressed cf_button_binding_released cf_button_binding_down cf_button_binding_value
 */
CF_API bool CF_CALL cf_button_binding_released(CF_ButtonBinding b);

/**
 * @function cf_button_binding_down
 * @category binding
 * @brief    Returns true if the button is currently held down.
 * @param    b              The button binding handle.
 * @related  CF_ButtonBinding cf_button_binding_pressed cf_button_binding_released cf_button_binding_down cf_button_binding_value
 */
CF_API bool CF_CALL cf_button_binding_down(CF_ButtonBinding b);

/**
 * @function cf_button_binding_value
 * @category binding
 * @brief    Returns the analog value of the button (0..1).
 * @param    b              The button binding handle.
 * @related  CF_ButtonBinding cf_button_binding_value cf_button_binding_sign
 */
CF_API float CF_CALL cf_button_binding_value(CF_ButtonBinding b);

/**
 * @function cf_button_binding_sign
 * @category binding
 * @brief    Returns -1, 0, or 1 based on the button value.
 * @param    b              The button binding handle.
 * @related  CF_ButtonBinding cf_button_binding_value cf_button_binding_sign
 */
CF_API float CF_CALL cf_button_binding_sign(CF_ButtonBinding b);

/**
 * @function cf_button_binding_consume_press
 * @category binding
 * @brief    Consumes the press event so subsequent queries return false until a new press occurs.
 * @param    b              The button binding handle.
 * @return   Returns true if there was a press to consume.
 * @related  CF_ButtonBinding cf_button_binding_consume_press cf_button_binding_consume_release
 */
CF_API bool CF_CALL cf_button_binding_consume_press(CF_ButtonBinding b);

/**
 * @function cf_button_binding_consume_release
 * @category binding
 * @brief    Consumes the release event so subsequent queries return false until a new release occurs.
 * @param    b              The button binding handle.
 * @return   Returns true if there was a release to consume.
 * @related  CF_ButtonBinding cf_button_binding_consume_press cf_button_binding_consume_release
 */
CF_API bool CF_CALL cf_button_binding_consume_release(CF_ButtonBinding b);

/**
 * @function cf_button_binding_set_deadzone
 * @category binding
 * @brief    Sets a per-binding deadzone, overriding the global deadzone for this binding.
 * @param    b              The button binding handle.
 * @param    deadzone       The deadzone value. Set to -1 to use the global deadzone.
 * @related  CF_ButtonBinding cf_binding_set_deadzone cf_axis_binding_set_deadzone
 */
CF_API void CF_CALL cf_button_binding_set_deadzone(CF_ButtonBinding b, float deadzone);

/**
 * @function cf_button_binding_value_raw
 * @category binding
 * @brief    Returns the raw analog value (0..1) with no deadzone applied.
 * @param    b              The button binding handle.
 * @related  CF_ButtonBinding cf_button_binding_value cf_axis_binding_value_raw cf_stick_binding_value_raw
 */
CF_API float CF_CALL cf_button_binding_value_raw(CF_ButtonBinding b);

/**
 * @function cf_button_binding_set_repeat
 * @category binding
 * @brief    Enables key repeat for this button binding.
 * @param    b              The button binding handle.
 * @param    delay          Seconds before the first repeat fires. Set to -1 to disable repeat.
 * @param    interval       Seconds between subsequent repeats.
 * @related  CF_ButtonBinding cf_button_binding_repeated
 */
CF_API void CF_CALL cf_button_binding_set_repeat(CF_ButtonBinding b, float delay, float interval);

/**
 * @function cf_button_binding_repeated
 * @category binding
 * @brief    Returns true on frames where the key repeat fires.
 * @param    b              The button binding handle.
 * @related  CF_ButtonBinding cf_button_binding_set_repeat
 */
CF_API bool CF_CALL cf_button_binding_repeated(CF_ButtonBinding b);

//--------------------------------------------------------------------------------------------------
// AxisBinding API.

/**
 * @function cf_make_axis_binding
 * @category binding
 * @brief    Creates an axis binding for the given player.
 * @param    player_index   The joypad player index (0 for single player).
 * @return   Returns a `CF_AxisBinding` handle.
 * @related  CF_AxisBinding cf_make_axis_binding cf_destroy_axis_binding
 */
CF_API CF_AxisBinding CF_CALL cf_make_axis_binding(int player_index);

/**
 * @function cf_destroy_axis_binding
 * @category binding
 * @brief    Destroys an axis binding and its internal button bindings.
 * @param    a              The axis binding handle.
 * @related  CF_AxisBinding cf_make_axis_binding cf_destroy_axis_binding
 */
CF_API void CF_CALL cf_destroy_axis_binding(CF_AxisBinding a);

/**
 * @function cf_axis_binding_add_keys
 * @category binding
 * @brief    Binds a negative/positive key pair to this axis.
 * @param    a              The axis binding handle.
 * @param    negative       The key for negative direction.
 * @param    positive       The key for positive direction.
 * @related  CF_AxisBinding cf_axis_binding_add_keys cf_axis_binding_add_mouse_buttons cf_axis_binding_add_joypad_buttons
 */
CF_API void CF_CALL cf_axis_binding_add_keys(CF_AxisBinding a, CF_KeyButton negative, CF_KeyButton positive);

/**
 * @function cf_axis_binding_add_mouse_buttons
 * @category binding
 * @brief    Binds a negative/positive mouse button pair to this axis.
 * @param    a              The axis binding handle.
 * @param    negative       The mouse button for negative direction.
 * @param    positive       The mouse button for positive direction.
 * @related  CF_AxisBinding cf_axis_binding_add_keys cf_axis_binding_add_mouse_buttons cf_axis_binding_add_joypad_buttons
 */
CF_API void CF_CALL cf_axis_binding_add_mouse_buttons(CF_AxisBinding a, CF_MouseButton negative, CF_MouseButton positive);

/**
 * @function cf_axis_binding_add_joypad_buttons
 * @category binding
 * @brief    Binds a negative/positive joypad button pair to this axis.
 * @param    a              The axis binding handle.
 * @param    negative       The joypad button for negative direction.
 * @param    positive       The joypad button for positive direction.
 * @related  CF_AxisBinding cf_axis_binding_add_keys cf_axis_binding_add_mouse_buttons cf_axis_binding_add_joypad_buttons
 */
CF_API void CF_CALL cf_axis_binding_add_joypad_buttons(CF_AxisBinding a, CF_JoypadButton negative, CF_JoypadButton positive);

/**
 * @function cf_axis_binding_add_triggers
 * @category binding
 * @brief    Binds a negative/positive joypad axis pair (as triggers) to this axis.
 * @param    a              The axis binding handle.
 * @param    negative       The joypad axis for negative direction.
 * @param    positive       The joypad axis for positive direction.
 * @param    threshold      The threshold value (0..1) at which the axis counts as pressed.
 * @related  CF_AxisBinding cf_axis_binding_add_triggers
 */
CF_API void CF_CALL cf_axis_binding_add_triggers(CF_AxisBinding a, CF_JoypadAxis negative, CF_JoypadAxis positive, float threshold);

/**
 * @function cf_axis_binding_add_left_stick_x
 * @category binding
 * @brief    Binds the left stick X axis.
 * @param    a              The axis binding handle.
 * @param    threshold      Deadzone threshold.
 * @related  CF_AxisBinding cf_axis_binding_add_left_stick_x cf_axis_binding_add_left_stick_y cf_axis_binding_add_right_stick_x cf_axis_binding_add_right_stick_y
 */
CF_API void CF_CALL cf_axis_binding_add_left_stick_x(CF_AxisBinding a, float threshold);

/**
 * @function cf_axis_binding_add_left_stick_y
 * @category binding
 * @brief    Binds the left stick Y axis.
 * @param    a              The axis binding handle.
 * @param    threshold      Deadzone threshold.
 * @related  CF_AxisBinding cf_axis_binding_add_left_stick_x cf_axis_binding_add_left_stick_y cf_axis_binding_add_right_stick_x cf_axis_binding_add_right_stick_y
 */
CF_API void CF_CALL cf_axis_binding_add_left_stick_y(CF_AxisBinding a, float threshold);

/**
 * @function cf_axis_binding_add_right_stick_x
 * @category binding
 * @brief    Binds the right stick X axis.
 * @param    a              The axis binding handle.
 * @param    threshold      Deadzone threshold.
 * @related  CF_AxisBinding cf_axis_binding_add_left_stick_x cf_axis_binding_add_left_stick_y cf_axis_binding_add_right_stick_x cf_axis_binding_add_right_stick_y
 */
CF_API void CF_CALL cf_axis_binding_add_right_stick_x(CF_AxisBinding a, float threshold);

/**
 * @function cf_axis_binding_add_right_stick_y
 * @category binding
 * @brief    Binds the right stick Y axis.
 * @param    a              The axis binding handle.
 * @param    threshold      Deadzone threshold.
 * @related  CF_AxisBinding cf_axis_binding_add_left_stick_x cf_axis_binding_add_left_stick_y cf_axis_binding_add_right_stick_x cf_axis_binding_add_right_stick_y
 */
CF_API void CF_CALL cf_axis_binding_add_right_stick_y(CF_AxisBinding a, float threshold);

/**
 * @function cf_axis_binding_set_conflict
 * @category binding
 * @brief    Sets the conflict resolution mode for when both directions are pressed.
 * @param    a              The axis binding handle.
 * @param    mode           The conflict resolution mode.
 * @related  CF_AxisBinding CF_AxisConflict cf_axis_binding_set_conflict
 */
CF_API void CF_CALL cf_axis_binding_set_conflict(CF_AxisBinding a, CF_AxisConflict mode);

/**
 * @function cf_axis_binding_pressed
 * @category binding
 * @brief    Returns true if either direction of the axis was pressed.
 * @param    a              The axis binding handle.
 * @related  CF_AxisBinding cf_axis_binding_pressed cf_axis_binding_released cf_axis_binding_value cf_axis_binding_sign
 */
CF_API bool CF_CALL cf_axis_binding_pressed(CF_AxisBinding a);

/**
 * @function cf_axis_binding_released
 * @category binding
 * @brief    Returns true if either direction of the axis was released.
 * @param    a              The axis binding handle.
 * @related  CF_AxisBinding cf_axis_binding_pressed cf_axis_binding_released cf_axis_binding_value cf_axis_binding_sign
 */
CF_API bool CF_CALL cf_axis_binding_released(CF_AxisBinding a);

/**
 * @function cf_axis_binding_value
 * @category binding
 * @brief    Returns the axis value from -1 to 1, with conflict resolution applied.
 * @param    a              The axis binding handle.
 * @related  CF_AxisBinding cf_axis_binding_pressed cf_axis_binding_released cf_axis_binding_value cf_axis_binding_sign
 */
CF_API float CF_CALL cf_axis_binding_value(CF_AxisBinding a);

/**
 * @function cf_axis_binding_sign
 * @category binding
 * @brief    Returns -1, 0, or 1 based on the axis value.
 * @param    a              The axis binding handle.
 * @related  CF_AxisBinding cf_axis_binding_value cf_axis_binding_sign
 */
CF_API float CF_CALL cf_axis_binding_sign(CF_AxisBinding a);

/**
 * @function cf_axis_binding_consume_press
 * @category binding
 * @brief    Consumes press events on both directions of the axis.
 * @param    a              The axis binding handle.
 * @return   Returns true if there was a press to consume.
 * @related  CF_AxisBinding cf_axis_binding_consume_press cf_axis_binding_consume_release
 */
CF_API bool CF_CALL cf_axis_binding_consume_press(CF_AxisBinding a);

/**
 * @function cf_axis_binding_consume_release
 * @category binding
 * @brief    Consumes release events on both directions of the axis.
 * @param    a              The axis binding handle.
 * @return   Returns true if there was a release to consume.
 * @related  CF_AxisBinding cf_axis_binding_consume_press cf_axis_binding_consume_release
 */
CF_API bool CF_CALL cf_axis_binding_consume_release(CF_AxisBinding a);

/**
 * @function cf_axis_binding_set_deadzone
 * @category binding
 * @brief    Sets a per-binding deadzone on both directions of the axis.
 * @param    a              The axis binding handle.
 * @param    deadzone       The deadzone value. Set to -1 to use the global deadzone.
 * @related  CF_AxisBinding cf_button_binding_set_deadzone cf_binding_set_deadzone
 */
CF_API void CF_CALL cf_axis_binding_set_deadzone(CF_AxisBinding a, float deadzone);

/**
 * @function cf_axis_binding_value_raw
 * @category binding
 * @brief    Returns the raw axis value (-1..1) with no deadzone applied.
 * @param    a              The axis binding handle.
 * @related  CF_AxisBinding cf_axis_binding_value cf_button_binding_value_raw cf_stick_binding_value_raw
 */
CF_API float CF_CALL cf_axis_binding_value_raw(CF_AxisBinding a);

//--------------------------------------------------------------------------------------------------
// StickBinding API.

/**
 * @function cf_make_stick_binding
 * @category binding
 * @brief    Creates a stick binding for the given player, combining X and Y axes.
 * @param    player_index   The joypad player index (0 for single player).
 * @return   Returns a `CF_StickBinding` handle.
 * @related  CF_StickBinding cf_make_stick_binding cf_destroy_stick_binding
 */
CF_API CF_StickBinding CF_CALL cf_make_stick_binding(int player_index);

/**
 * @function cf_destroy_stick_binding
 * @category binding
 * @brief    Destroys a stick binding and its internal axis bindings.
 * @param    s              The stick binding handle.
 * @related  CF_StickBinding cf_make_stick_binding cf_destroy_stick_binding
 */
CF_API void CF_CALL cf_destroy_stick_binding(CF_StickBinding s);

/**
 * @function cf_stick_binding_add_keys
 * @category binding
 * @brief    Binds four keys (up/down/left/right) to this stick.
 * @param    s              The stick binding handle.
 * @param    up             Key for up.
 * @param    down           Key for down.
 * @param    left           Key for left.
 * @param    right          Key for right.
 * @related  CF_StickBinding cf_stick_binding_add_keys cf_stick_binding_add_wasd cf_stick_binding_add_arrow_keys cf_stick_binding_add_dpad
 */
CF_API void CF_CALL cf_stick_binding_add_keys(CF_StickBinding s, CF_KeyButton up, CF_KeyButton down, CF_KeyButton left, CF_KeyButton right);

/**
 * @function cf_stick_binding_add_wasd
 * @category binding
 * @brief    Binds WASD keys to this stick.
 * @param    s              The stick binding handle.
 * @related  CF_StickBinding cf_stick_binding_add_keys cf_stick_binding_add_wasd cf_stick_binding_add_arrow_keys cf_stick_binding_add_dpad
 */
CF_API void CF_CALL cf_stick_binding_add_wasd(CF_StickBinding s);

/**
 * @function cf_stick_binding_add_arrow_keys
 * @category binding
 * @brief    Binds arrow keys to this stick.
 * @param    s              The stick binding handle.
 * @related  CF_StickBinding cf_stick_binding_add_keys cf_stick_binding_add_wasd cf_stick_binding_add_arrow_keys cf_stick_binding_add_dpad
 */
CF_API void CF_CALL cf_stick_binding_add_arrow_keys(CF_StickBinding s);

/**
 * @function cf_stick_binding_add_dpad
 * @category binding
 * @brief    Binds the joypad d-pad to this stick.
 * @param    s              The stick binding handle.
 * @related  CF_StickBinding cf_stick_binding_add_keys cf_stick_binding_add_wasd cf_stick_binding_add_arrow_keys cf_stick_binding_add_dpad
 */
CF_API void CF_CALL cf_stick_binding_add_dpad(CF_StickBinding s);

/**
 * @function cf_stick_binding_add_left_stick
 * @category binding
 * @brief    Binds the left analog stick to this stick.
 * @param    s              The stick binding handle.
 * @param    threshold      Deadzone threshold.
 * @related  CF_StickBinding cf_stick_binding_add_left_stick cf_stick_binding_add_right_stick
 */
CF_API void CF_CALL cf_stick_binding_add_left_stick(CF_StickBinding s, float threshold);

/**
 * @function cf_stick_binding_add_right_stick
 * @category binding
 * @brief    Binds the right analog stick to this stick.
 * @param    s              The stick binding handle.
 * @param    threshold      Deadzone threshold.
 * @related  CF_StickBinding cf_stick_binding_add_left_stick cf_stick_binding_add_right_stick
 */
CF_API void CF_CALL cf_stick_binding_add_right_stick(CF_StickBinding s, float threshold);

/**
 * @function cf_stick_binding_pressed
 * @category binding
 * @brief    Returns true if any direction of the stick was pressed.
 * @param    s              The stick binding handle.
 * @related  CF_StickBinding cf_stick_binding_pressed cf_stick_binding_released cf_stick_binding_value cf_stick_binding_sign
 */
CF_API bool CF_CALL cf_stick_binding_pressed(CF_StickBinding s);

/**
 * @function cf_stick_binding_released
 * @category binding
 * @brief    Returns true if any direction of the stick was released.
 * @param    s              The stick binding handle.
 * @related  CF_StickBinding cf_stick_binding_pressed cf_stick_binding_released cf_stick_binding_value cf_stick_binding_sign
 */
CF_API bool CF_CALL cf_stick_binding_released(CF_StickBinding s);

/**
 * @function cf_stick_binding_value
 * @category binding
 * @brief    Returns a 2D vector with x/y axis values, each from -1 to 1.
 * @param    s              The stick binding handle.
 * @related  CF_StickBinding cf_stick_binding_pressed cf_stick_binding_released cf_stick_binding_value cf_stick_binding_sign
 */
CF_API CF_V2 CF_CALL cf_stick_binding_value(CF_StickBinding s);

/**
 * @function cf_stick_binding_sign
 * @category binding
 * @brief    Returns a 2D vector where each component is -1, 0, or 1.
 * @param    s              The stick binding handle.
 * @related  CF_StickBinding cf_stick_binding_value cf_stick_binding_sign
 */
CF_API CF_V2 CF_CALL cf_stick_binding_sign(CF_StickBinding s);

/**
 * @function cf_stick_binding_consume_press
 * @category binding
 * @brief    Consumes press events on both axes of the stick.
 * @param    s              The stick binding handle.
 * @return   Returns true if there was a press to consume.
 * @related  CF_StickBinding cf_stick_binding_consume_press cf_stick_binding_consume_release
 */
CF_API bool CF_CALL cf_stick_binding_consume_press(CF_StickBinding s);

/**
 * @function cf_stick_binding_consume_release
 * @category binding
 * @brief    Consumes release events on both axes of the stick.
 * @param    s              The stick binding handle.
 * @return   Returns true if there was a release to consume.
 * @related  CF_StickBinding cf_stick_binding_consume_press cf_stick_binding_consume_release
 */
CF_API bool CF_CALL cf_stick_binding_consume_release(CF_StickBinding s);

/**
 * @function cf_stick_binding_set_circular_deadzone
 * @category binding
 * @brief    Sets a circular deadzone on the stick, applied to the combined x/y magnitude.
 * @param    s              The stick binding handle.
 * @param    deadzone       The circular deadzone radius (0..1). Set to -1 to disable.
 * @remarks  When active, both underlying axis deadzones are set to 0 so analog values pass through for the circular calculation.
 *           Digital inputs (keys, dpad) override the analog value when active.
 * @related  CF_StickBinding cf_stick_binding_value cf_binding_set_deadzone
 */
CF_API void CF_CALL cf_stick_binding_set_circular_deadzone(CF_StickBinding s, float deadzone);

/**
 * @function cf_stick_binding_value_raw
 * @category binding
 * @brief    Returns the raw 2D stick value with no deadzone applied.
 * @param    s              The stick binding handle.
 * @related  CF_StickBinding cf_stick_binding_value cf_button_binding_value_raw cf_axis_binding_value_raw
 */
CF_API CF_V2 CF_CALL cf_stick_binding_value_raw(CF_StickBinding s);

//--------------------------------------------------------------------------------------------------
// Joypad connection tracking.

/**
 * @function cf_register_joypad_connect_callback
 * @category binding
 * @brief    Registers a callback invoked when a joypad is connected or disconnected.
 * @param    fn             Callback function: player_index, connected (true=connect, false=disconnect), udata.
 * @param    udata          User data passed to the callback.
 * @related  cf_register_joypad_connect_callback
 */
CF_API void CF_CALL cf_register_joypad_connect_callback(void (*fn)(int player_index, bool connected, void* udata), void* udata);

//--------------------------------------------------------------------------------------------------
// Deadzone.

/**
 * @function cf_binding_get_deadzone
 * @category binding
 * @brief    Returns the global deadzone threshold for analog stick inputs.
 * @remarks  Defaults to 0.15f. Values below this threshold are zeroed out. Valid range is roughly 0.1 to 0.2.
 *           Individual bindings can override this with `cf_button_binding_set_deadzone` or `cf_axis_binding_set_deadzone`.
 * @related  cf_binding_get_deadzone cf_binding_set_deadzone cf_button_binding_set_deadzone cf_axis_binding_set_deadzone
 */
CF_API float CF_CALL cf_binding_get_deadzone();

/**
 * @function cf_binding_set_deadzone
 * @category binding
 * @brief    Sets the global deadzone threshold for analog stick inputs.
 * @param    deadzone       The deadzone value (roughly 0.1 to 0.2). Default is 0.15f.
 * @remarks  Inputs from controllers below this threshold are zeroed out. Setting too low causes stick drift, too high reduces sensitivity.
 *           Individual bindings can override this with `cf_button_binding_set_deadzone` or `cf_axis_binding_set_deadzone`.
 * @related  cf_binding_get_deadzone cf_binding_set_deadzone cf_button_binding_set_deadzone cf_axis_binding_set_deadzone
 */
CF_API void CF_CALL cf_binding_set_deadzone(float deadzone);

//--------------------------------------------------------------------------------------------------
// _Generic dispatch macros (C11).

#ifndef __cplusplus

/**
 * @function cf_binding_pressed
 * @category binding
 * @brief    Returns true if the binding was pressed (with buffering), dispatching by handle type.
 * @param    x              A `CF_ButtonBinding`, `CF_AxisBinding`, or `CF_StickBinding` handle.
 * @remarks  This is a C11 `_Generic` macro. In C++ use the overloaded `binding_pressed()` functions in `namespace Cute`.
 * @related  cf_button_binding_pressed cf_axis_binding_pressed cf_stick_binding_pressed cf_binding_released cf_binding_value cf_binding_sign
 */
#define cf_binding_pressed(x) _Generic((x), \
	CF_ButtonBinding: cf_button_binding_pressed, \
	CF_AxisBinding: cf_axis_binding_pressed, \
	CF_StickBinding: cf_stick_binding_pressed \
)(x)

/**
 * @function cf_binding_released
 * @category binding
 * @brief    Returns true if the binding was released (with buffering), dispatching by handle type.
 * @param    x              A `CF_ButtonBinding`, `CF_AxisBinding`, or `CF_StickBinding` handle.
 * @remarks  This is a C11 `_Generic` macro. In C++ use the overloaded `binding_released()` functions in `namespace Cute`.
 * @related  cf_button_binding_released cf_axis_binding_released cf_stick_binding_released cf_binding_pressed cf_binding_value cf_binding_sign
 */
#define cf_binding_released(x) _Generic((x), \
	CF_ButtonBinding: cf_button_binding_released, \
	CF_AxisBinding: cf_axis_binding_released, \
	CF_StickBinding: cf_stick_binding_released \
)(x)

/**
 * @function cf_binding_value
 * @category binding
 * @brief    Returns the analog value of the binding, dispatching by handle type.
 * @param    x              A `CF_ButtonBinding`, `CF_AxisBinding`, or `CF_StickBinding` handle.
 * @remarks  This is a C11 `_Generic` macro. Returns `float` for button/axis, `CF_V2` for stick. In C++ use the overloaded `binding_value()` functions in `namespace Cute`.
 * @related  cf_button_binding_value cf_axis_binding_value cf_stick_binding_value cf_binding_pressed cf_binding_released cf_binding_sign
 */
#define cf_binding_value(x) _Generic((x), \
	CF_ButtonBinding: cf_button_binding_value, \
	CF_AxisBinding: cf_axis_binding_value, \
	CF_StickBinding: cf_stick_binding_value \
)(x)

/**
 * @function cf_binding_sign
 * @category binding
 * @brief    Returns the sign of the binding value, dispatching by handle type.
 * @param    x              A `CF_ButtonBinding`, `CF_AxisBinding`, or `CF_StickBinding` handle.
 * @remarks  This is a C11 `_Generic` macro. Returns `float` (-1/0/1) for button/axis, `CF_V2` for stick. In C++ use the overloaded `binding_sign()` functions in `namespace Cute`.
 * @related  cf_button_binding_sign cf_axis_binding_sign cf_stick_binding_sign cf_binding_pressed cf_binding_released cf_binding_value
 */
#define cf_binding_sign(x) _Generic((x), \
	CF_ButtonBinding: cf_button_binding_sign, \
	CF_AxisBinding: cf_axis_binding_sign, \
	CF_StickBinding: cf_stick_binding_sign \
)(x)

/**
 * @function cf_binding_consume_press
 * @category binding
 * @brief    Consumes the press event on the binding, dispatching by handle type.
 * @param    x              A `CF_ButtonBinding`, `CF_AxisBinding`, or `CF_StickBinding` handle.
 * @remarks  This is a C11 `_Generic` macro. In C++ use the overloaded `binding_consume_press()` functions in `namespace Cute`.
 * @related  cf_button_binding_consume_press cf_axis_binding_consume_press cf_stick_binding_consume_press cf_binding_consume_release
 */
#define cf_binding_consume_press(x) _Generic((x), \
	CF_ButtonBinding: cf_button_binding_consume_press, \
	CF_AxisBinding: cf_axis_binding_consume_press, \
	CF_StickBinding: cf_stick_binding_consume_press \
)(x)

/**
 * @function cf_binding_consume_release
 * @category binding
 * @brief    Consumes the release event on the binding, dispatching by handle type.
 * @param    x              A `CF_ButtonBinding`, `CF_AxisBinding`, or `CF_StickBinding` handle.
 * @remarks  This is a C11 `_Generic` macro. In C++ use the overloaded `binding_consume_release()` functions in `namespace Cute`.
 * @related  cf_button_binding_consume_release cf_axis_binding_consume_release cf_stick_binding_consume_release cf_binding_consume_press
 */
#define cf_binding_consume_release(x) _Generic((x), \
	CF_ButtonBinding: cf_button_binding_consume_release, \
	CF_AxisBinding: cf_axis_binding_consume_release, \
	CF_StickBinding: cf_stick_binding_consume_release \
)(x)

/**
 * @function cf_binding_value_raw
 * @category binding
 * @brief    Returns the raw value of the binding (no deadzone), dispatching by handle type.
 * @param    x              A `CF_ButtonBinding`, `CF_AxisBinding`, or `CF_StickBinding` handle.
 * @remarks  This is a C11 `_Generic` macro. Returns `float` for button/axis, `CF_V2` for stick.
 * @related  cf_button_binding_value_raw cf_axis_binding_value_raw cf_stick_binding_value_raw cf_binding_value
 */
#define cf_binding_value_raw(x) _Generic((x), \
	CF_ButtonBinding: cf_button_binding_value_raw, \
	CF_AxisBinding: cf_axis_binding_value_raw, \
	CF_StickBinding: cf_stick_binding_value_raw \
)(x)

/**
 * @function cf_binding_set_deadzone_per
 * @category binding
 * @brief    Sets a per-binding deadzone, dispatching by handle type (button or axis).
 * @param    x              A `CF_ButtonBinding` or `CF_AxisBinding` handle.
 * @param    dz             The deadzone value. Set to -1 to use the global deadzone.
 * @remarks  This is a C11 `_Generic` macro. For stick bindings use `cf_stick_binding_set_circular_deadzone` instead.
 * @related  cf_button_binding_set_deadzone cf_axis_binding_set_deadzone cf_stick_binding_set_circular_deadzone
 */
#define cf_binding_set_deadzone_per(x, dz) _Generic((x), \
	CF_ButtonBinding: cf_button_binding_set_deadzone, \
	CF_AxisBinding: cf_axis_binding_set_deadzone \
)(x, dz)

/**
 * @function cf_destroy_binding
 * @category binding
 * @brief    Destroys a binding and frees its resources, dispatching by handle type.
 * @param    x              A `CF_ButtonBinding`, `CF_AxisBinding`, or `CF_StickBinding` handle.
 * @remarks  This is a C11 `_Generic` macro. In C++ use the overloaded `destroy_binding()` functions in `namespace Cute`.
 * @related  cf_destroy_button_binding cf_destroy_axis_binding cf_destroy_stick_binding cf_make_button_binding cf_make_axis_binding cf_make_stick_binding
 */
#define cf_destroy_binding(x) _Generic((x), \
	CF_ButtonBinding: cf_destroy_button_binding, \
	CF_AxisBinding: cf_destroy_axis_binding, \
	CF_StickBinding: cf_destroy_stick_binding \
)(x)

#endif // !__cplusplus

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

CF_INLINE CF_ButtonBinding make_button_binding(int player_index, float press_buffer) { return cf_make_button_binding(player_index, press_buffer); }
CF_INLINE void destroy_button_binding(CF_ButtonBinding b) { cf_destroy_button_binding(b); }
CF_INLINE void button_binding_add_key(CF_ButtonBinding b, CF_KeyButton key) { cf_button_binding_add_key(b, key); }
CF_INLINE void button_binding_add_mouse_button(CF_ButtonBinding b, CF_MouseButton button) { cf_button_binding_add_mouse_button(b, button); }
CF_INLINE void button_binding_add_joypad_button(CF_ButtonBinding b, CF_JoypadButton button) { cf_button_binding_add_joypad_button(b, button); }
CF_INLINE void button_binding_add_trigger(CF_ButtonBinding b, CF_JoypadAxis axis, float threshold, bool positive) { cf_button_binding_add_trigger(b, axis, threshold, positive); }
CF_INLINE bool button_binding_pressed(CF_ButtonBinding b) { return cf_button_binding_pressed(b); }
CF_INLINE bool button_binding_released(CF_ButtonBinding b) { return cf_button_binding_released(b); }
CF_INLINE bool button_binding_down(CF_ButtonBinding b) { return cf_button_binding_down(b); }
CF_INLINE float button_binding_value(CF_ButtonBinding b) { return cf_button_binding_value(b); }
CF_INLINE float button_binding_sign(CF_ButtonBinding b) { return cf_button_binding_sign(b); }
CF_INLINE bool button_binding_consume_press(CF_ButtonBinding b) { return cf_button_binding_consume_press(b); }
CF_INLINE bool button_binding_consume_release(CF_ButtonBinding b) { return cf_button_binding_consume_release(b); }
CF_INLINE void button_binding_set_deadzone(CF_ButtonBinding b, float deadzone) { cf_button_binding_set_deadzone(b, deadzone); }
CF_INLINE float button_binding_value_raw(CF_ButtonBinding b) { return cf_button_binding_value_raw(b); }
CF_INLINE void button_binding_set_repeat(CF_ButtonBinding b, float delay, float interval) { cf_button_binding_set_repeat(b, delay, interval); }
CF_INLINE bool button_binding_repeated(CF_ButtonBinding b) { return cf_button_binding_repeated(b); }

CF_INLINE CF_AxisBinding make_axis_binding(int player_index) { return cf_make_axis_binding(player_index); }
CF_INLINE void destroy_axis_binding(CF_AxisBinding a) { cf_destroy_axis_binding(a); }
CF_INLINE void axis_binding_add_keys(CF_AxisBinding a, CF_KeyButton negative, CF_KeyButton positive) { cf_axis_binding_add_keys(a, negative, positive); }
CF_INLINE void axis_binding_add_mouse_buttons(CF_AxisBinding a, CF_MouseButton negative, CF_MouseButton positive) { cf_axis_binding_add_mouse_buttons(a, negative, positive); }
CF_INLINE void axis_binding_add_joypad_buttons(CF_AxisBinding a, CF_JoypadButton negative, CF_JoypadButton positive) { cf_axis_binding_add_joypad_buttons(a, negative, positive); }
CF_INLINE void axis_binding_add_triggers(CF_AxisBinding a, CF_JoypadAxis negative, CF_JoypadAxis positive, float threshold) { cf_axis_binding_add_triggers(a, negative, positive, threshold); }
CF_INLINE void axis_binding_add_left_stick_x(CF_AxisBinding a, float threshold) { cf_axis_binding_add_left_stick_x(a, threshold); }
CF_INLINE void axis_binding_add_left_stick_y(CF_AxisBinding a, float threshold) { cf_axis_binding_add_left_stick_y(a, threshold); }
CF_INLINE void axis_binding_add_right_stick_x(CF_AxisBinding a, float threshold) { cf_axis_binding_add_right_stick_x(a, threshold); }
CF_INLINE void axis_binding_add_right_stick_y(CF_AxisBinding a, float threshold) { cf_axis_binding_add_right_stick_y(a, threshold); }
CF_INLINE void axis_binding_set_conflict(CF_AxisBinding a, CF_AxisConflict mode) { cf_axis_binding_set_conflict(a, mode); }
CF_INLINE bool axis_binding_pressed(CF_AxisBinding a) { return cf_axis_binding_pressed(a); }
CF_INLINE bool axis_binding_released(CF_AxisBinding a) { return cf_axis_binding_released(a); }
CF_INLINE float axis_binding_value(CF_AxisBinding a) { return cf_axis_binding_value(a); }
CF_INLINE float axis_binding_sign(CF_AxisBinding a) { return cf_axis_binding_sign(a); }
CF_INLINE bool axis_binding_consume_press(CF_AxisBinding a) { return cf_axis_binding_consume_press(a); }
CF_INLINE bool axis_binding_consume_release(CF_AxisBinding a) { return cf_axis_binding_consume_release(a); }
CF_INLINE void axis_binding_set_deadzone(CF_AxisBinding a, float deadzone) { cf_axis_binding_set_deadzone(a, deadzone); }
CF_INLINE float axis_binding_value_raw(CF_AxisBinding a) { return cf_axis_binding_value_raw(a); }

CF_INLINE CF_StickBinding make_stick_binding(int player_index) { return cf_make_stick_binding(player_index); }
CF_INLINE void destroy_stick_binding(CF_StickBinding s) { cf_destroy_stick_binding(s); }
CF_INLINE void stick_binding_add_keys(CF_StickBinding s, CF_KeyButton up, CF_KeyButton down, CF_KeyButton left, CF_KeyButton right) { cf_stick_binding_add_keys(s, up, down, left, right); }
CF_INLINE void stick_binding_add_wasd(CF_StickBinding s) { cf_stick_binding_add_wasd(s); }
CF_INLINE void stick_binding_add_arrow_keys(CF_StickBinding s) { cf_stick_binding_add_arrow_keys(s); }
CF_INLINE void stick_binding_add_dpad(CF_StickBinding s) { cf_stick_binding_add_dpad(s); }
CF_INLINE void stick_binding_add_left_stick(CF_StickBinding s, float threshold) { cf_stick_binding_add_left_stick(s, threshold); }
CF_INLINE void stick_binding_add_right_stick(CF_StickBinding s, float threshold) { cf_stick_binding_add_right_stick(s, threshold); }
CF_INLINE bool stick_binding_pressed(CF_StickBinding s) { return cf_stick_binding_pressed(s); }
CF_INLINE bool stick_binding_released(CF_StickBinding s) { return cf_stick_binding_released(s); }
CF_INLINE CF_V2 stick_binding_value(CF_StickBinding s) { return cf_stick_binding_value(s); }
CF_INLINE CF_V2 stick_binding_sign(CF_StickBinding s) { return cf_stick_binding_sign(s); }
CF_INLINE bool stick_binding_consume_press(CF_StickBinding s) { return cf_stick_binding_consume_press(s); }
CF_INLINE bool stick_binding_consume_release(CF_StickBinding s) { return cf_stick_binding_consume_release(s); }
CF_INLINE void stick_binding_set_circular_deadzone(CF_StickBinding s, float deadzone) { cf_stick_binding_set_circular_deadzone(s, deadzone); }
CF_INLINE CF_V2 stick_binding_value_raw(CF_StickBinding s) { return cf_stick_binding_value_raw(s); }

CF_INLINE void register_joypad_connect_callback(void (*fn)(int player_index, bool connected, void* udata), void* udata) { cf_register_joypad_connect_callback(fn, udata); }

CF_INLINE float binding_get_deadzone() { return cf_binding_get_deadzone(); }
CF_INLINE void binding_set_deadzone(float deadzone) { cf_binding_set_deadzone(deadzone); }

// Overloaded dispatch functions.
CF_INLINE bool binding_pressed(CF_ButtonBinding b) { return cf_button_binding_pressed(b); }
CF_INLINE bool binding_pressed(CF_AxisBinding a) { return cf_axis_binding_pressed(a); }
CF_INLINE bool binding_pressed(CF_StickBinding s) { return cf_stick_binding_pressed(s); }

CF_INLINE bool binding_released(CF_ButtonBinding b) { return cf_button_binding_released(b); }
CF_INLINE bool binding_released(CF_AxisBinding a) { return cf_axis_binding_released(a); }
CF_INLINE bool binding_released(CF_StickBinding s) { return cf_stick_binding_released(s); }

CF_INLINE float binding_value(CF_ButtonBinding b) { return cf_button_binding_value(b); }
CF_INLINE float binding_value(CF_AxisBinding a) { return cf_axis_binding_value(a); }
CF_INLINE CF_V2 binding_value(CF_StickBinding s) { return cf_stick_binding_value(s); }

CF_INLINE float binding_sign(CF_ButtonBinding b) { return cf_button_binding_sign(b); }
CF_INLINE float binding_sign(CF_AxisBinding a) { return cf_axis_binding_sign(a); }
CF_INLINE CF_V2 binding_sign(CF_StickBinding s) { return cf_stick_binding_sign(s); }

CF_INLINE bool binding_consume_press(CF_ButtonBinding b) { return cf_button_binding_consume_press(b); }
CF_INLINE bool binding_consume_press(CF_AxisBinding a) { return cf_axis_binding_consume_press(a); }
CF_INLINE bool binding_consume_press(CF_StickBinding s) { return cf_stick_binding_consume_press(s); }

CF_INLINE bool binding_consume_release(CF_ButtonBinding b) { return cf_button_binding_consume_release(b); }
CF_INLINE bool binding_consume_release(CF_AxisBinding a) { return cf_axis_binding_consume_release(a); }
CF_INLINE bool binding_consume_release(CF_StickBinding s) { return cf_stick_binding_consume_release(s); }

CF_INLINE float binding_value_raw(CF_ButtonBinding b) { return cf_button_binding_value_raw(b); }
CF_INLINE float binding_value_raw(CF_AxisBinding a) { return cf_axis_binding_value_raw(a); }
CF_INLINE CF_V2 binding_value_raw(CF_StickBinding s) { return cf_stick_binding_value_raw(s); }

CF_INLINE void binding_set_deadzone(CF_ButtonBinding b, float deadzone) { cf_button_binding_set_deadzone(b, deadzone); }
CF_INLINE void binding_set_deadzone(CF_AxisBinding a, float deadzone) { cf_axis_binding_set_deadzone(a, deadzone); }

CF_INLINE void destroy_binding(CF_ButtonBinding b) { cf_destroy_button_binding(b); }
CF_INLINE void destroy_binding(CF_AxisBinding a) { cf_destroy_axis_binding(a); }
CF_INLINE void destroy_binding(CF_StickBinding s) { cf_destroy_stick_binding(s); }

}

#endif // CF_CPP

#endif // CF_BINDING_H
