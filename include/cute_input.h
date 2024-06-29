/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_INPUT_H
#define CF_INPUT_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @enum     CF_MouseButton
 * @category input
 * @brief    The mouse buttons.
 * @related  CF_MouseButton cf_mouse_button_to_string cf_mouse_down
 */
#define CF_MOUSE_BUTTON_DEFS \
	/* @entry */ \
	CF_ENUM(MOUSE_BUTTON_LEFT, 0) \
	/* @entry */ \
	CF_ENUM(MOUSE_BUTTON_RIGHT, 1) \
	/* @entry */ \
	CF_ENUM(MOUSE_BUTTON_MIDDLE, 2) \
	/* @end */

typedef enum CF_MouseButton
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_MOUSE_BUTTON_DEFS
	#undef CF_ENUM
} CF_MouseButton;

/**
 * @function cf_mouse_button_to_string
 * @category input
 * @brief    Convert an enum `CF_MouseButton` to a c-style string.
 * @param    state        The state to convert to a string.
 * @related  CF_MouseButton cf_mouse_button_to_string cf_mouse_down
 */
CF_INLINE const char* cf_mouse_button_to_string(CF_MouseButton button)
{
	switch (button) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_MOUSE_BUTTON_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @enum     CF_KeyButton
 * @category input
 * @brief    The keys.
 * @related  CF_KeyButton cf_key_button_to_string cf_key_down
 */
#define CF_KEY_BUTTON_DEFS \
	/* @entry */ \
	CF_ENUM(KEY_UNKNOWN, 0) \
	/* @entry */ \
	CF_ENUM(KEY_RETURN, 13) \
	/* @entry */ \
	CF_ENUM(KEY_ESCAPE, '\033') \
	/* @entry */ \
	CF_ENUM(KEY_BACKSPACE, '\b') \
	/* @entry */ \
	CF_ENUM(KEY_TAB, '\t') \
	/* @entry */ \
	CF_ENUM(KEY_SPACE, ' ') \
	/* @entry */ \
	CF_ENUM(KEY_EXCLAIM, '!') \
	/* @entry */ \
	CF_ENUM(KEY_QUOTEDBL, '"') \
	/* @entry */ \
	CF_ENUM(KEY_HASH, '#') \
	/* @entry */ \
	CF_ENUM(KEY_PERCENT, '%') \
	/* @entry */ \
	CF_ENUM(KEY_DOLLAR, '$') \
	/* @entry */ \
	CF_ENUM(KEY_AMPERSAND, '&') \
	/* @entry */ \
	CF_ENUM(KEY_QUOTE, '\'') \
	/* @entry */ \
	CF_ENUM(KEY_LEFTPAREN, '(') \
	/* @entry */ \
	CF_ENUM(KEY_RIGHTPAREN, ')') \
	/* @entry */ \
	CF_ENUM(KEY_ASTERISK, '*') \
	/* @entry */ \
	CF_ENUM(KEY_PLUS, '+') \
	/* @entry */ \
	CF_ENUM(KEY_COMMA, ',') \
	/* @entry */ \
	CF_ENUM(KEY_MINUS, '-') \
	/* @entry */ \
	CF_ENUM(KEY_PERIOD, '.') \
	/* @entry */ \
	CF_ENUM(KEY_SLASH, '/') \
	/* @entry */ \
	CF_ENUM(KEY_0, '0') \
	/* @entry */ \
	CF_ENUM(KEY_1, '1') \
	/* @entry */ \
	CF_ENUM(KEY_2, '2') \
	/* @entry */ \
	CF_ENUM(KEY_3, '3') \
	/* @entry */ \
	CF_ENUM(KEY_4, '4') \
	/* @entry */ \
	CF_ENUM(KEY_5, '5') \
	/* @entry */ \
	CF_ENUM(KEY_6, '6') \
	/* @entry */ \
	CF_ENUM(KEY_7, '7') \
	/* @entry */ \
	CF_ENUM(KEY_8, '8') \
	/* @entry */ \
	CF_ENUM(KEY_9, '9') \
	/* @entry */ \
	CF_ENUM(KEY_COLON, ':') \
	/* @entry */ \
	CF_ENUM(KEY_SEMICOLON, ';') \
	/* @entry */ \
	CF_ENUM(KEY_LESS, '<') \
	/* @entry */ \
	CF_ENUM(KEY_EQUALS, '=') \
	/* @entry */ \
	CF_ENUM(KEY_GREATER, '>') \
	/* @entry */ \
	CF_ENUM(KEY_QUESTION, '?') \
	/* @entry */ \
	CF_ENUM(KEY_AT, '@') \
	/* @entry */ \
	CF_ENUM(KEY_LEFTBRACKET, '[') \
	/* @entry */ \
	CF_ENUM(KEY_BACKSLASH, '\\') \
	/* @entry */ \
	CF_ENUM(KEY_RIGHTBRACKET, ']') \
	/* @entry */ \
	CF_ENUM(KEY_CARET, '^') \
	/* @entry */ \
	CF_ENUM(KEY_UNDERSCORE, '_') \
	/* @entry */ \
	CF_ENUM(KEY_BACKQUOTE, '`') \
	/* @entry */ \
	CF_ENUM(KEY_A, 'a') \
	/* @entry */ \
	CF_ENUM(KEY_B, 'b') \
	/* @entry */ \
	CF_ENUM(KEY_C, 'c') \
	/* @entry */ \
	CF_ENUM(KEY_D, 'd') \
	/* @entry */ \
	CF_ENUM(KEY_E, 'e') \
	/* @entry */ \
	CF_ENUM(KEY_F, 'f') \
	/* @entry */ \
	CF_ENUM(KEY_G, 'g') \
	/* @entry */ \
	CF_ENUM(KEY_H, 'h') \
	/* @entry */ \
	CF_ENUM(KEY_I, 'i') \
	/* @entry */ \
	CF_ENUM(KEY_J, 'j') \
	/* @entry */ \
	CF_ENUM(KEY_K, 'k') \
	/* @entry */ \
	CF_ENUM(KEY_L, 'l') \
	/* @entry */ \
	CF_ENUM(KEY_M, 'm') \
	/* @entry */ \
	CF_ENUM(KEY_N, 'n') \
	/* @entry */ \
	CF_ENUM(KEY_O, 'o') \
	/* @entry */ \
	CF_ENUM(KEY_P, 'p') \
	/* @entry */ \
	CF_ENUM(KEY_Q, 'q') \
	/* @entry */ \
	CF_ENUM(KEY_R, 'r') \
	/* @entry */ \
	CF_ENUM(KEY_S, 's') \
	/* @entry */ \
	CF_ENUM(KEY_T, 't') \
	/* @entry */ \
	CF_ENUM(KEY_U, 'u') \
	/* @entry */ \
	CF_ENUM(KEY_V, 'v') \
	/* @entry */ \
	CF_ENUM(KEY_W, 'w') \
	/* @entry */ \
	CF_ENUM(KEY_X, 'x') \
	/* @entry */ \
	CF_ENUM(KEY_Y, 'y') \
	/* @entry */ \
	CF_ENUM(KEY_Z, 'z') \
	/* @entry */ \
	CF_ENUM(KEY_CAPSLOCK, 123) \
	/* @entry */ \
	CF_ENUM(KEY_F1, 124) \
	/* @entry */ \
	CF_ENUM(KEY_F2, 125) \
	/* @entry */ \
	CF_ENUM(KEY_F3, 126) \
	/* @entry */ \
	CF_ENUM(KEY_F4, 127) \
	/* @entry */ \
	CF_ENUM(KEY_F5, 128) \
	/* @entry */ \
	CF_ENUM(KEY_F6, 129) \
	/* @entry */ \
	CF_ENUM(KEY_F7, 130) \
	/* @entry */ \
	CF_ENUM(KEY_F8, 131) \
	/* @entry */ \
	CF_ENUM(KEY_F9, 132) \
	/* @entry */ \
	CF_ENUM(KEY_F10, 133) \
	/* @entry */ \
	CF_ENUM(KEY_F11, 134) \
	/* @entry */ \
	CF_ENUM(KEY_F12, 135) \
	/* @entry */ \
	CF_ENUM(KEY_PRINTSCREEN, 136) \
	/* @entry */ \
	CF_ENUM(KEY_SCROLLLOCK, 137) \
	/* @entry */ \
	CF_ENUM(KEY_PAUSE, 138) \
	/* @entry */ \
	CF_ENUM(KEY_INSERT, 139) \
	/* @entry */ \
	CF_ENUM(KEY_HOME, 140) \
	/* @entry */ \
	CF_ENUM(KEY_PAGEUP, 141) \
	/* @entry */ \
	CF_ENUM(KEY_DELETE, 142) \
	/* @entry */ \
	CF_ENUM(KEY_END, 143) \
	/* @entry */ \
	CF_ENUM(KEY_PAGEDOWN, 144) \
	/* @entry */ \
	CF_ENUM(KEY_RIGHT, 145) \
	/* @entry */ \
	CF_ENUM(KEY_LEFT, 146) \
	/* @entry */ \
	CF_ENUM(KEY_DOWN, 147) \
	/* @entry */ \
	CF_ENUM(KEY_UP, 148) \
	/* @entry */ \
	CF_ENUM(KEY_NUMLOCKCLEAR, 149) \
	/* @entry */ \
	CF_ENUM(KEY_KP_DIVIDE, 150) \
	/* @entry */ \
	CF_ENUM(KEY_KP_MULTIPLY, 151) \
	/* @entry */ \
	CF_ENUM(KEY_KP_MINUS, 152) \
	/* @entry */ \
	CF_ENUM(KEY_KP_PLUS, 153) \
	/* @entry */ \
	CF_ENUM(KEY_KP_ENTER, 154) \
	/* @entry */ \
	CF_ENUM(KEY_KP_1, 155) \
	/* @entry */ \
	CF_ENUM(KEY_KP_2, 156) \
	/* @entry */ \
	CF_ENUM(KEY_KP_3, 157) \
	/* @entry */ \
	CF_ENUM(KEY_KP_4, 158) \
	/* @entry */ \
	CF_ENUM(KEY_KP_5, 159) \
	/* @entry */ \
	CF_ENUM(KEY_KP_6, 160) \
	/* @entry */ \
	CF_ENUM(KEY_KP_7, 161) \
	/* @entry */ \
	CF_ENUM(KEY_KP_8, 162) \
	/* @entry */ \
	CF_ENUM(KEY_KP_9, 163) \
	/* @entry */ \
	CF_ENUM(KEY_KP_0, 164) \
	/* @entry */ \
	CF_ENUM(KEY_KP_PERIOD, 165) \
	/* @entry */ \
	CF_ENUM(KEY_APPLICATION, 166) \
	/* @entry */ \
	CF_ENUM(KEY_POWER, 167) \
	/* @entry */ \
	CF_ENUM(KEY_KP_EQUALS, 168) \
	/* @entry */ \
	CF_ENUM(KEY_F13, 169) \
	/* @entry */ \
	CF_ENUM(KEY_F14, 170) \
	/* @entry */ \
	CF_ENUM(KEY_F15, 171) \
	/* @entry */ \
	CF_ENUM(KEY_F16, 172) \
	/* @entry */ \
	CF_ENUM(KEY_F17, 173) \
	/* @entry */ \
	CF_ENUM(KEY_F18, 174) \
	/* @entry */ \
	CF_ENUM(KEY_F19, 175) \
	/* @entry */ \
	CF_ENUM(KEY_F20, 176) \
	/* @entry */ \
	CF_ENUM(KEY_F21, 177) \
	/* @entry */ \
	CF_ENUM(KEY_F22, 178) \
	/* @entry */ \
	CF_ENUM(KEY_F23, 179) \
	/* @entry */ \
	CF_ENUM(KEY_F24, 180) \
	/* @entry */ \
	CF_ENUM(KEY_HELP, 181) \
	/* @entry */ \
	CF_ENUM(KEY_MENU, 182) \
	/* @entry */ \
	CF_ENUM(KEY_SELECT, 183) \
	/* @entry */ \
	CF_ENUM(KEY_STOP, 184) \
	/* @entry */ \
	CF_ENUM(KEY_AGAIN, 185) \
	/* @entry */ \
	CF_ENUM(KEY_UNDO, 186) \
	/* @entry */ \
	CF_ENUM(KEY_CUT, 187) \
	/* @entry */ \
	CF_ENUM(KEY_COPY, 188) \
	/* @entry */ \
	CF_ENUM(KEY_PASTE, 189) \
	/* @entry */ \
	CF_ENUM(KEY_FIND, 190) \
	/* @entry */ \
	CF_ENUM(KEY_MUTE, 191) \
	/* @entry */ \
	CF_ENUM(KEY_VOLUMEUP, 192) \
	/* @entry */ \
	CF_ENUM(KEY_VOLUMEDOWN, 193) \
	/* @entry */ \
	CF_ENUM(KEY_KP_COMMA, 194) \
	/* @entry */ \
	CF_ENUM(KEY_KP_EQUALSAS400, 195) \
	/* @entry */ \
	CF_ENUM(KEY_ALTERASE, 196) \
	/* @entry */ \
	CF_ENUM(KEY_SYSREQ, 197) \
	/* @entry */ \
	CF_ENUM(KEY_CANCEL, 198) \
	/* @entry */ \
	CF_ENUM(KEY_CLEAR, 199) \
	/* @entry */ \
	CF_ENUM(KEY_PRIOR, 200) \
	/* @entry */ \
	CF_ENUM(KEY_RETURN2, 201) \
	/* @entry */ \
	CF_ENUM(KEY_SEPARATOR, 202) \
	/* @entry */ \
	CF_ENUM(KEY_OUT, 203) \
	/* @entry */ \
	CF_ENUM(KEY_OPER, 204) \
	/* @entry */ \
	CF_ENUM(KEY_CLEARAGAIN, 205) \
	/* @entry */ \
	CF_ENUM(KEY_CRSEL, 206) \
	/* @entry */ \
	CF_ENUM(KEY_EXSEL, 207) \
	/* @entry */ \
	CF_ENUM(KEY_KP_00, 208) \
	/* @entry */ \
	CF_ENUM(KEY_KP_000, 209) \
	/* @entry */ \
	CF_ENUM(KEY_THOUSANDSSEPARATOR, 210) \
	/* @entry */ \
	CF_ENUM(KEY_DECIMALSEPARATOR, 211) \
	/* @entry */ \
	CF_ENUM(KEY_CURRENCYUNIT, 212) \
	/* @entry */ \
	CF_ENUM(KEY_CURRENCYSUBUNIT, 213) \
	/* @entry */ \
	CF_ENUM(KEY_KP_LEFTPAREN, 214) \
	/* @entry */ \
	CF_ENUM(KEY_KP_RIGHTPAREN, 215) \
	/* @entry */ \
	CF_ENUM(KEY_KP_LEFTBRACE, 216) \
	/* @entry */ \
	CF_ENUM(KEY_KP_RIGHTBRACE, 217) \
	/* @entry */ \
	CF_ENUM(KEY_KP_TAB, 218) \
	/* @entry */ \
	CF_ENUM(KEY_KP_BACKSPACE, 219) \
	/* @entry */ \
	CF_ENUM(KEY_KP_A, 220) \
	/* @entry */ \
	CF_ENUM(KEY_KP_B, 221) \
	/* @entry */ \
	CF_ENUM(KEY_KP_C, 222) \
	/* @entry */ \
	CF_ENUM(KEY_KP_D, 223) \
	/* @entry */ \
	CF_ENUM(KEY_KP_E, 224) \
	/* @entry */ \
	CF_ENUM(KEY_KP_F, 225) \
	/* @entry */ \
	CF_ENUM(KEY_KP_XOR, 226) \
	/* @entry */ \
	CF_ENUM(KEY_KP_POWER, 227) \
	/* @entry */ \
	CF_ENUM(KEY_KP_PERCENT, 228) \
	/* @entry */ \
	CF_ENUM(KEY_KP_LESS, 229) \
	/* @entry */ \
	CF_ENUM(KEY_KP_GREATER, 230) \
	/* @entry */ \
	CF_ENUM(KEY_KP_AMPERSAND, 231) \
	/* @entry */ \
	CF_ENUM(KEY_KP_DBLAMPERSAND, 232) \
	/* @entry */ \
	CF_ENUM(KEY_KP_VERTICALBAR, 233) \
	/* @entry */ \
	CF_ENUM(KEY_KP_DBLVERTICALBAR, 234) \
	/* @entry */ \
	CF_ENUM(KEY_KP_COLON, 235) \
	/* @entry */ \
	CF_ENUM(KEY_KP_HASH, 236) \
	/* @entry */ \
	CF_ENUM(KEY_KP_SPACE, 237) \
	/* @entry */ \
	CF_ENUM(KEY_KP_AT, 238) \
	/* @entry */ \
	CF_ENUM(KEY_KP_EXCLAM, 239) \
	/* @entry */ \
	CF_ENUM(KEY_KP_MEMSTORE, 240) \
	/* @entry */ \
	CF_ENUM(KEY_KP_MEMRECALL, 241) \
	/* @entry */ \
	CF_ENUM(KEY_KP_MEMCLEAR, 242) \
	/* @entry */ \
	CF_ENUM(KEY_KP_MEMADD, 243) \
	/* @entry */ \
	CF_ENUM(KEY_KP_MEMSUBTRACT, 244) \
	/* @entry */ \
	CF_ENUM(KEY_KP_MEMMULTIPLY, 245) \
	/* @entry */ \
	CF_ENUM(KEY_KP_MEMDIVIDE, 246) \
	/* @entry */ \
	CF_ENUM(KEY_KP_PLUSMINUS, 247) \
	/* @entry */ \
	CF_ENUM(KEY_KP_CLEAR, 248) \
	/* @entry */ \
	CF_ENUM(KEY_KP_CLEARENTRY, 249) \
	/* @entry */ \
	CF_ENUM(KEY_KP_BINARY, 250) \
	/* @entry */ \
	CF_ENUM(KEY_KP_OCTAL, 251) \
	/* @entry */ \
	CF_ENUM(KEY_KP_DECIMAL, 252) \
	/* @entry */ \
	CF_ENUM(KEY_KP_HEXADECIMAL, 253) \
	/* @entry */ \
	CF_ENUM(KEY_LCTRL, 254) \
	/* @entry */ \
	CF_ENUM(KEY_LSHIFT, 255) \
	/* @entry */ \
	CF_ENUM(KEY_LALT, 256) \
	/* @entry */ \
	CF_ENUM(KEY_LGUI, 257) \
	/* @entry */ \
	CF_ENUM(KEY_RCTRL, 258) \
	/* @entry */ \
	CF_ENUM(KEY_RSHIFT, 259) \
	/* @entry */ \
	CF_ENUM(KEY_RALT, 260) \
	/* @entry */ \
	CF_ENUM(KEY_RGUI, 261) \
	/* @entry */ \
	CF_ENUM(KEY_MODE, 262) \
	/* @entry */ \
	CF_ENUM(KEY_AUDIONEXT, 263) \
	/* @entry */ \
	CF_ENUM(KEY_AUDIOPREV, 264) \
	/* @entry */ \
	CF_ENUM(KEY_AUDIOSTOP, 265) \
	/* @entry */ \
	CF_ENUM(KEY_AUDIOPLAY, 266) \
	/* @entry */ \
	CF_ENUM(KEY_AUDIOMUTE, 267) \
	/* @entry */ \
	CF_ENUM(KEY_MEDIASELECT, 268) \
	/* @entry */ \
	CF_ENUM(KEY_WWW, 269) \
	/* @entry */ \
	CF_ENUM(KEY_MAIL, 270) \
	/* @entry */ \
	CF_ENUM(KEY_CALCULATOR, 271) \
	/* @entry */ \
	CF_ENUM(KEY_COMPUTER, 272) \
	/* @entry */ \
	CF_ENUM(KEY_AC_SEARCH, 273) \
	/* @entry */ \
	CF_ENUM(KEY_AC_HOME, 274) \
	/* @entry */ \
	CF_ENUM(KEY_AC_BACK, 275) \
	/* @entry */ \
	CF_ENUM(KEY_AC_FORWARD, 276) \
	/* @entry */ \
	CF_ENUM(KEY_AC_STOP, 277) \
	/* @entry */ \
	CF_ENUM(KEY_AC_REFRESH, 278) \
	/* @entry */ \
	CF_ENUM(KEY_AC_BOOKMARKS, 279) \
	/* @entry */ \
	CF_ENUM(KEY_BRIGHTNESSDOWN, 280) \
	/* @entry */ \
	CF_ENUM(KEY_BRIGHTNESSUP, 281) \
	/* @entry */ \
	CF_ENUM(KEY_DISPLAYSWITCH, 282) \
	/* @entry */ \
	CF_ENUM(KEY_KBDILLUMTOGGLE, 283) \
	/* @entry */ \
	CF_ENUM(KEY_KBDILLUMDOWN, 284) \
	/* @entry */ \
	CF_ENUM(KEY_KBDILLUMUP, 285) \
	/* @entry */ \
	CF_ENUM(KEY_EJECT, 286) \
	/* @entry */ \
	CF_ENUM(KEY_SLEEP, 287) \
	/* @entry */ \
	CF_ENUM(KEY_ANY, 288) \
	/* @entry */ \
	CF_ENUM(KEY_COUNT, 512) \
	/* @end */

typedef enum CF_KeyButton
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_KEY_BUTTON_DEFS
	#undef CF_ENUM
} CF_KeyButton;

/**
 * @function cf_key_button_to_string
 * @category input
 * @brief    Convert an enum `CF_KeyButton` to a c-style string.
 * @param    state        The state to convert to a string.
 * @related  CF_KeyButton cf_key_button_to_string cf_key_down
 */
CF_INLINE const char* cf_key_button_to_string(CF_KeyButton button)
{
	switch (button) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_KEY_BUTTON_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_key_down
 * @category input
 * @brief    Returns true if a key is currently down.
 * @related  CF_KeyButton cf_key_down cf_key_up cf_key_just_pressed cf_key_just_released cf_key_ctrl cf_key_shift cf_key_alt cf_key_gui cf_clear_key_states cf_key_repeating
 */
CF_API bool CF_CALL cf_key_down(CF_KeyButton key);

/**
 * @function cf_key_just_pressed
 * @category input
 * @brief    Returns true if a key was just pressed.
 * @related  CF_KeyButton cf_key_down cf_key_up cf_key_just_pressed cf_key_just_released cf_key_ctrl cf_key_shift cf_key_alt cf_key_gui cf_clear_key_states cf_key_repeating
 */
CF_API bool CF_CALL cf_key_just_pressed(CF_KeyButton key);

/**
 * @function cf_key_just_released
 * @category input
 * @brief    Returns true if a key was just released.
 * @related  CF_KeyButton cf_key_down cf_key_up cf_key_just_pressed cf_key_just_released cf_key_ctrl cf_key_shift cf_key_alt cf_key_gui cf_clear_key_states cf_key_repeating
 */
CF_API bool CF_CALL cf_key_just_released(CF_KeyButton key);

/**
 * @function cf_key_repeating
 * @category input
 * @brief    Returns true if a key was held long enough to repeat.
 * @related  CF_KeyButton cf_key_down cf_key_up cf_key_just_pressed cf_key_just_released cf_key_ctrl cf_key_shift cf_key_alt cf_key_gui cf_clear_key_states cf_key_repeating
 */
CF_API bool CF_CALL cf_key_repeating(CF_KeyButton key);

/**
 * @function cf_key_ctrl
 * @category input
 * @brief    Returns true if the left or right control key is currently down.
 * @related  CF_KeyButton cf_key_down cf_key_up cf_key_just_pressed cf_key_just_released cf_key_ctrl cf_key_shift cf_key_alt cf_key_gui cf_clear_key_states cf_key_repeating
 */
CF_API bool CF_CALL cf_key_ctrl();

/**
 * @function cf_key_shift
 * @category input
 * @brief    Returns true if the left or right shift key is currently down.
 * @related  CF_KeyButton cf_key_down cf_key_up cf_key_just_pressed cf_key_just_released cf_key_ctrl cf_key_shift cf_key_alt cf_key_gui cf_clear_key_states
 */
CF_API bool CF_CALL cf_key_shift();

/**
 * @function cf_key_alt
 * @category input
 * @brief    Returns true if the left or right alt key is currently down.
 * @related  CF_KeyButton cf_key_down cf_key_up cf_key_just_pressed cf_key_just_released cf_key_ctrl cf_key_shift cf_key_alt cf_key_gui cf_clear_key_states
 */
CF_API bool CF_CALL cf_key_alt();

/**
 * @function cf_key_gui
 * @category input
 * @brief    Returns true if the left or right gui key is currently down.
 * @remarks  Windows key in Windows, Command key in OSX.
 * @related  CF_KeyButton cf_key_down cf_key_up cf_key_just_pressed cf_key_just_released cf_key_ctrl cf_key_shift cf_key_alt cf_key_gui cf_clear_key_states
 */
CF_API bool CF_CALL cf_key_gui();

/**
 * @function cf_clear_key_states
 * @category input
 * @brief    Zeroes out all internal keyboard state.
 * @related  CF_KeyButton cf_key_down cf_key_up cf_key_just_pressed cf_key_just_released cf_key_ctrl cf_key_shift cf_key_alt cf_key_gui cf_clear_key_states
 */
CF_API void CF_CALL cf_clear_key_states();

/**
 * @function cf_register_key_callback
 * @category input
 * @brief    Registers a callback invoked whenever a key is pressed.
 * @related  CF_KeyButton cf_key_down cf_key_up cf_key_just_pressed cf_key_just_released cf_key_ctrl cf_key_shift cf_key_alt cf_key_gui cf_clear_key_states
 */
CF_API void CF_CALL cf_register_key_callback(void (*key_callback)(CF_KeyButton key, bool true_down_false_up));

/**
 * @function cf_mouse_x
 * @category input
 * @brief    Returns the current mouse x-coordinate in pixels.
 * @remarks  (0, 0) is the top-left of the screen, y-downards.
 * @related  CF_MouseButton cf_mouse_down cf_mouse_x cf_mouse_y
 */
CF_API int CF_CALL cf_mouse_x();

/**
 * @function cf_mouse_y
 * @category input
 * @brief    Returns the current mouse y-coordinate in pixels.
 * @remarks  (0, 0) is the top-left of the screen, y-downwards.
 * @related  CF_MouseButton cf_mouse_down cf_mouse_x cf_mouse_y
 */
CF_API int CF_CALL cf_mouse_y();

/**
 * @function cf_mouse_down
 * @category input
 * @brief    Returns true if the mouse button is currently down.
 * @related  CF_MouseButton cf_mouse_down cf_mouse_x cf_mouse_y cf_mouse_just_pressed cf_mouse_just_released cf_mouse_wheel_motion cf_mouse_double_click_held cf_mouse_double_clicked
 */
CF_API bool CF_CALL cf_mouse_down(CF_MouseButton button);

/**
 * @function cf_mouse_just_pressed
 * @category input
 * @brief    Returns true if the mouse button was just pressed.
 * @related  CF_MouseButton cf_mouse_down cf_mouse_x cf_mouse_y cf_mouse_just_pressed cf_mouse_just_released cf_mouse_wheel_motion cf_mouse_double_click_held cf_mouse_double_clicked
 */
CF_API bool CF_CALL cf_mouse_just_pressed(CF_MouseButton button);

/**
 * @function cf_mouse_just_released
 * @category input
 * @brief    Returns true if the mouse button was just released.
 * @related  CF_MouseButton cf_mouse_down cf_mouse_x cf_mouse_y cf_mouse_just_pressed cf_mouse_just_released cf_mouse_wheel_motion cf_mouse_double_click_held cf_mouse_double_clicked
 */
CF_API bool CF_CALL cf_mouse_just_released(CF_MouseButton button);

/**
 * @function cf_mouse_wheel_motion
 * @category input
 * @brief    Returns a signed integer representing by how much the mouse wheel was rotated.
 * @related  CF_MouseButton cf_mouse_down cf_mouse_x cf_mouse_y cf_mouse_just_pressed cf_mouse_just_released cf_mouse_wheel_motion cf_mouse_double_click_held cf_mouse_double_clicked
 */
CF_API int CF_CALL cf_mouse_wheel_motion();

/**
 * @function cf_mouse_double_click_held
 * @category input
 * @brief    Returns true while a double click was detected and currently held down.
 * @related  CF_MouseButton cf_mouse_down cf_mouse_x cf_mouse_y cf_mouse_just_pressed cf_mouse_just_released cf_mouse_wheel_motion cf_mouse_double_click_held cf_mouse_double_clicked
 */
CF_API bool CF_CALL cf_mouse_double_click_held(CF_MouseButton button);

/**
 * @function cf_mouse_double_clicked
 * @category input
 * @brief    Returns true if a double click was just detected.
 * @related  CF_MouseButton cf_mouse_down cf_mouse_x cf_mouse_y cf_mouse_just_pressed cf_mouse_just_released cf_mouse_wheel_motion cf_mouse_double_click_held cf_mouse_double_clicked
 */
CF_API bool CF_CALL cf_mouse_double_clicked(CF_MouseButton button);

/**
 * @function cf_mouse_hide
 * @category input
 * @brief    Hides or shows the mouse.
 * @related  cf_mouse_hide cf_mouse_hidden cf_mouse_lock_inside_window
 */
CF_API void CF_CALL cf_mouse_hide(bool true_to_hide);

/**
 * @function cf_mouse_hidden
 * @category input
 * @brief    Returns whether the mouse is hidden.
 * @return   True means hidden, false means not hidden.
 * @related  cf_mouse_hide cf_mouse_hidden cf_mouse_lock_inside_window
 */
CF_API bool CF_CALL cf_mouse_hidden();

/**
 * @function cf_mouse_lock_inside_window
 * @category input
 * @brief    Locks the mouse within the window's borders.
 * @remarks  This is off by default, meaning the mouse is free to leave the border of the window.
 * @related  cf_mouse_hide cf_mouse_hidden cf_mouse_lock_inside_window
 */
CF_API void CF_CALL cf_mouse_lock_inside_window(bool true_to_lock);

/**
 * @function cf_input_text_add_utf8
 * @category input
 * @brief    Adds a utf8 codepoint to the input buffer of the application.
 * @remarks  The input text functions are for dealing with text input. Not all text inputs come from a single key-stroke, as some are comprised of
 *           multiple keystrokes, especially when dealing with non-Latin based inputs.
 * @related  cf_input_text_add_utf8 cf_input_text_pop_utf32 cf_input_text_has_data cf_input_text_clear
 */
CF_API void CF_CALL cf_input_text_add_utf8(const char* text);

/**
 * @function cf_input_text_pop_utf32
 * @category input
 * @brief    Pops a utf8 codepoint off of the input buffer of the application.
 * @remarks  The input text functions are for dealing with text input. Not all text inputs come from a single key-stroke, as some are comprised of
 *           multiple keystrokes, especially when dealing with non-Latin based inputs.
 * @related  cf_input_text_add_utf8 cf_input_text_pop_utf32 cf_input_text_has_data cf_input_text_clear
 */
CF_API int CF_CALL cf_input_text_pop_utf32();

/**
 * @function cf_input_text_has_data
 * @category input
 * @brief    Returns true if the input buffer of the application has any text within.
 * @remarks  The input text functions are for dealing with text input. Not all text inputs come from a single key-stroke, as some are comprised of
 *           multiple keystrokes, especially when dealing with non-Latin based inputs.
 * @related  cf_input_text_add_utf8 cf_input_text_pop_utf32 cf_input_text_has_data cf_input_text_clear
 */
CF_API bool CF_CALL cf_input_text_has_data();

/**
 * @function cf_input_text_clear
 * @category input
 * @brief    Clears the application's input text buffer.
 * @remarks  The input text functions are for dealing with text input. Not all text inputs come from a single key-stroke, as some are comprised of
 *           multiple keystrokes, especially when dealing with non-Latin based inputs.
 * @related  cf_input_text_add_utf8 cf_input_text_pop_utf32 cf_input_text_has_data cf_input_text_clear
 */
CF_API void CF_CALL cf_input_text_clear();

/**
 * @function cf_input_enable_ime
 * @category input
 * @brief    Enables the IME (Input Method Editor) for the operating system.
 * @remarks  This is an advanced function. It's useful for gathering input from a variety of languages, where keystrokes are translated into a variety
 *           of different language inputs. This is usually a feature of the underlying operating system.
 * @related  cf_input_enable_ime cf_input_disable_ime cf_input_is_ime_enabled cf_input_has_ime_keyboard_support cf_input_is_ime_keyboard_shown cf_input_set_ime_rect
 */
CF_API void CF_CALL cf_input_enable_ime();

/**
 * @function cf_input_disable_ime
 * @category input
 * @brief    Disables the IME (Input Method Editor) for the operating system.
 * @remarks  This is an advanced function. It's useful for gathering input from a variety of languages, where keystrokes are translated into a variety
 *           of different language inputs. This is usually a feature of the underlying operating system.
 * @related  cf_input_enable_ime cf_input_disable_ime cf_input_is_ime_enabled cf_input_has_ime_keyboard_support cf_input_is_ime_keyboard_shown cf_input_set_ime_rect
 */
CF_API void CF_CALL cf_input_disable_ime();

/**
 * @function cf_input_is_ime_enabled
 * @category input
 * @brief    Returns true if the IME (Input Method Editor) for the operating system is enabled.
 * @remarks  This is an advanced function. It's useful for gathering input from a variety of languages, where keystrokes are translated into a variety
 *           of different language inputs. This is usually a feature of the underlying operating system.
 * @related  cf_input_enable_ime cf_input_disable_ime cf_input_is_ime_enabled cf_input_has_ime_keyboard_support cf_input_is_ime_keyboard_shown cf_input_set_ime_rect
 */
CF_API bool CF_CALL cf_input_is_ime_enabled();

/**
 * @function cf_input_has_ime_keyboard_support
 * @category input
 * @brief    Returns true if the IME (Input Method Editor) for the operating system has keyboard support.
 * @remarks  This is an advanced function. It's useful for gathering input from a variety of languages, where keystrokes are translated into a variety
 *           of different language inputs. This is usually a feature of the underlying operating system.
 * @related  cf_input_enable_ime cf_input_disable_ime cf_input_is_ime_enabled cf_input_has_ime_keyboard_support cf_input_is_ime_keyboard_shown cf_input_set_ime_rect
 */
CF_API bool CF_CALL cf_input_has_ime_keyboard_support();

/**
 * @function cf_input_is_ime_keyboard_shown
 * @category input
 * @brief    Returns true if the IME (Input Method Editor) for the operating system is currently showing the keyboard.
 * @remarks  This is an advanced function. It's useful for gathering input from a variety of languages, where keystrokes are translated into a variety
 *           of different language inputs. This is usually a feature of the underlying operating system.
 * @related  cf_input_enable_ime cf_input_disable_ime cf_input_is_ime_enabled cf_input_has_ime_keyboard_support cf_input_is_ime_keyboard_shown cf_input_set_ime_rect
 */
CF_API bool CF_CALL cf_input_is_ime_keyboard_shown();

/**
 * @function cf_input_set_ime_rect
 * @category input
 * @brief    Tells the operating system where the current IME (Input Method Editor) rect should be.
 * @remarks  This is an advanced function. It's useful for gathering input from a variety of languages, where keystrokes are translated into a variety
 *           of different language inputs. This is usually a feature of the underlying operating system.
 * @related  cf_input_enable_ime cf_input_disable_ime cf_input_is_ime_enabled cf_input_has_ime_keyboard_support cf_input_is_ime_keyboard_shown cf_input_set_ime_rect
 */
CF_API void CF_CALL cf_input_set_ime_rect(int x, int y, int w, int h);

/**
 * @struct   CF_ImeComposition
 * @category input
 * @brief    Represents the IME (Input Method Editor) composition from the operating system, for gathering complex text inputs.
 * @related  cf_input_enable_ime CF_ImeComposition cf_input_get_ime_composition
 */
typedef struct CF_ImeComposition
{
	/* @member The composition text, as it's currently being constructed. */
	const char* composition;

	/* @member Where in the text the user is currently editing/typing/selecting. */
	int cursor;

	/* @member If the user is currently selecting text, describes the length of the selection. */
	int selection_len;
} CF_ImeComposition;
// @end

/**
 * @function cf_input_get_ime_composition
 * @category input
 * @brief    Returns the current IME (Input Method Editor) composition. See `CF_ImeComposition`.
 * @param    composition    The text composition.
 * @return   Returns true if the IME (Input Method Editor) is currently composing text.
 * @remarks  This is an advanced function. It's useful for gathering input from a variety of languages, where keystrokes are translated into a variety
 *           of different language inputs. This is usually a feature of the underlying operating system.
 * @related  cf_input_enable_ime CF_ImeComposition cf_input_get_ime_composition
 */
CF_API bool CF_CALL cf_input_get_ime_composition(CF_ImeComposition* composition);

/**
 * @struct   CF_Touch
 * @category input
 * @brief    Represents a single touch event on the device.
 * @related  CF_Touch cf_touch_get_all cf_touch_get
 */
typedef struct CF_Touch
{
	/* @member A unique identifier for every touch event. */
	uint64_t id;

	/* @member The x-position of the touch event. Normalized from [0,1], where [0,0] is the top-left corner. */
	float x;

	/* @member The y-position of the touch event. Normalized from [0,1], where [0,0] is the top-left corner. */
	float y;

	/* @member A number from [0,1] representing the touch pressure. */
	float pressure;
} CF_Touch;
// @end

/**
 * @function cf_touch_get_all
 * @category input
 * @brief    Returns an array of all touch events.
 * @param    touch_all      An array of all `CF_Touch` touch events. See example section.
 * @return   Returns the number of `CF_Touch` events in `touch_all`.
 * @example > Looping over all touch events.
 *     CF_Touch* touches = NULL;
 *     int touch_count = cf_touch_get_all(&touches);
 *     for (int i = 0; i < touch_count; ++i) {
 *         do_something(touches[i]);
 *     }
 * @related  CF_Touch cf_touch_get_all cf_touch_get
 */
CF_API int CF_CALL cf_touch_get_all(CF_Touch** touch_all);

/**
 * @function cf_touch_get
 * @category input
 * @brief    Fetches a specific touch event.
 * @param    id        The unique identifier of a specific touch event.
 * @param    touch     Pointer to a `CF_Touch` to fill in.
 * @return   Returns true if the touch event was found and currently still active.
 * @remarks  You should use `cf_touch_get_all` to peek at all current touch events. Make note of any touch events that are
 *           new. Then, you can loop over all touch events you've noted with this function, and remove them when they
 *           become unavailable.
 * @related  CF_Touch cf_touch_get_all cf_touch_get
 */
CF_API bool CF_CALL cf_touch_get(uint64_t id, CF_Touch* touch);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using KeyButton = CF_KeyButton;
#define CF_ENUM(K, V) CF_INLINE constexpr KeyButton K = CF_##K;
CF_KEY_BUTTON_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(KeyButton button)
{
	switch (button) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_KEY_BUTTON_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using MouseButton = CF_MouseButton;
#define CF_ENUM(K, V) CF_INLINE constexpr MouseButton K = CF_##K;
CF_MOUSE_BUTTON_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(MouseButton button)
{
	switch (button) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_MOUSE_BUTTON_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using ImeComposition = CF_ImeComposition;
using Touch = CF_Touch;

CF_INLINE bool key_down(KeyButton key) { return cf_key_down(key); }
CF_INLINE bool key_just_pressed(KeyButton key) { return cf_key_just_pressed(key); }
CF_INLINE bool key_just_released(KeyButton key) { return cf_key_just_released(key); }
CF_INLINE bool key_repeating(KeyButton key) { return cf_key_repeating(key); }
CF_INLINE bool key_ctrl() { return cf_key_ctrl(); }
CF_INLINE bool key_shift() { return cf_key_shift(); }
CF_INLINE bool key_alt() { return cf_key_alt(); }
CF_INLINE bool key_gui() { return cf_key_gui(); }
CF_INLINE void clear_key_states() { cf_clear_key_states(); }

CF_INLINE int mouse_x() { return cf_mouse_x(); }
CF_INLINE int mouse_y() { return cf_mouse_y(); }

CF_INLINE bool mouse_down(MouseButton button) { return cf_mouse_down(button); }
CF_INLINE bool mouse_just_pressed(MouseButton button) { return cf_mouse_just_pressed(button); }
CF_INLINE bool mouse_just_released(MouseButton button) { return cf_mouse_just_released(button); }
CF_INLINE int mouse_wheel_motion() { return cf_mouse_wheel_motion(); }
CF_INLINE bool mouse_double_click_held(MouseButton button) { return cf_mouse_double_click_held(button); }
CF_INLINE bool mouse_double_clicked(MouseButton button) { return cf_mouse_double_clicked(button); }
CF_INLINE void mouse_hide(bool true_to_hide) { cf_mouse_hide(true_to_hide); }
CF_INLINE bool mouse_hidden() { return cf_mouse_hidden(); }
CF_INLINE void mouse_lock_inside_window(bool true_to_lock) { cf_mouse_lock_inside_window(true_to_lock); }

CF_INLINE void input_text_add_utf8(const char* text) { cf_input_text_add_utf8(text); }
CF_INLINE int input_text_pop_utf32() { return cf_input_text_pop_utf32(); }
CF_INLINE bool input_text_has_data() { return cf_input_text_has_data(); }
CF_INLINE void input_text_clear() {  cf_input_text_clear(); }
	 
CF_INLINE void input_enable_ime() { cf_input_enable_ime(); }
CF_INLINE void input_disable_ime() { cf_input_disable_ime(); }
CF_INLINE bool input_is_ime_enabled() { return cf_input_is_ime_enabled(); }
CF_INLINE bool input_has_ime_keyboard_support() { return cf_input_has_ime_keyboard_support(); }
CF_INLINE bool input_is_ime_keyboard_shown() { return cf_input_is_ime_keyboard_shown(); }
CF_INLINE void input_set_ime_rect(int x, int y, int w, int h) { cf_input_set_ime_rect(x, y, w, h); }

CF_INLINE bool input_get_ime_composition(ImeComposition* composition) { return cf_input_get_ime_composition(composition); }

CF_INLINE bool touch_get(uint64_t id, Touch* touch) { return cf_touch_get(id,touch); }

}

#endif // CF_CPP

#endif // CF_INPUT_H
