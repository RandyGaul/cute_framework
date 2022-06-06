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

namespace cute
{

enum cf_key_button_t;// : int;
enum cf_mouse_button_t;// : int;
enum cf_mouse_click_t;// : int;

CUTE_API bool CUTE_CALL cf_key_is_down(cf_key_button_t key);
CUTE_API bool CUTE_CALL cf_key_is_up(cf_key_button_t key);
CUTE_API bool CUTE_CALL cf_key_was_pressed(cf_key_button_t key);
CUTE_API bool CUTE_CALL cf_key_was_released(cf_key_button_t key);
CUTE_API void CUTE_CALL cf_clear_all_key_state();
CUTE_API int CUTE_CALL cf_key_mod_bit_flags();

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

struct cf_ime_composition_t
{
	const char* composition;
	int cursor;
	int selection_len;
};

CUTE_API bool CUTE_CALL cf_input_get_ime_composition(cf_ime_composition_t* composition);

struct cf_touch_t
{
	uint64_t id;
	float x;
	float y;
	float pressure;
};

CUTE_API cf_array<cf_touch_t> CUTE_CALL cf_touch_get_all();
CUTE_API bool CUTE_CALL cf_touch_get(uint64_t id, cf_touch_t* touch);

enum cf_mouse_button_t : int
{
	CF_MOUSE_BUTTON_LEFT,
	CF_MOUSE_BUTTON_RIGHT,
	CF_MOUSE_BUTTON_MIDDLE
};

enum cf_key_button_t : int
{
	CF_KEY_UNKNOWN = 0,

	CF_KEY_RETURN = '\r',
	CF_KEY_ESCAPE = '\033',
	CF_KEY_BACKSPACE = '\b',
	CF_KEY_TAB = '\t',
	CF_KEY_SPACE = ' ',
	CF_KEY_EXCLAIM = '!',
	CF_KEY_QUOTEDBL = '"',
	CF_KEY_HASH = '#',
	CF_KEY_PERCENT = '%',
	CF_KEY_DOLLAR = '$',
	CF_KEY_AMPERSAND = '&',
	CF_KEY_QUOTE = '\'',
	CF_KEY_LEFTPAREN = '(',
	CF_KEY_RIGHTPAREN = ')',
	CF_KEY_ASTERISK = '*',
	CF_KEY_PLUS = '+',
	CF_KEY_COMMA = ',',
	CF_KEY_MINUS = '-',
	CF_KEY_PERIOD = '.',
	CF_KEY_SLASH = '/',
	CF_KEY_0 = '0',
	CF_KEY_1 = '1',
	CF_KEY_2 = '2',
	CF_KEY_3 = '3',
	CF_KEY_4 = '4',
	CF_KEY_5 = '5',
	CF_KEY_6 = '6',
	CF_KEY_7 = '7',
	CF_KEY_8 = '8',
	CF_KEY_9 = '9',
	CF_KEY_COLON = ':',
	CF_KEY_SEMICOLON = ';',
	CF_KEY_LESS = '<',
	CF_KEY_EQUALS = '=',
	CF_KEY_GREATER = '>',
	CF_KEY_QUESTION = '?',
	CF_KEY_AT = '@',
	// Skip uppercase letters
	CF_KEY_LEFTBRACKET = '[',
	CF_KEY_BACKSLASH = '\\',
	CF_KEY_RIGHTBRACKET = ']',
	CF_KEY_CARET = '^',
	CF_KEY_UNDERSCORE = '_',
	CF_KEY_BACKQUOTE = '`',
	CF_KEY_A = 'a',
	CF_KEY_B = 'b',
	CF_KEY_C = 'c',
	CF_KEY_D = 'd',
	CF_KEY_E = 'e',
	CF_KEY_F = 'f',
	CF_KEY_G = 'g',
	CF_KEY_H = 'h',
	CF_KEY_I = 'i',
	CF_KEY_J = 'j',
	CF_KEY_K = 'k',
	CF_KEY_L = 'l',
	CF_KEY_M = 'm',
	CF_KEY_N = 'n',
	CF_KEY_O = 'o',
	CF_KEY_P = 'p',
	CF_KEY_Q = 'q',
	CF_KEY_R = 'r',
	CF_KEY_S = 's',
	CF_KEY_T = 't',
	CF_KEY_U = 'u',
	CF_KEY_V = 'v',
	CF_KEY_W = 'w',
	CF_KEY_X = 'x',
	CF_KEY_Y = 'y',
	CF_KEY_Z = 'z',

	CF_KEY_CAPSLOCK,

	CF_KEY_F1,
	CF_KEY_F2,
	CF_KEY_F3,
	CF_KEY_F4,
	CF_KEY_F5,
	CF_KEY_F6,
	CF_KEY_F7,
	CF_KEY_F8,
	CF_KEY_F9,
	CF_KEY_F10,
	CF_KEY_F11,
	CF_KEY_F12,

	CF_KEY_PRINTSCREEN,
	CF_KEY_SCROLLLOCK,
	CF_KEY_PAUSE,
	CF_KEY_INSERT,
	CF_KEY_HOME,
	CF_KEY_PAGEUP,
	CF_KEY_DELETE,
	CF_KEY_END,
	CF_KEY_PAGEDOWN,
	CF_KEY_RIGHT,
	CF_KEY_LEFT,
	CF_KEY_DOWN,
	CF_KEY_UP,

	CF_KEY_NUMLOCKCLEAR,
	CF_KEY_KP_DIVIDE,
	CF_KEY_KP_MULTIPLY,
	CF_KEY_KP_MINUS,
	CF_KEY_KP_PLUS,
	CF_KEY_KP_ENTER,
	CF_KEY_KP_1,
	CF_KEY_KP_2,
	CF_KEY_KP_3,
	CF_KEY_KP_4,
	CF_KEY_KP_5,
	CF_KEY_KP_6,
	CF_KEY_KP_7,
	CF_KEY_KP_8,
	CF_KEY_KP_9,
	CF_KEY_KP_0,
	CF_KEY_KP_PERIOD,
	CF_KEY_APPLICATION,
	CF_KEY_POWER,
	CF_KEY_KP_EQUALS,

	CF_KEY_F13,
	CF_KEY_F14,
	CF_KEY_F15,
	CF_KEY_F16,
	CF_KEY_F17,
	CF_KEY_F18,
	CF_KEY_F19,
	CF_KEY_F20,
	CF_KEY_F21,
	CF_KEY_F22,
	CF_KEY_F23,
	CF_KEY_F24,
	CF_KEY_HELP,
	CF_KEY_MENU,
	CF_KEY_SELECT,
	CF_KEY_STOP,
	CF_KEY_AGAIN,
	CF_KEY_UNDO,
	CF_KEY_CUT,
	CF_KEY_COPY,
	CF_KEY_PASTE,
	CF_KEY_FIND,
	CF_KEY_MUTE,
	CF_KEY_VOLUMEUP,
	CF_KEY_VOLUMEDOWN,

	CF_KEY_KP_COMMA,
	CF_KEY_KP_EQUALSAS400,
	CF_KEY_ALTERASE,
	CF_KEY_SYSREQ,
	CF_KEY_CANCEL,
	CF_KEY_CLEAR,
	CF_KEY_PRIOR,
	CF_KEY_RETURN2,
	CF_KEY_SEPARATOR,
	CF_KEY_OUT,
	CF_KEY_OPER,
	CF_KEY_CLEARAGAIN,

	CF_KEY_CRSEL,
	CF_KEY_EXSEL,
	CF_KEY_KP_00,
	CF_KEY_KP_000,
	CF_KEY_THOUSANDSSEPARATOR,
	CF_KEY_DECIMALSEPARATOR,
	CF_KEY_CURRENCYUNIT,
	CF_KEY_CURRENCYSUBUNIT,
	CF_KEY_KP_LEFTPAREN,
	CF_KEY_KP_RIGHTPAREN,
	CF_KEY_KP_LEFTBRACE,
	CF_KEY_KP_RIGHTBRACE,
	CF_KEY_KP_TAB,
	CF_KEY_KP_BACKSPACE,
	CF_KEY_KP_A,
	CF_KEY_KP_B,
	CF_KEY_KP_C,
	CF_KEY_KP_D,
	CF_KEY_KP_E,
	CF_KEY_KP_F,
	CF_KEY_KP_XOR,
	CF_KEY_KP_POWER,
	CF_KEY_KP_PERCENT,
	CF_KEY_KP_LESS,
	CF_KEY_KP_GREATER,
	CF_KEY_KP_AMPERSAND,
	CF_KEY_KP_DBLAMPERSAND,
	CF_KEY_KP_VERTICALBAR,
	CF_KEY_KP_DBLVERTICALBAR,
	CF_KEY_KP_COLON,
	CF_KEY_KP_HASH,
	CF_KEY_KP_SPACE,
	CF_KEY_KP_AT,
	CF_KEY_KP_EXCLAM,
	CF_KEY_KP_MEMSTORE,
	CF_KEY_KP_MEMRECALL,
	CF_KEY_KP_MEMCLEAR,
	CF_KEY_KP_MEMADD,
	CF_KEY_KP_MEMSUBTRACT,
	CF_KEY_KP_MEMMULTIPLY,
	CF_KEY_KP_MEMDIVIDE,
	CF_KEY_KP_PLUSMINUS,
	CF_KEY_KP_CLEAR,
	CF_KEY_KP_CLEARENTRY,
	CF_KEY_KP_BINARY,
	CF_KEY_KP_OCTAL,
	CF_KEY_KP_DECIMAL,
	CF_KEY_KP_HEXADECIMAL,

	CF_KEY_LCTRL,
	CF_KEY_LSHIFT,
	CF_KEY_LALT,
	CF_KEY_LGUI,
	CF_KEY_RCTRL,
	CF_KEY_RSHIFT,
	CF_KEY_RALT,
	CF_KEY_RGUI,

	CF_KEY_MODE,

	CF_KEY_AUDIONEXT,
	CF_KEY_AUDIOPREV,
	CF_KEY_AUDIOSTOP,
	CF_KEY_AUDIOPLAY,
	CF_KEY_AUDIOMUTE,
	CF_KEY_MEDIASELECT,
	CF_KEY_WWW,
	CF_KEY_MAIL,
	CF_KEY_CALCULATOR,
	CF_KEY_COMPUTER,
	CF_KEY_AC_SEARCH,
	CF_KEY_AC_HOME,
	CF_KEY_AC_BACK,
	CF_KEY_AC_FORWARD,
	CF_KEY_AC_STOP,
	CF_KEY_AC_REFRESH,
	CF_KEY_AC_BOOKMARKS,
	CF_KEY_BRIGHTNESSDOWN,
	CF_KEY_BRIGHTNESSUP,
	CF_KEY_DISPLAYSWITCH,
	CF_KEY_KBDILLUMTOGGLE,
	CF_KEY_KBDILLUMDOWN,
	CF_KEY_KBDILLUMUP,
	CF_KEY_EJECT,
	CF_KEY_SLEEP,

	CF_KEY_ANY,

	CF_KEY_COUNT
};

#define CUTE_KEY_MOD_NONE     0x0000
#define CUTE_KEY_MOD_LSHIFT   0x0001
#define CUTE_KEY_MOD_RSHIFT   0x0002
#define CUTE_KEY_MOD_LCTRL    0x0040
#define CUTE_KEY_MOD_RCTRL    0x0080
#define CUTE_KEY_MOD_LALT     0x0100
#define CUTE_KEY_MOD_RALT     0x0200
#define CUTE_KEY_MOD_LGUI     0x0400
#define CUTE_KEY_MOD_RGUI     0x0800
#define CUTE_KEY_MOD_NUMLOCK  0x1000
#define CUTE_KEY_MOD_CAPSLOCK 0x2000

#define CUTE_KEY_MOD_CTRL  (CUTE_KEY_MOD_LCTRL | CUTE_KEY_MOD_RCTRL)
#define CUTE_KEY_MOD_SHIFT (CUTE_KEY_MOD_LSHIFT | CUTE_KEY_MOD_RSHIFT)
#define CUTE_KEY_MOD_ALT   (CUTE_KEY_MOD_LALT | CUTE_KEY_MOD_RALT)
#define CUTE_KEY_MOD_GUI   (CUTE_KEY_MOD_LGUI | CUTE_KEY_MOD_RGUI)

}

#endif // CUTE_INPUT_H
