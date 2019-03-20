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

#include <cute_input.h>
#include <cute_internal.h>
#include <cute_c_runtime.h>

#include <SDL2/SDL.h>

namespace cute
{

static int s_map_SDL_keys(int key)
{
	if (key < 128) return key;
	switch (key)
	{
		case SDLK_CAPSLOCK: return KEY_CAPSLOCK;
		case SDLK_F1: return KEY_F1;
		case SDLK_F2: return KEY_F2;
		case SDLK_F3: return KEY_F3;
		case SDLK_F4: return KEY_F4;
		case SDLK_F5: return KEY_F5;
		case SDLK_F6: return KEY_F6;
		case SDLK_F7: return KEY_F7;
		case SDLK_F8: return KEY_F8;
		case SDLK_F9: return KEY_F9;
		case SDLK_F10: return KEY_F10;
		case SDLK_F11: return KEY_F11;
		case SDLK_F12: return KEY_F12;
		case SDLK_PRINTSCREEN: return KEY_PRINTSCREEN;
		case SDLK_SCROLLLOCK: return KEY_SCROLLLOCK;
		case SDLK_PAUSE: return KEY_PAUSE;
		case SDLK_INSERT: return KEY_INSERT;
		case SDLK_HOME: return KEY_HOME;
		case SDLK_PAGEUP: return KEY_PAGEUP;
		case KEY_DELETE: return KEY_DELETE;
		case SDLK_END: return KEY_END;
		case SDLK_PAGEDOWN: return KEY_PAGEDOWN;
		case SDLK_RIGHT: return KEY_RIGHT;
		case SDLK_LEFT: return KEY_LEFT;
		case SDLK_DOWN: return KEY_DOWN;
		case SDLK_UP: return KEY_UP;
		case SDLK_NUMLOCKCLEAR: return KEY_NUMLOCKCLEAR;
		case SDLK_KP_DIVIDE: return KEY_KP_DIVIDE;
		case SDLK_KP_MULTIPLY: return KEY_KP_MULTIPLY;
		case SDLK_KP_MINUS: return KEY_KP_MINUS;
		case SDLK_KP_PLUS: return KEY_KP_PLUS;
		case SDLK_KP_ENTER: return KEY_KP_ENTER;
		case SDLK_KP_1: return KEY_KP_1;
		case SDLK_KP_2: return KEY_KP_2;
		case SDLK_KP_3: return KEY_KP_3;
		case SDLK_KP_4: return KEY_KP_4;
		case SDLK_KP_5: return KEY_KP_5;
		case SDLK_KP_6: return KEY_KP_6;
		case SDLK_KP_7: return KEY_KP_7;
		case SDLK_KP_8: return KEY_KP_8;
		case SDLK_KP_9: return KEY_KP_9;
		case SDLK_KP_0: return KEY_KP_0;
		case SDLK_KP_PERIOD: return KEY_KP_PERIOD;
		case SDLK_APPLICATION: return KEY_APPLICATION;
		case SDLK_POWER: return KEY_POWER;
		case SDLK_KP_EQUALS: return KEY_KP_EQUALS;
		case SDLK_F13: return KEY_F13;
		case SDLK_F14: return KEY_F14;
		case SDLK_F15: return KEY_F15;
		case SDLK_F16: return KEY_F16;
		case SDLK_F17: return KEY_F17;
		case SDLK_F18: return KEY_F18;
		case SDLK_F19: return KEY_F19;
		case SDLK_F20: return KEY_F20;
		case SDLK_F21: return KEY_F21;
		case SDLK_F22: return KEY_F22;
		case SDLK_F23: return KEY_F23;
		case SDLK_F24: return KEY_F24;
		case SDLK_HELP: return KEY_HELP;
		case SDLK_MENU: return KEY_MENU;
		case SDLK_SELECT: return KEY_SELECT;
		case SDLK_STOP: return KEY_STOP;
		case SDLK_AGAIN: return KEY_AGAIN;
		case SDLK_UNDO: return KEY_UNDO;
		case SDLK_CUT: return KEY_CUT;
		case SDLK_COPY: return KEY_COPY;
		case SDLK_PASTE: return KEY_PASTE;
		case SDLK_FIND: return KEY_FIND;
		case SDLK_MUTE: return KEY_MUTE;
		case SDLK_VOLUMEUP: return KEY_VOLUMEUP;
		case SDLK_VOLUMEDOWN: return KEY_VOLUMEDOWN;
		case SDLK_KP_COMMA: return KEY_KP_COMMA;
		case SDLK_KP_EQUALSAS400: return KEY_KP_EQUALSAS400;
		case SDLK_ALTERASE: return KEY_ALTERASE;
		case SDLK_SYSREQ: return KEY_SYSREQ;
		case SDLK_CANCEL: return KEY_CANCEL;
		case SDLK_CLEAR: return KEY_CLEAR;
		case SDLK_PRIOR: return KEY_PRIOR;
		case SDLK_RETURN2: return KEY_RETURN2;
		case SDLK_SEPARATOR: return KEY_SEPARATOR;
		case SDLK_OUT: return KEY_OUT;
		case SDLK_OPER: return KEY_OPER;
		case SDLK_CLEARAGAIN: return KEY_CLEARAGAIN;
		case SDLK_CRSEL: return KEY_CRSEL;
		case SDLK_EXSEL: return KEY_EXSEL;
		case SDLK_KP_00: return KEY_KP_00;
		case SDLK_KP_000: return KEY_KP_000;
		case SDLK_THOUSANDSSEPARATOR: return KEY_THOUSANDSSEPARATOR;
		case SDLK_DECIMALSEPARATOR: return KEY_DECIMALSEPARATOR;
		case SDLK_CURRENCYUNIT: return KEY_CURRENCYUNIT;
		case SDLK_CURRENCYSUBUNIT: return KEY_CURRENCYSUBUNIT;
		case SDLK_KP_LEFTPAREN: return KEY_KP_LEFTPAREN;
		case SDLK_KP_RIGHTPAREN: return KEY_KP_RIGHTPAREN;
		case SDLK_KP_LEFTBRACE: return KEY_KP_LEFTBRACE;
		case SDLK_KP_RIGHTBRACE: return KEY_KP_RIGHTBRACE;
		case SDLK_KP_TAB: return KEY_KP_TAB;
		case SDLK_KP_BACKSPACE: return KEY_KP_BACKSPACE;
		case SDLK_KP_A: return KEY_KP_A;
		case SDLK_KP_B: return KEY_KP_B;
		case SDLK_KP_C: return KEY_KP_C;
		case SDLK_KP_D: return KEY_KP_D;
		case SDLK_KP_E: return KEY_KP_E;
		case SDLK_KP_F: return KEY_KP_F;
		case SDLK_KP_XOR: return KEY_KP_XOR;
		case SDLK_KP_POWER: return KEY_KP_POWER;
		case SDLK_KP_PERCENT: return KEY_KP_PERCENT;
		case SDLK_KP_LESS: return KEY_KP_LESS;
		case SDLK_KP_GREATER: return KEY_KP_GREATER;
		case SDLK_KP_AMPERSAND: return KEY_KP_AMPERSAND;
		case SDLK_KP_DBLAMPERSAND: return KEY_KP_DBLAMPERSAND;
		case SDLK_KP_VERTICALBAR: return KEY_KP_VERTICALBAR;
		case SDLK_KP_DBLVERTICALBAR: return KEY_KP_DBLVERTICALBAR;
		case SDLK_KP_COLON: return KEY_KP_COLON;
		case SDLK_KP_HASH: return KEY_KP_HASH;
		case SDLK_KP_SPACE: return KEY_KP_SPACE;
		case SDLK_KP_AT: return KEY_KP_AT;
		case SDLK_KP_EXCLAM: return KEY_KP_EXCLAM;
		case SDLK_KP_MEMSTORE: return KEY_KP_MEMSTORE;
		case SDLK_KP_MEMRECALL: return KEY_KP_MEMRECALL;
		case SDLK_KP_MEMCLEAR: return KEY_KP_MEMCLEAR;
		case SDLK_KP_MEMADD: return KEY_KP_MEMADD;
		case SDLK_KP_MEMSUBTRACT: return KEY_KP_MEMSUBTRACT;
		case SDLK_KP_MEMMULTIPLY: return KEY_KP_MEMMULTIPLY;
		case SDLK_KP_MEMDIVIDE: return KEY_KP_MEMDIVIDE;
		case SDLK_KP_PLUSMINUS: return KEY_KP_PLUSMINUS;
		case SDLK_KP_CLEAR: return KEY_KP_CLEAR;
		case SDLK_KP_CLEARENTRY: return KEY_KP_CLEARENTRY;
		case SDLK_KP_BINARY: return KEY_KP_BINARY;
		case SDLK_KP_OCTAL: return KEY_KP_OCTAL;
		case SDLK_KP_DECIMAL: return KEY_KP_DECIMAL;
		case SDLK_KP_HEXADECIMAL: return KEY_KP_HEXADECIMAL;
		case SDLK_LCTRL: return KEY_LCTRL;
		case SDLK_LSHIFT: return KEY_LSHIFT;
		case SDLK_LALT: return KEY_LALT;
		case SDLK_LGUI: return KEY_LGUI;
		case SDLK_RCTRL: return KEY_RCTRL;
		case SDLK_RSHIFT: return KEY_RSHIFT;
		case SDLK_RALT: return KEY_RALT;
		case SDLK_RGUI: return KEY_RGUI;
		case SDLK_MODE: return KEY_MODE;
		case SDLK_AUDIONEXT: return KEY_AUDIONEXT;
		case SDLK_AUDIOPREV: return KEY_AUDIOPREV;
		case SDLK_AUDIOSTOP: return KEY_AUDIOSTOP;
		case SDLK_AUDIOPLAY: return KEY_AUDIOPLAY;
		case SDLK_AUDIOMUTE: return KEY_AUDIOMUTE;
		case SDLK_MEDIASELECT: return KEY_MEDIASELECT;
		case SDLK_WWW: return KEY_WWW;
		case SDLK_MAIL: return KEY_MAIL;
		case SDLK_CALCULATOR: return KEY_CALCULATOR;
		case SDLK_COMPUTER: return KEY_COMPUTER;
		case SDLK_AC_SEARCH: return KEY_AC_SEARCH;
		case SDLK_AC_HOME: return KEY_AC_HOME;
		case SDLK_AC_BACK: return KEY_AC_BACK;
		case SDLK_AC_FORWARD: return KEY_AC_FORWARD;
		case SDLK_AC_STOP: return KEY_AC_STOP;
		case SDLK_AC_REFRESH: return KEY_AC_REFRESH;
		case SDLK_AC_BOOKMARKS: return KEY_AC_BOOKMARKS;
		case SDLK_BRIGHTNESSDOWN: return KEY_BRIGHTNESSDOWN;
		case SDLK_BRIGHTNESSUP: return KEY_BRIGHTNESSUP;
		case SDLK_DISPLAYSWITCH: return KEY_DISPLAYSWITCH;
		case SDLK_KBDILLUMTOGGLE: return KEY_KBDILLUMTOGGLE;
		case SDLK_KBDILLUMDOWN: return KEY_KBDILLUMDOWN;
		case SDLK_KBDILLUMUP: return KEY_KBDILLUMUP;
		case SDLK_EJECT: return KEY_EJECT;
		case SDLK_SLEEP: return KEY_SLEEP;
	}
	return 0;
}

int key_is_down(app_t* app, key_button_t key)
{
	CUTE_ASSERT(key >= 0 && key < 512);
	return app->keys[key] | app->keys_prev[key];
}

int key_is_up(app_t* app, key_button_t key)
{
	CUTE_ASSERT(key >= 0 && key < 512);
	return !app->keys[key] & !app->keys_prev[key];
}

int key_was_pressed(app_t* app, key_button_t key)
{
	CUTE_ASSERT(key >= 0 && key < 512);

	float repeat_delay = 0.5f;
	float repeat_rate = 0.035f;
	float t = app->keys_duration[key];
	int repeat_count = 0;

	if (t > repeat_delay) {
		repeat_count = (int)((t - repeat_delay) / repeat_rate);
		app->keys_duration[key] -= repeat_count * repeat_rate;
	}

	return (app->keys[key] & !app->keys_prev[key]) | repeat_count;
}

int key_was_released(app_t* app, key_button_t key)
{
	CUTE_ASSERT(key >= 0 && key < 512);
	return !app->keys[key] & app->keys_prev[key];
}

int mouse_x(app_t* app)
{
	return app->mouse.x;
}

int mouse_y(app_t* app)
{
	return app->mouse.y;
}

int mouse_is_down(app_t* app, mouse_button_t button)
{
	switch (button)
	{
	case MOUSE_BUTTON_LEFT:   return app->mouse.left_button   | app->mouse_prev.left_button;
	case MOUSE_BUTTON_RIGHT:  return app->mouse.right_button  | app->mouse_prev.right_button;
	case MOUSE_BUTTON_MIDDLE: return app->mouse.middle_button | app->mouse_prev.middle_button;
	}
	return 0;
}

int mouse_is_up(app_t* app, mouse_button_t button)
{
	switch (button)
	{
	case MOUSE_BUTTON_LEFT:   return !app->mouse.left_button   & !app->mouse_prev.left_button;
	case MOUSE_BUTTON_RIGHT:  return !app->mouse.right_button  & !app->mouse_prev.right_button;
	case MOUSE_BUTTON_MIDDLE: return !app->mouse.middle_button & !app->mouse_prev.middle_button;
	}
	return 0;
}

int mouse_was_pressed(app_t* app, mouse_button_t button)
{
	switch (button)
	{
	case MOUSE_BUTTON_LEFT:   return !app->mouse.left_button   & app->mouse_prev.left_button;
	case MOUSE_BUTTON_RIGHT:  return !app->mouse.right_button  & app->mouse_prev.right_button;
	case MOUSE_BUTTON_MIDDLE: return !app->mouse.middle_button & app->mouse_prev.middle_button;
	}
	return 0;
}

int mouse_was_released(app_t* app, mouse_button_t button)
{
	switch (button)
	{
	case MOUSE_BUTTON_LEFT:   return app->mouse.left_button   & !app->mouse_prev.left_button;
	case MOUSE_BUTTON_RIGHT:  return app->mouse.right_button  & !app->mouse_prev.right_button;
	case MOUSE_BUTTON_MIDDLE: return app->mouse.middle_button & !app->mouse_prev.middle_button;
	}
	return 0;
}

int mouse_wheel_motion(app_t* app)
{
	return app->mouse.wheel_motion;
}

int mouse_double_click(app_t* app, mouse_button_t button)
{
	return mouse_is_down(app, button) && app->mouse.click_type == MOUSE_CLICK_DOUBLE;
}

void input_text_add_utf8(app_t* app, const char* text)
{
}

int input_text_pop_utf32(app_t* app)
{
	return 0;
}

int input_text_has_data(app_t* app)
{
	return 0;
}

void input_text_clear(app_t* app)
{
}

namespace internal
{
	void pump_input_msgs(app_t* app)
	{
	}
}

}
