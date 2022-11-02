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

#ifndef CUTE_INPUT_H
#define CUTE_INPUT_H

#include "cute_defines.h"
#include "cute_array.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum cf_key_button_t cf_key_button_t;
typedef enum cf_mouse_button_t cf_mouse_button_t;
typedef enum cf_mouse_click_t cf_mouse_click_t;

CUTE_API bool CUTE_CALL cf_key_is_down(cf_key_button_t key);
CUTE_API bool CUTE_CALL cf_key_is_up(cf_key_button_t key);
CUTE_API bool CUTE_CALL cf_key_was_pressed(cf_key_button_t key);
CUTE_API bool CUTE_CALL cf_key_was_released(cf_key_button_t key);
CUTE_API bool CUTE_CALL cf_key_ctrl();
CUTE_API bool CUTE_CALL cf_key_shift();
CUTE_API bool CUTE_CALL cf_key_alt();
CUTE_API bool CUTE_CALL cf_key_gui(); // Windows key in Windows, Command key in OSX.
CUTE_API void CUTE_CALL cf_clear_all_key_state();

CUTE_API int CUTE_CALL cf_mouse_x();
CUTE_API int CUTE_CALL cf_mouse_y();

CUTE_API bool CUTE_CALL cf_mouse_is_down(cf_mouse_button_t button);
CUTE_API bool CUTE_CALL cf_mouse_is_up(cf_mouse_button_t button);
CUTE_API bool CUTE_CALL cf_mouse_was_pressed(cf_mouse_button_t button);
CUTE_API bool CUTE_CALL cf_mouse_was_released(cf_mouse_button_t button);
CUTE_API int CUTE_CALL cf_mouse_wheel_motion();
CUTE_API bool CUTE_CALL cf_mouse_is_down_double_click(cf_mouse_button_t button);
CUTE_API bool CUTE_CALL cf_mouse_double_click_was_pressed(cf_mouse_button_t button);

CUTE_API void CUTE_CALL cf_input_text_add_utf8(const char* text);
CUTE_API int CUTE_CALL cf_input_text_pop_utf32();
CUTE_API bool CUTE_CALL cf_input_text_has_data();
CUTE_API void CUTE_CALL cf_input_text_clear();

CUTE_API void CUTE_CALL cf_input_enable_ime();
CUTE_API void CUTE_CALL cf_input_disable_ime();
CUTE_API bool CUTE_CALL cf_input_is_ime_enabled();
CUTE_API bool CUTE_CALL cf_input_has_ime_keyboard_support();
CUTE_API bool CUTE_CALL cf_input_is_ime_keyboard_shown();
CUTE_API void CUTE_CALL cf_input_set_ime_rect(int x, int y, int w, int h);

typedef struct cf_ime_composition_t
{
	const char* composition;
	int cursor;
	int selection_len;
} cf_ime_composition_t;

CUTE_API bool CUTE_CALL cf_input_get_ime_composition(cf_ime_composition_t* composition);

typedef struct cf_touch_t
{
	uint64_t id;
	float x;
	float y;
	float pressure;
} cf_touch_t;

CUTE_API int CUTE_CALL cf_touch_get_all(cf_touch_t** touch_all);
CUTE_API bool CUTE_CALL cf_touch_get(uint64_t id, cf_touch_t* touch);

#define CF_MOUSE_BUTTON_DEFS \
	CF_ENUM(MOUSE_BUTTON_LEFT, 0) \
	CF_ENUM(MOUSE_BUTTON_RIGHT, 1) \
	CF_ENUM(MOUSE_BUTTON_MIDDLE, 2) \

typedef enum cf_mouse_button_t
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_MOUSE_BUTTON_DEFS
	#undef CF_ENUM
} cf_mouse_button_t;

#define CF_KEY_BUTTON_DEFS \
	CF_ENUM(KEY_UNKNOWN, 0) \
	CF_ENUM(KEY_RETURN, 4) \
	CF_ENUM(KEY_ESCAPE, '\033') \
	CF_ENUM(KEY_BACKSPACE, '\b') \
	CF_ENUM(KEY_TAB, '\t') \
	CF_ENUM(KEY_SPACE, ' ') \
	CF_ENUM(KEY_EXCLAIM, '!') \
	CF_ENUM(KEY_QUOTEDBL, '"') \
	CF_ENUM(KEY_HASH, '#') \
	CF_ENUM(KEY_PERCENT, '%') \
	CF_ENUM(KEY_DOLLAR, '$') \
	CF_ENUM(KEY_AMPERSAND, '&') \
	CF_ENUM(KEY_QUOTE, '\'') \
	CF_ENUM(KEY_LEFTPAREN, '(') \
	CF_ENUM(KEY_RIGHTPAREN, ')') \
	CF_ENUM(KEY_ASTERISK, '*') \
	CF_ENUM(KEY_PLUS, '+') \
	CF_ENUM(KEY_COMMA, ',') \
	CF_ENUM(KEY_MINUS, '-') \
	CF_ENUM(KEY_PERIOD, '.') \
	CF_ENUM(KEY_SLASH, '/') \
	CF_ENUM(KEY_0, '0') \
	CF_ENUM(KEY_1, '1') \
	CF_ENUM(KEY_2, '2') \
	CF_ENUM(KEY_3, '3') \
	CF_ENUM(KEY_4, '4') \
	CF_ENUM(KEY_5, '5') \
	CF_ENUM(KEY_6, '6') \
	CF_ENUM(KEY_7, '7') \
	CF_ENUM(KEY_8, '8') \
	CF_ENUM(KEY_9, '9') \
	CF_ENUM(KEY_COLON, ':') \
	CF_ENUM(KEY_SEMICOLON, ';') \
	CF_ENUM(KEY_LESS, '<') \
	CF_ENUM(KEY_EQUALS, '=') \
	CF_ENUM(KEY_GREATER, '>') \
	CF_ENUM(KEY_QUESTION, '?') \
	CF_ENUM(KEY_AT, '@') \
	CF_ENUM(KEY_LEFTBRACKET, '[') \
	CF_ENUM(KEY_BACKSLASH, '\\') \
	CF_ENUM(KEY_RIGHTBRACKET, ']') \
	CF_ENUM(KEY_CARET, '^') \
	CF_ENUM(KEY_UNDERSCORE, '_') \
	CF_ENUM(KEY_BACKQUOTE, '`') \
	CF_ENUM(KEY_A, 'a') \
	CF_ENUM(KEY_B, 'b') \
	CF_ENUM(KEY_C, 'c') \
	CF_ENUM(KEY_D, 'd') \
	CF_ENUM(KEY_E, 'e') \
	CF_ENUM(KEY_F, 'f') \
	CF_ENUM(KEY_G, 'g') \
	CF_ENUM(KEY_H, 'h') \
	CF_ENUM(KEY_I, 'i') \
	CF_ENUM(KEY_J, 'j') \
	CF_ENUM(KEY_K, 'k') \
	CF_ENUM(KEY_L, 'l') \
	CF_ENUM(KEY_M, 'm') \
	CF_ENUM(KEY_N, 'n') \
	CF_ENUM(KEY_O, 'o') \
	CF_ENUM(KEY_P, 'p') \
	CF_ENUM(KEY_Q, 'q') \
	CF_ENUM(KEY_R, 'r') \
	CF_ENUM(KEY_S, 's') \
	CF_ENUM(KEY_T, 't') \
	CF_ENUM(KEY_U, 'u') \
	CF_ENUM(KEY_V, 'v') \
	CF_ENUM(KEY_W, 'w') \
	CF_ENUM(KEY_X, 'x') \
	CF_ENUM(KEY_Y, 'y') \
	CF_ENUM(KEY_Z, 'z') \
	CF_ENUM(KEY_CAPSLOCK, 123) \
	CF_ENUM(KEY_F1, 124) \
	CF_ENUM(KEY_F2, 125) \
	CF_ENUM(KEY_F3, 126) \
	CF_ENUM(KEY_F4, 127) \
	CF_ENUM(KEY_F5, 128) \
	CF_ENUM(KEY_F6, 129) \
	CF_ENUM(KEY_F7, 130) \
	CF_ENUM(KEY_F8, 131) \
	CF_ENUM(KEY_F9, 132) \
	CF_ENUM(KEY_F10, 133) \
	CF_ENUM(KEY_F11, 134) \
	CF_ENUM(KEY_F12, 135) \
	CF_ENUM(KEY_PRINTSCREEN, 136) \
	CF_ENUM(KEY_SCROLLLOCK, 137) \
	CF_ENUM(KEY_PAUSE, 138) \
	CF_ENUM(KEY_INSERT, 139) \
	CF_ENUM(KEY_HOME, 140) \
	CF_ENUM(KEY_PAGEUP, 141) \
	CF_ENUM(KEY_DELETE, 142) \
	CF_ENUM(KEY_END, 143) \
	CF_ENUM(KEY_PAGEDOWN, 144) \
	CF_ENUM(KEY_RIGHT, 145) \
	CF_ENUM(KEY_LEFT, 146) \
	CF_ENUM(KEY_DOWN, 147) \
	CF_ENUM(KEY_UP, 148) \
	CF_ENUM(KEY_NUMLOCKCLEAR, 149) \
	CF_ENUM(KEY_KP_DIVIDE, 150) \
	CF_ENUM(KEY_KP_MULTIPLY, 151) \
	CF_ENUM(KEY_KP_MINUS, 152) \
	CF_ENUM(KEY_KP_PLUS, 153) \
	CF_ENUM(KEY_KP_ENTER, 154) \
	CF_ENUM(KEY_KP_1, 155) \
	CF_ENUM(KEY_KP_2, 156) \
	CF_ENUM(KEY_KP_3, 157) \
	CF_ENUM(KEY_KP_4, 158) \
	CF_ENUM(KEY_KP_5, 159) \
	CF_ENUM(KEY_KP_6, 160) \
	CF_ENUM(KEY_KP_7, 161) \
	CF_ENUM(KEY_KP_8, 162) \
	CF_ENUM(KEY_KP_9, 163) \
	CF_ENUM(KEY_KP_0, 164) \
	CF_ENUM(KEY_KP_PERIOD, 165) \
	CF_ENUM(KEY_APPLICATION, 166) \
	CF_ENUM(KEY_POWER, 167) \
	CF_ENUM(KEY_KP_EQUALS, 168) \
	CF_ENUM(KEY_F13, 169) \
	CF_ENUM(KEY_F14, 170) \
	CF_ENUM(KEY_F15, 171) \
	CF_ENUM(KEY_F16, 172) \
	CF_ENUM(KEY_F17, 173) \
	CF_ENUM(KEY_F18, 174) \
	CF_ENUM(KEY_F19, 175) \
	CF_ENUM(KEY_F20, 176) \
	CF_ENUM(KEY_F21, 177) \
	CF_ENUM(KEY_F22, 178) \
	CF_ENUM(KEY_F23, 179) \
	CF_ENUM(KEY_F24, 180) \
	CF_ENUM(KEY_HELP, 181) \
	CF_ENUM(KEY_MENU, 182) \
	CF_ENUM(KEY_SELECT, 183) \
	CF_ENUM(KEY_STOP, 184) \
	CF_ENUM(KEY_AGAIN, 185) \
	CF_ENUM(KEY_UNDO, 186) \
	CF_ENUM(KEY_CUT, 187) \
	CF_ENUM(KEY_COPY, 188) \
	CF_ENUM(KEY_PASTE, 189) \
	CF_ENUM(KEY_FIND, 190) \
	CF_ENUM(KEY_MUTE, 191) \
	CF_ENUM(KEY_VOLUMEUP, 192) \
	CF_ENUM(KEY_VOLUMEDOWN, 193) \
	CF_ENUM(KEY_KP_COMMA, 194) \
	CF_ENUM(KEY_KP_EQUALSAS400, 195) \
	CF_ENUM(KEY_ALTERASE, 196) \
	CF_ENUM(KEY_SYSREQ, 197) \
	CF_ENUM(KEY_CANCEL, 198) \
	CF_ENUM(KEY_CLEAR, 199) \
	CF_ENUM(KEY_PRIOR, 200) \
	CF_ENUM(KEY_RETURN2, 201) \
	CF_ENUM(KEY_SEPARATOR, 202) \
	CF_ENUM(KEY_OUT, 203) \
	CF_ENUM(KEY_OPER, 204) \
	CF_ENUM(KEY_CLEARAGAIN, 205) \
	CF_ENUM(KEY_CRSEL, 206) \
	CF_ENUM(KEY_EXSEL, 207) \
	CF_ENUM(KEY_KP_00, 208) \
	CF_ENUM(KEY_KP_000, 209) \
	CF_ENUM(KEY_THOUSANDSSEPARATOR, 210) \
	CF_ENUM(KEY_DECIMALSEPARATOR, 211) \
	CF_ENUM(KEY_CURRENCYUNIT, 212) \
	CF_ENUM(KEY_CURRENCYSUBUNIT, 213) \
	CF_ENUM(KEY_KP_LEFTPAREN, 214) \
	CF_ENUM(KEY_KP_RIGHTPAREN, 215) \
	CF_ENUM(KEY_KP_LEFTBRACE, 216) \
	CF_ENUM(KEY_KP_RIGHTBRACE, 217) \
	CF_ENUM(KEY_KP_TAB, 218) \
	CF_ENUM(KEY_KP_BACKSPACE, 219) \
	CF_ENUM(KEY_KP_A, 220) \
	CF_ENUM(KEY_KP_B, 221) \
	CF_ENUM(KEY_KP_C, 222) \
	CF_ENUM(KEY_KP_D, 223) \
	CF_ENUM(KEY_KP_E, 224) \
	CF_ENUM(KEY_KP_F, 225) \
	CF_ENUM(KEY_KP_XOR, 226) \
	CF_ENUM(KEY_KP_POWER, 227) \
	CF_ENUM(KEY_KP_PERCENT, 228) \
	CF_ENUM(KEY_KP_LESS, 229) \
	CF_ENUM(KEY_KP_GREATER, 230) \
	CF_ENUM(KEY_KP_AMPERSAND, 231) \
	CF_ENUM(KEY_KP_DBLAMPERSAND, 232) \
	CF_ENUM(KEY_KP_VERTICALBAR, 233) \
	CF_ENUM(KEY_KP_DBLVERTICALBAR, 234) \
	CF_ENUM(KEY_KP_COLON, 235) \
	CF_ENUM(KEY_KP_HASH, 236) \
	CF_ENUM(KEY_KP_SPACE, 237) \
	CF_ENUM(KEY_KP_AT, 238) \
	CF_ENUM(KEY_KP_EXCLAM, 239) \
	CF_ENUM(KEY_KP_MEMSTORE, 240) \
	CF_ENUM(KEY_KP_MEMRECALL, 241) \
	CF_ENUM(KEY_KP_MEMCLEAR, 242) \
	CF_ENUM(KEY_KP_MEMADD, 243) \
	CF_ENUM(KEY_KP_MEMSUBTRACT, 244) \
	CF_ENUM(KEY_KP_MEMMULTIPLY, 245) \
	CF_ENUM(KEY_KP_MEMDIVIDE, 246) \
	CF_ENUM(KEY_KP_PLUSMINUS, 247) \
	CF_ENUM(KEY_KP_CLEAR, 248) \
	CF_ENUM(KEY_KP_CLEARENTRY, 249) \
	CF_ENUM(KEY_KP_BINARY, 250) \
	CF_ENUM(KEY_KP_OCTAL, 251) \
	CF_ENUM(KEY_KP_DECIMAL, 252) \
	CF_ENUM(KEY_KP_HEXADECIMAL, 253) \
	CF_ENUM(KEY_LCTRL, 254) \
	CF_ENUM(KEY_LSHIFT, 255) \
	CF_ENUM(KEY_LALT, 256) \
	CF_ENUM(KEY_LGUI, 257) \
	CF_ENUM(KEY_RCTRL, 258) \
	CF_ENUM(KEY_RSHIFT, 259) \
	CF_ENUM(KEY_RALT, 260) \
	CF_ENUM(KEY_RGUI, 261) \
	CF_ENUM(KEY_MODE, 262) \
	CF_ENUM(KEY_AUDIONEXT, 263) \
	CF_ENUM(KEY_AUDIOPREV, 264) \
	CF_ENUM(KEY_AUDIOSTOP, 265) \
	CF_ENUM(KEY_AUDIOPLAY, 266) \
	CF_ENUM(KEY_AUDIOMUTE, 267) \
	CF_ENUM(KEY_MEDIASELECT, 268) \
	CF_ENUM(KEY_WWW, 269) \
	CF_ENUM(KEY_MAIL, 270) \
	CF_ENUM(KEY_CALCULATOR, 271) \
	CF_ENUM(KEY_COMPUTER, 272) \
	CF_ENUM(KEY_AC_SEARCH, 273) \
	CF_ENUM(KEY_AC_HOME, 274) \
	CF_ENUM(KEY_AC_BACK, 275) \
	CF_ENUM(KEY_AC_FORWARD, 276) \
	CF_ENUM(KEY_AC_STOP, 277) \
	CF_ENUM(KEY_AC_REFRESH, 278) \
	CF_ENUM(KEY_AC_BOOKMARKS, 279) \
	CF_ENUM(KEY_BRIGHTNESSDOWN, 280) \
	CF_ENUM(KEY_BRIGHTNESSUP, 281) \
	CF_ENUM(KEY_DISPLAYSWITCH, 282) \
	CF_ENUM(KEY_KBDILLUMTOGGLE, 283) \
	CF_ENUM(KEY_KBDILLUMDOWN, 284) \
	CF_ENUM(KEY_KBDILLUMUP, 285) \
	CF_ENUM(KEY_EJECT, 286) \
	CF_ENUM(KEY_SLEEP, 287) \
	CF_ENUM(KEY_ANY, 288) \
	CF_ENUM(KEY_COUNT, 512) \

typedef enum cf_key_button_t
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_KEY_BUTTON_DEFS
	#undef CF_ENUM
} cf_key_button_t;

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

enum key_button_t : int
{
#define CF_ENUM(K, V) K = V,
CF_KEY_BUTTON_DEFS
#undef CF_ENUM
};

enum mouse_button_t : int
{
#define CF_ENUM(K, V) K = V,
CF_MOUSE_BUTTON_DEFS
#undef CF_ENUM
};

using ime_composition_t = cf_ime_composition_t;
using touch_t = cf_touch_t;

CUTE_INLINE bool CUTE_CALL key_is_down(key_button_t key) { return cf_key_is_down((cf_key_button_t)key); }
CUTE_INLINE bool CUTE_CALL key_is_up(key_button_t key) { return cf_key_is_up((cf_key_button_t)key); }
CUTE_INLINE bool CUTE_CALL key_was_pressed(key_button_t key) { return cf_key_was_pressed((cf_key_button_t)key); }
CUTE_INLINE bool CUTE_CALL key_was_released(key_button_t key) { return cf_key_was_released((cf_key_button_t)key); }
CUTE_INLINE bool CUTE_CALL key_ctrl() { return cf_key_ctrl(); }
CUTE_INLINE bool CUTE_CALL key_shift() { return cf_key_shift(); }
CUTE_INLINE bool CUTE_CALL key_alt() { return cf_key_alt(); }
CUTE_INLINE bool CUTE_CALL key_gui() { return cf_key_gui(); }
CUTE_INLINE void CUTE_CALL clear_all_key_state() { cf_clear_all_key_state(); }
	 
CUTE_INLINE int CUTE_CALL mouse_x() { return cf_mouse_x(); }
CUTE_INLINE int CUTE_CALL mouse_y() { return cf_mouse_y(); }
	 
CUTE_INLINE bool CUTE_CALL mouse_is_down(mouse_button_t button) { return cf_mouse_is_down((cf_mouse_button_t)button); }
CUTE_INLINE bool CUTE_CALL mouse_is_up(mouse_button_t button) { return cf_mouse_is_up((cf_mouse_button_t)button); }
CUTE_INLINE bool CUTE_CALL mouse_was_pressed(mouse_button_t button) { return cf_mouse_was_pressed((cf_mouse_button_t)button); }
CUTE_INLINE bool CUTE_CALL mouse_was_released(mouse_button_t button) { return cf_mouse_was_released((cf_mouse_button_t)button); }
CUTE_INLINE int CUTE_CALL mouse_wheel_motion() { return cf_mouse_wheel_motion(); }
CUTE_INLINE bool CUTE_CALL mouse_is_down_double_click(mouse_button_t button) { return cf_mouse_is_down_double_click((cf_mouse_button_t)button); }
CUTE_INLINE bool CUTE_CALL mouse_double_click_was_pressed(mouse_button_t button) { return cf_mouse_double_click_was_pressed((cf_mouse_button_t)button); }
	 
CUTE_INLINE void CUTE_CALL input_text_add_utf8(const char* text) { cf_input_text_add_utf8(text); }
CUTE_INLINE int CUTE_CALL input_text_pop_utf32() { return cf_input_text_pop_utf32(); }
CUTE_INLINE bool CUTE_CALL input_text_has_data() { return cf_input_text_has_data(); }
CUTE_INLINE void CUTE_CALL input_text_clear() {  cf_input_text_clear(); }
	 
CUTE_INLINE void CUTE_CALL input_enable_ime() { cf_input_enable_ime(); }
CUTE_INLINE void CUTE_CALL input_disable_ime() { cf_input_disable_ime(); }
CUTE_INLINE bool CUTE_CALL input_is_ime_enabled() { return cf_input_is_ime_enabled(); }
CUTE_INLINE bool CUTE_CALL input_has_ime_keyboard_support() { return cf_input_has_ime_keyboard_support(); }
CUTE_INLINE bool CUTE_CALL input_is_ime_keyboard_shown() { return cf_input_is_ime_keyboard_shown(); }
CUTE_INLINE void CUTE_CALL input_set_ime_rect(int x, int y, int w, int h) { cf_input_set_ime_rect(x, y, w, h); }

CUTE_INLINE bool CUTE_CALL input_get_ime_composition(ime_composition_t* composition) { return cf_input_get_ime_composition(composition); }

CUTE_API cf_array<touch_t> CUTE_CALL touch_get_all();
CUTE_INLINE bool CUTE_CALL touch_get(uint64_t id, touch_t* touch) { return cf_touch_get(id,touch); }

}

#endif // CUTE_CPP

#endif // CUTE_INPUT_H
