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

#include <cute_defines.h>

namespace cute
{

enum key_button_t : int;
enum mouse_button_t : int;
enum mouse_click_t : int;

CUTE_API bool CUTE_CALL key_is_down(app_t* app, key_button_t key);
CUTE_API bool CUTE_CALL key_is_up(app_t* app, key_button_t key);
CUTE_API bool CUTE_CALL key_was_pressed(app_t* app, key_button_t key);
CUTE_API bool CUTE_CALL key_was_released(app_t* app, key_button_t key);
CUTE_API int CUTE_CALL key_mod_bit_flags(app_t* app);

CUTE_API int CUTE_CALL mouse_x(app_t* app);
CUTE_API int CUTE_CALL mouse_y(app_t* app);
CUTE_API struct v2 CUTE_CALL mouse_pos_in_world_space(app_t* app);

CUTE_API bool CUTE_CALL mouse_is_down(app_t* app, mouse_button_t button);
CUTE_API bool CUTE_CALL mouse_is_up(app_t* app, mouse_button_t button);
CUTE_API bool CUTE_CALL mouse_was_pressed(app_t* app, mouse_button_t button);
CUTE_API bool CUTE_CALL mouse_was_released(app_t* app, mouse_button_t button);
CUTE_API int CUTE_CALL mouse_wheel_motion(app_t* app);
CUTE_API bool CUTE_CALL mouse_is_down_double_click(app_t* app, mouse_button_t button);
CUTE_API bool CUTE_CALL mouse_double_click_was_pressed(app_t* app, mouse_button_t button);

CUTE_API void CUTE_CALL input_text_add_utf8(app_t* app, const char* text);
CUTE_API int CUTE_CALL input_text_pop_utf32(app_t* app);
CUTE_API bool CUTE_CALL input_text_has_data(app_t* app);
CUTE_API void CUTE_CALL input_text_clear(app_t* app);

CUTE_API void CUTE_CALL input_enable_ime();
CUTE_API void CUTE_CALL input_disable_ime();
CUTE_API bool CUTE_CALL input_is_ime_enabled();
CUTE_API bool CUTE_CALL input_has_ime_keyboard_support();
CUTE_API bool CUTE_CALL input_is_ime_keyboard_shown(app_t* app);
CUTE_API void CUTE_CALL input_set_ime_rect(int x, int y, int w, int h);

struct ime_composition_t
{
	const char* composition;
	int cursor;
	int selection_len;
};

CUTE_API bool CUTE_CALL input_get_ime_composition(app_t* app, ime_composition_t* composition);

enum mouse_button_t : int
{
	MOUSE_BUTTON_LEFT,
	MOUSE_BUTTON_RIGHT,
	MOUSE_BUTTON_MIDDLE
};

enum key_button_t : int
{
	KEY_UNKNOWN = 0,

	KEY_RETURN = '\r',
	KEY_ESCAPE = '\033',
	KEY_BACKSPACE = '\b',
	KEY_TAB = '\t',
	KEY_SPACE = ' ',
	KEY_EXCLAIM = '!',
	KEY_QUOTEDBL = '"',
	KEY_HASH = '#',
	KEY_PERCENT = '%',
	KEY_DOLLAR = '$',
	KEY_AMPERSAND = '&',
	KEY_QUOTE = '\'',
	KEY_LEFTPAREN = '(',
	KEY_RIGHTPAREN = ')',
	KEY_ASTERISK = '*',
	KEY_PLUS = '+',
	KEY_COMMA = ',',
	KEY_MINUS = '-',
	KEY_PERIOD = '.',
	KEY_SLASH = '/',
	KEY_0 = '0',
	KEY_1 = '1',
	KEY_2 = '2',
	KEY_3 = '3',
	KEY_4 = '4',
	KEY_5 = '5',
	KEY_6 = '6',
	KEY_7 = '7',
	KEY_8 = '8',
	KEY_9 = '9',
	KEY_COLON = ':',
	KEY_SEMICOLON = ';',
	KEY_LESS = '<',
	KEY_EQUALS = '=',
	KEY_GREATER = '>',
	KEY_QUESTION = '?',
	KEY_AT = '@',
	// Skip uppercase letters
	KEY_LEFTBRACKET = '[',
	KEY_BACKSLASH = '\\',
	KEY_RIGHTBRACKET = ']',
	KEY_CARET = '^',
	KEY_UNDERSCORE = '_',
	KEY_BACKQUOTE = '`',
	KEY_A = 'a',
	KEY_B = 'b',
	KEY_C = 'c',
	KEY_D = 'd',
	KEY_E = 'e',
	KEY_F = 'f',
	KEY_G = 'g',
	KEY_H = 'h',
	KEY_I = 'i',
	KEY_J = 'j',
	KEY_K = 'k',
	KEY_L = 'l',
	KEY_M = 'm',
	KEY_N = 'n',
	KEY_O = 'o',
	KEY_P = 'p',
	KEY_Q = 'q',
	KEY_R = 'r',
	KEY_S = 's',
	KEY_T = 't',
	KEY_U = 'u',
	KEY_V = 'v',
	KEY_W = 'w',
	KEY_X = 'x',
	KEY_Y = 'y',
	KEY_Z = 'z',

	KEY_CAPSLOCK,

	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,

	KEY_PRINTSCREEN,
	KEY_SCROLLLOCK,
	KEY_PAUSE,
	KEY_INSERT,
	KEY_HOME,
	KEY_PAGEUP,
	KEY_DELETE,
	KEY_END,
	KEY_PAGEDOWN,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_DOWN,
	KEY_UP,

	KEY_NUMLOCKCLEAR,
	KEY_KP_DIVIDE,
	KEY_KP_MULTIPLY,
	KEY_KP_MINUS,
	KEY_KP_PLUS,
	KEY_KP_ENTER,
	KEY_KP_1,
	KEY_KP_2,
	KEY_KP_3,
	KEY_KP_4,
	KEY_KP_5,
	KEY_KP_6,
	KEY_KP_7,
	KEY_KP_8,
	KEY_KP_9,
	KEY_KP_0,
	KEY_KP_PERIOD,
	KEY_APPLICATION,
	KEY_POWER,
	KEY_KP_EQUALS,

	KEY_F13,
	KEY_F14,
	KEY_F15,
	KEY_F16,
	KEY_F17,
	KEY_F18,
	KEY_F19,
	KEY_F20,
	KEY_F21,
	KEY_F22,
	KEY_F23,
	KEY_F24,
	KEY_HELP,
	KEY_MENU,
	KEY_SELECT,
	KEY_STOP,
	KEY_AGAIN,
	KEY_UNDO,
	KEY_CUT,
	KEY_COPY,
	KEY_PASTE,
	KEY_FIND,
	KEY_MUTE,
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,

	KEY_KP_COMMA,
	KEY_KP_EQUALSAS400,
	KEY_ALTERASE,
	KEY_SYSREQ,
	KEY_CANCEL,
	KEY_CLEAR,
	KEY_PRIOR,
	KEY_RETURN2,
	KEY_SEPARATOR,
	KEY_OUT,
	KEY_OPER,
	KEY_CLEARAGAIN,

	KEY_CRSEL,
	KEY_EXSEL,
	KEY_KP_00,
	KEY_KP_000,
	KEY_THOUSANDSSEPARATOR,
	KEY_DECIMALSEPARATOR,
	KEY_CURRENCYUNIT,
	KEY_CURRENCYSUBUNIT,
	KEY_KP_LEFTPAREN,
	KEY_KP_RIGHTPAREN,
	KEY_KP_LEFTBRACE,
	KEY_KP_RIGHTBRACE,
	KEY_KP_TAB,
	KEY_KP_BACKSPACE,
	KEY_KP_A,
	KEY_KP_B,
	KEY_KP_C,
	KEY_KP_D,
	KEY_KP_E,
	KEY_KP_F,
	KEY_KP_XOR,
	KEY_KP_POWER,
	KEY_KP_PERCENT,
	KEY_KP_LESS,
	KEY_KP_GREATER,
	KEY_KP_AMPERSAND,
	KEY_KP_DBLAMPERSAND,
	KEY_KP_VERTICALBAR,
	KEY_KP_DBLVERTICALBAR,
	KEY_KP_COLON,
	KEY_KP_HASH,
	KEY_KP_SPACE,
	KEY_KP_AT,
	KEY_KP_EXCLAM,
	KEY_KP_MEMSTORE,
	KEY_KP_MEMRECALL,
	KEY_KP_MEMCLEAR,
	KEY_KP_MEMADD,
	KEY_KP_MEMSUBTRACT,
	KEY_KP_MEMMULTIPLY,
	KEY_KP_MEMDIVIDE,
	KEY_KP_PLUSMINUS,
	KEY_KP_CLEAR,
	KEY_KP_CLEARENTRY,
	KEY_KP_BINARY,
	KEY_KP_OCTAL,
	KEY_KP_DECIMAL,
	KEY_KP_HEXADECIMAL,

	KEY_LCTRL,
	KEY_LSHIFT,
	KEY_LALT,
	KEY_LGUI,
	KEY_RCTRL,
	KEY_RSHIFT,
	KEY_RALT,
	KEY_RGUI,

	KEY_MODE,

	KEY_AUDIONEXT,
	KEY_AUDIOPREV,
	KEY_AUDIOSTOP,
	KEY_AUDIOPLAY,
	KEY_AUDIOMUTE,
	KEY_MEDIASELECT,
	KEY_WWW,
	KEY_MAIL,
	KEY_CALCULATOR,
	KEY_COMPUTER,
	KEY_AC_SEARCH,
	KEY_AC_HOME,
	KEY_AC_BACK,
	KEY_AC_FORWARD,
	KEY_AC_STOP,
	KEY_AC_REFRESH,
	KEY_AC_BOOKMARKS,
	KEY_BRIGHTNESSDOWN,
	KEY_BRIGHTNESSUP,
	KEY_DISPLAYSWITCH,
	KEY_KBDILLUMTOGGLE,
	KEY_KBDILLUMDOWN,
	KEY_KBDILLUMUP,
	KEY_EJECT,
	KEY_SLEEP,

	KEY_COUNT
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
