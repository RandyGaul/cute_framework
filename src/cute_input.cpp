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
#include <cute_c_runtime.h>
#include <cute_math.h>
#include <cute_utf8.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_input_internal.h>
#include <internal/imgui/imgui_impl_sdl.h>

#include <SDL.h>

static int cf_s_map_SDL_keys(int key)
{
	if (key < 127) return key;
	switch (key)
	{
		case SDLK_CAPSLOCK: return CF_KEY_CAPSLOCK;
		case SDLK_F1: return CF_KEY_F1;
		case SDLK_F2: return CF_KEY_F2;
		case SDLK_F3: return CF_KEY_F3;
		case SDLK_F4: return CF_KEY_F4;
		case SDLK_F5: return CF_KEY_F5;
		case SDLK_F6: return CF_KEY_F6;
		case SDLK_F7: return CF_KEY_F7;
		case SDLK_F8: return CF_KEY_F8;
		case SDLK_F9: return CF_KEY_F9;
		case SDLK_F10: return CF_KEY_F10;
		case SDLK_F11: return CF_KEY_F11;
		case SDLK_F12: return CF_KEY_F12;
		case SDLK_PRINTSCREEN: return CF_KEY_PRINTSCREEN;
		case SDLK_SCROLLLOCK: return CF_KEY_SCROLLLOCK;
		case SDLK_PAUSE: return CF_KEY_PAUSE;
		case SDLK_INSERT: return CF_KEY_INSERT;
		case SDLK_HOME: return CF_KEY_HOME;
		case SDLK_PAGEUP: return CF_KEY_PAGEUP;
		case SDLK_DELETE: return CF_KEY_DELETE;
		case SDLK_END: return CF_KEY_END;
		case SDLK_PAGEDOWN: return CF_KEY_PAGEDOWN;
		case SDLK_RIGHT: return CF_KEY_RIGHT;
		case SDLK_LEFT: return CF_KEY_LEFT;
		case SDLK_DOWN: return CF_KEY_DOWN;
		case SDLK_UP: return CF_KEY_UP;
		case SDLK_NUMLOCKCLEAR: return CF_KEY_NUMLOCKCLEAR;
		case SDLK_KP_DIVIDE: return CF_KEY_KP_DIVIDE;
		case SDLK_KP_MULTIPLY: return CF_KEY_KP_MULTIPLY;
		case SDLK_KP_MINUS: return CF_KEY_KP_MINUS;
		case SDLK_KP_PLUS: return CF_KEY_KP_PLUS;
		case SDLK_KP_ENTER: return CF_KEY_KP_ENTER;
		case SDLK_KP_1: return CF_KEY_KP_1;
		case SDLK_KP_2: return CF_KEY_KP_2;
		case SDLK_KP_3: return CF_KEY_KP_3;
		case SDLK_KP_4: return CF_KEY_KP_4;
		case SDLK_KP_5: return CF_KEY_KP_5;
		case SDLK_KP_6: return CF_KEY_KP_6;
		case SDLK_KP_7: return CF_KEY_KP_7;
		case SDLK_KP_8: return CF_KEY_KP_8;
		case SDLK_KP_9: return CF_KEY_KP_9;
		case SDLK_KP_0: return CF_KEY_KP_0;
		case SDLK_KP_PERIOD: return CF_KEY_KP_PERIOD;
		case SDLK_APPLICATION: return CF_KEY_APPLICATION;
		case SDLK_POWER: return CF_KEY_POWER;
		case SDLK_KP_EQUALS: return CF_KEY_KP_EQUALS;
		case SDLK_F13: return CF_KEY_F13;
		case SDLK_F14: return CF_KEY_F14;
		case SDLK_F15: return CF_KEY_F15;
		case SDLK_F16: return CF_KEY_F16;
		case SDLK_F17: return CF_KEY_F17;
		case SDLK_F18: return CF_KEY_F18;
		case SDLK_F19: return CF_KEY_F19;
		case SDLK_F20: return CF_KEY_F20;
		case SDLK_F21: return CF_KEY_F21;
		case SDLK_F22: return CF_KEY_F22;
		case SDLK_F23: return CF_KEY_F23;
		case SDLK_F24: return CF_KEY_F24;
		case SDLK_HELP: return CF_KEY_HELP;
		case SDLK_MENU: return CF_KEY_MENU;
		case SDLK_SELECT: return CF_KEY_SELECT;
		case SDLK_STOP: return CF_KEY_STOP;
		case SDLK_AGAIN: return CF_KEY_AGAIN;
		case SDLK_UNDO: return CF_KEY_UNDO;
		case SDLK_CUT: return CF_KEY_CUT;
		case SDLK_COPY: return CF_KEY_COPY;
		case SDLK_PASTE: return CF_KEY_PASTE;
		case SDLK_FIND: return CF_KEY_FIND;
		case SDLK_MUTE: return CF_KEY_MUTE;
		case SDLK_VOLUMEUP: return CF_KEY_VOLUMEUP;
		case SDLK_VOLUMEDOWN: return CF_KEY_VOLUMEDOWN;
		case SDLK_KP_COMMA: return CF_KEY_KP_COMMA;
		case SDLK_KP_EQUALSAS400: return CF_KEY_KP_EQUALSAS400;
		case SDLK_ALTERASE: return CF_KEY_ALTERASE;
		case SDLK_SYSREQ: return CF_KEY_SYSREQ;
		case SDLK_CANCEL: return CF_KEY_CANCEL;
		case SDLK_CLEAR: return CF_KEY_CLEAR;
		case SDLK_PRIOR: return CF_KEY_PRIOR;
		case SDLK_RETURN2: return CF_KEY_RETURN2;
		case SDLK_SEPARATOR: return CF_KEY_SEPARATOR;
		case SDLK_OUT: return CF_KEY_OUT;
		case SDLK_OPER: return CF_KEY_OPER;
		case SDLK_CLEARAGAIN: return CF_KEY_CLEARAGAIN;
		case SDLK_CRSEL: return CF_KEY_CRSEL;
		case SDLK_EXSEL: return CF_KEY_EXSEL;
		case SDLK_KP_00: return CF_KEY_KP_00;
		case SDLK_KP_000: return CF_KEY_KP_000;
		case SDLK_THOUSANDSSEPARATOR: return CF_KEY_THOUSANDSSEPARATOR;
		case SDLK_DECIMALSEPARATOR: return CF_KEY_DECIMALSEPARATOR;
		case SDLK_CURRENCYUNIT: return CF_KEY_CURRENCYUNIT;
		case SDLK_CURRENCYSUBUNIT: return CF_KEY_CURRENCYSUBUNIT;
		case SDLK_KP_LEFTPAREN: return CF_KEY_KP_LEFTPAREN;
		case SDLK_KP_RIGHTPAREN: return CF_KEY_KP_RIGHTPAREN;
		case SDLK_KP_LEFTBRACE: return CF_KEY_KP_LEFTBRACE;
		case SDLK_KP_RIGHTBRACE: return CF_KEY_KP_RIGHTBRACE;
		case SDLK_KP_TAB: return CF_KEY_KP_TAB;
		case SDLK_KP_BACKSPACE: return CF_KEY_KP_BACKSPACE;
		case SDLK_KP_A: return CF_KEY_KP_A;
		case SDLK_KP_B: return CF_KEY_KP_B;
		case SDLK_KP_C: return CF_KEY_KP_C;
		case SDLK_KP_D: return CF_KEY_KP_D;
		case SDLK_KP_E: return CF_KEY_KP_E;
		case SDLK_KP_F: return CF_KEY_KP_F;
		case SDLK_KP_XOR: return CF_KEY_KP_XOR;
		case SDLK_KP_POWER: return CF_KEY_KP_POWER;
		case SDLK_KP_PERCENT: return CF_KEY_KP_PERCENT;
		case SDLK_KP_LESS: return CF_KEY_KP_LESS;
		case SDLK_KP_GREATER: return CF_KEY_KP_GREATER;
		case SDLK_KP_AMPERSAND: return CF_KEY_KP_AMPERSAND;
		case SDLK_KP_DBLAMPERSAND: return CF_KEY_KP_DBLAMPERSAND;
		case SDLK_KP_VERTICALBAR: return CF_KEY_KP_VERTICALBAR;
		case SDLK_KP_DBLVERTICALBAR: return CF_KEY_KP_DBLVERTICALBAR;
		case SDLK_KP_COLON: return CF_KEY_KP_COLON;
		case SDLK_KP_HASH: return CF_KEY_KP_HASH;
		case SDLK_KP_SPACE: return CF_KEY_KP_SPACE;
		case SDLK_KP_AT: return CF_KEY_KP_AT;
		case SDLK_KP_EXCLAM: return CF_KEY_KP_EXCLAM;
		case SDLK_KP_MEMSTORE: return CF_KEY_KP_MEMSTORE;
		case SDLK_KP_MEMRECALL: return CF_KEY_KP_MEMRECALL;
		case SDLK_KP_MEMCLEAR: return CF_KEY_KP_MEMCLEAR;
		case SDLK_KP_MEMADD: return CF_KEY_KP_MEMADD;
		case SDLK_KP_MEMSUBTRACT: return CF_KEY_KP_MEMSUBTRACT;
		case SDLK_KP_MEMMULTIPLY: return CF_KEY_KP_MEMMULTIPLY;
		case SDLK_KP_MEMDIVIDE: return CF_KEY_KP_MEMDIVIDE;
		case SDLK_KP_PLUSMINUS: return CF_KEY_KP_PLUSMINUS;
		case SDLK_KP_CLEAR: return CF_KEY_KP_CLEAR;
		case SDLK_KP_CLEARENTRY: return CF_KEY_KP_CLEARENTRY;
		case SDLK_KP_BINARY: return CF_KEY_KP_BINARY;
		case SDLK_KP_OCTAL: return CF_KEY_KP_OCTAL;
		case SDLK_KP_DECIMAL: return CF_KEY_KP_DECIMAL;
		case SDLK_KP_HEXADECIMAL: return CF_KEY_KP_HEXADECIMAL;
		case SDLK_LCTRL: return CF_KEY_LCTRL;
		case SDLK_LSHIFT: return CF_KEY_LSHIFT;
		case SDLK_LALT: return CF_KEY_LALT;
		case SDLK_LGUI: return CF_KEY_LGUI;
		case SDLK_RCTRL: return CF_KEY_RCTRL;
		case SDLK_RSHIFT: return CF_KEY_RSHIFT;
		case SDLK_RALT: return CF_KEY_RALT;
		case SDLK_RGUI: return CF_KEY_RGUI;
		case SDLK_MODE: return CF_KEY_MODE;
		case SDLK_AUDIONEXT: return CF_KEY_AUDIONEXT;
		case SDLK_AUDIOPREV: return CF_KEY_AUDIOPREV;
		case SDLK_AUDIOSTOP: return CF_KEY_AUDIOSTOP;
		case SDLK_AUDIOPLAY: return CF_KEY_AUDIOPLAY;
		case SDLK_AUDIOMUTE: return CF_KEY_AUDIOMUTE;
		case SDLK_MEDIASELECT: return CF_KEY_MEDIASELECT;
		case SDLK_WWW: return CF_KEY_WWW;
		case SDLK_MAIL: return CF_KEY_MAIL;
		case SDLK_CALCULATOR: return CF_KEY_CALCULATOR;
		case SDLK_COMPUTER: return CF_KEY_COMPUTER;
		case SDLK_AC_SEARCH: return CF_KEY_AC_SEARCH;
		case SDLK_AC_HOME: return CF_KEY_AC_HOME;
		case SDLK_AC_BACK: return CF_KEY_AC_BACK;
		case SDLK_AC_FORWARD: return CF_KEY_AC_FORWARD;
		case SDLK_AC_STOP: return CF_KEY_AC_STOP;
		case SDLK_AC_REFRESH: return CF_KEY_AC_REFRESH;
		case SDLK_AC_BOOKMARKS: return CF_KEY_AC_BOOKMARKS;
		case SDLK_BRIGHTNESSDOWN: return CF_KEY_BRIGHTNESSDOWN;
		case SDLK_BRIGHTNESSUP: return CF_KEY_BRIGHTNESSUP;
		case SDLK_DISPLAYSWITCH: return CF_KEY_DISPLAYSWITCH;
		case SDLK_KBDILLUMTOGGLE: return CF_KEY_KBDILLUMTOGGLE;
		case SDLK_KBDILLUMDOWN: return CF_KEY_KBDILLUMDOWN;
		case SDLK_KBDILLUMUP: return CF_KEY_KBDILLUMUP;
		case SDLK_EJECT: return CF_KEY_EJECT;
		case SDLK_SLEEP: return CF_KEY_SLEEP;
	}
	return 0;
}

bool cf_key_is_down(cf_key_button_t key)
{
	CUTE_ASSERT(key >= 0 && key < 512);
	return cf_app->keys[key];
}

bool cf_key_is_up(cf_key_button_t key)
{
	CUTE_ASSERT(key >= 0 && key < 512);
	return !cf_app->keys[key];
}

bool cf_key_was_pressed(cf_key_button_t key)
{
	CUTE_ASSERT(key >= 0 && key < 512);

	float repeat_delay = 0.5f;
	float repeat_rate = 0.035f;
	float t = cf_app->keys_duration[key];
	int repeat_count = 0;

	if (t > repeat_delay) {
		repeat_count = (int)((t - repeat_delay) / repeat_rate);
		cf_app->keys_duration[key] -= repeat_count * repeat_rate;
	}

	return (cf_app->keys[key] & !cf_app->keys_prev[key]) | repeat_count;
}

bool cf_key_was_released(cf_key_button_t key)
{
	CUTE_ASSERT(key >= 0 && key < 512);
	return !cf_app->keys[key] && cf_app->keys_prev[key];
}

void cf_clear_all_key_state()
{
	CUTE_MEMSET(cf_app->keys, 0, sizeof(cf_app->keys));
	CUTE_MEMSET(cf_app->keys_prev, 0, sizeof(cf_app->keys_prev));
}

int cf_key_mod_bit_flags()
{
	return cf_app->key_mod;
}

int cf_mouse_x()
{
	return cf_app->mouse.x;
}

int cf_mouse_y()
{
	return cf_app->mouse.y;
}

bool cf_mouse_is_down(cf_mouse_button_t button)
{
	switch (button)
	{
	case CF_MOUSE_BUTTON_LEFT:   return cf_app->mouse.left_button;
	case CF_MOUSE_BUTTON_RIGHT:  return cf_app->mouse.right_button;
	case CF_MOUSE_BUTTON_MIDDLE: return cf_app->mouse.middle_button;
	}
	return 0;
}

bool cf_mouse_is_up(cf_mouse_button_t button)
{
	switch (button)
	{
	case CF_MOUSE_BUTTON_LEFT:   return !cf_app->mouse.left_button;
	case CF_MOUSE_BUTTON_RIGHT:  return !cf_app->mouse.right_button;
	case CF_MOUSE_BUTTON_MIDDLE: return !cf_app->mouse.middle_button;
	}
	return 0;
}

bool cf_mouse_was_pressed(cf_mouse_button_t button)
{
	switch (button)
	{
	case CF_MOUSE_BUTTON_LEFT:   return cf_app->mouse.left_button   && !cf_app->mouse_prev.left_button;
	case CF_MOUSE_BUTTON_RIGHT:  return cf_app->mouse.right_button  && !cf_app->mouse_prev.right_button;
	case CF_MOUSE_BUTTON_MIDDLE: return cf_app->mouse.middle_button && !cf_app->mouse_prev.middle_button;
	}
	return 0;
}

bool cf_mouse_was_released(cf_mouse_button_t button)
{
	switch (button)
	{
	case CF_MOUSE_BUTTON_LEFT:   return !cf_app->mouse.left_button   && cf_app->mouse_prev.left_button;
	case CF_MOUSE_BUTTON_RIGHT:  return !cf_app->mouse.right_button  && cf_app->mouse_prev.right_button;
	case CF_MOUSE_BUTTON_MIDDLE: return !cf_app->mouse.middle_button && cf_app->mouse_prev.middle_button;
	}
	return 0;
}

int cf_mouse_wheel_motion()
{
	return cf_app->mouse.wheel_motion;
}

bool cf_mouse_is_down_double_click(cf_mouse_button_t button)
{
	return cf_mouse_is_down(button) && cf_app->mouse.click_type == CF_MOUSE_CLICK_DOUBLE;
}

bool cf_mouse_double_click_was_pressed(cf_mouse_button_t button)
{
	return cf_mouse_was_pressed(button) && cf_app->mouse.click_type == CF_MOUSE_CLICK_DOUBLE;
}

void cf_clear_all_mouse_state()
{
	CUTE_MEMSET(&cf_app->mouse, 0, sizeof(cf_app->mouse));
	CUTE_MEMSET(&cf_app->mouse_prev, 0, sizeof(cf_app->mouse_prev));
}

void cf_input_text_add_utf8(const char* text)
{
	while (*text) {
		int cp;
		text = cf_decode8(text, &cp);
		cf_app->input_text.add(cp);
	}
}

int cf_input_text_pop_utf32()
{
	return cf_app->input_text.pop();
}

bool cf_input_text_has_data()
{
	return cf_app->input_text.count() > 0 ? true : false;
}

void cf_input_text_clear()
{
	cf_app->input_text.clear();
}
void cf_input_enable_ime()
{
	SDL_StartTextInput();
}

void cf_input_disable_ime()
{
	SDL_StopTextInput();
}

bool cf_input_is_ime_enabled()
{
	return SDL_IsTextInputActive();
}

bool cf_input_has_ime_keyboard_support()
{
	return SDL_HasScreenKeyboardSupport();
}

bool cf_input_is_ime_keyboard_shown()
{
	return SDL_IsScreenKeyboardShown(cf_app->window);
}

void cf_input_set_ime_rect(int x, int y, int w, int h)
{
	SDL_Rect r = { x, y, w, h };
	SDL_SetTextInputRect(&r);
}

bool cf_input_get_ime_composition(cf_ime_composition_t* composition)
{
	composition->composition = cf_app->ime_composition.data();
	composition->cursor = cf_app->ime_composition_cursor;
	composition->selection_len = cf_app->ime_composition_selection_len;
	return cf_app->ime_composition.count() ? true : false;
}

static void cf_s_touch_remove(uint64_t id)
{
	for (int i = 0; i < cf_app->touches.size(); ++i) {
		if (cf_app->touches[i].id == id) {
			cf_app->touches.unordered_remove(i);
			break;
		}
	}
}

int cf_touch_get_all(cf_touch_t** touches)
{
	if (touches) {
		*touches = cf_app->touches.data();
	}
	return cf_app->touches.count();
}

bool cf_touch_get(uint64_t id, cf_touch_t* touch)
{
	for (int i = 0; i < cf_app->touches.size(); ++i) {
		if (cf_app->touches[i].id == id) {
			*touch = cf_app->touches[i];
			return true;
		}
	}
	return false;
}

static cf_joypad_t* cf_s_joy(SDL_JoystickID id)
{
	for (cf_list_node_t* n = cf_list_begin(&cf_app->joypads); n != cf_list_end(&cf_app->joypads); n = n->next) {
		cf_joypad_t* joypad = CUTE_LIST_HOST(cf_joypad_t, node, n);
		if (joypad->id == id) return joypad;
	}
	return NULL;
}

void cf_pump_input_msgs()
{
	// Clear any necessary single-frame state and copy to `prev` states.
	cf_app->mouse.xrel = 0;
	cf_app->mouse.yrel = 0;
	CUTE_MEMCPY(cf_app->keys_prev, cf_app->keys, sizeof(cf_app->keys));
	CUTE_MEMCPY(&cf_app->mouse_prev, &cf_app->mouse, sizeof(cf_app->mouse));
	CUTE_MEMCPY(&cf_app->window_state_prev, &cf_app->window_state, sizeof(cf_app->window_state));
	for (cf_list_node_t* n = cf_list_begin(&cf_app->joypads); n != cf_list_end(&cf_app->joypads); n = n->next) {
		cf_joypad_t* joypad = CUTE_LIST_HOST(cf_joypad_t, node, n);
		CUTE_MEMCPY(joypad->buttons_prev, joypad->buttons, sizeof(joypad->buttons));
	}
	cf_app->mouse.wheel_motion = 0;
	cf_app->window_state.moved = false;
	cf_app->window_state.restored = false;
	cf_app->window_state.resized = false;

	// Update key durations to simulate "press and hold" style for `key_was_pressed`.
	for (int i = 0; i < 512; ++i)
	{
		if (cf_key_is_down((cf_key_button_t)i)) {
			if (cf_app->keys_duration[i] < 0) {
				cf_app->keys_duration[i] = 0;
			} else {
				cf_app->keys_duration[i] += cf_app->dt;
			}
		} else {
			cf_app->keys_duration[i] = -1.0f;
		}
	}

	// Handle SDL messages.
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (cf_app->using_imgui) {
			ImGui_ImplSDL2_ProcessEvent(&event);
		}

		switch (event.type)
		{
		case SDL_QUIT:
			cf_app->running = false;
			break;

		case SDL_WINDOWEVENT:
			switch (event.window.event)
			{
			case SDL_WINDOWEVENT_RESIZED:
				cf_app->window_state.resized = true;
				cf_app->w = event.window.data1;
				cf_app->h = event.window.data2;
				break;

			case SDL_WINDOWEVENT_MOVED:
				cf_app->window_state.moved = true;
				cf_app->x = event.window.data1;
				cf_app->y = event.window.data2;
				break;

			case SDL_WINDOWEVENT_MINIMIZED:
				cf_app->window_state.minimized = true;
				break;

			case SDL_WINDOWEVENT_MAXIMIZED:
				cf_app->window_state.maximized = true;
				break;

			case SDL_WINDOWEVENT_RESTORED:
				cf_app->window_state.restored = true;
				break;

			case SDL_WINDOWEVENT_ENTER:
				cf_app->window_state.mouse_inside_window = true;
				break;

			case SDL_WINDOWEVENT_LEAVE:
				cf_app->window_state.mouse_inside_window = false;
				break;

			case SDL_WINDOWEVENT_FOCUS_GAINED:
				cf_app->window_state.has_keyboard_focus = true;
				break;

			case SDL_WINDOWEVENT_FOCUS_LOST:
				cf_app->window_state.has_keyboard_focus = false;
				break;
			}
			break;

		case SDL_KEYDOWN:
		{
			if (event.key.repeat) continue;
			int key = SDL_GetKeyFromScancode(event.key.keysym.scancode);
			key = cf_s_map_SDL_keys(key);
			CUTE_ASSERT(key >= 0 && key < 512);
			cf_app->keys[key] = 1;
			cf_app->keys[CF_KEY_ANY] = 1;
		}	break;

		case SDL_KEYUP:
		{
			if (event.key.repeat) continue;
			int key = SDL_GetKeyFromScancode(event.key.keysym.scancode);
			key = cf_s_map_SDL_keys(key);
			CUTE_ASSERT(key >= 0 && key < 512);
			cf_app->keys[key] = 0;
		}	break;

		case SDL_TEXTINPUT:
		{
			cf_input_text_add_utf8(event.text.text);
			cf_app->ime_composition.clear();
			cf_app->ime_composition_cursor = 0;
			cf_app->ime_composition_selection_len = 0;
		}	break;

		case SDL_TEXTEDITING:
		{
			const char* text = event.edit.text;
			while (*text) cf_app->ime_composition.add(*text++);
			cf_app->ime_composition_cursor = event.edit.start;
			cf_app->ime_composition_selection_len = event.edit.length;
		}	break;

		case SDL_MOUSEMOTION:
			cf_app->mouse.x = event.motion.x;
			cf_app->mouse.y = event.motion.y;
			cf_app->mouse.xrel = event.motion.xrel;
			cf_app->mouse.yrel = -event.motion.yrel;
			break;

		case SDL_MOUSEBUTTONDOWN:
			switch (event.button.button)
			{
			case SDL_BUTTON_LEFT: cf_app->mouse.left_button = 1; break;
			case SDL_BUTTON_RIGHT: cf_app->mouse.right_button = 1; break;
			case SDL_BUTTON_MIDDLE: cf_app->mouse.middle_button = 1; break;
			}
			cf_app->mouse.x = event.button.x;
			cf_app->mouse.y = event.button.y;
			if (event.button.clicks == 1) {
				cf_app->mouse.click_type = CF_MOUSE_CLICK_SINGLE;
			} else if (event.button.clicks == 2) {
				cf_app->mouse.click_type = CF_MOUSE_CLICK_DOUBLE;
			}
			break;

		case SDL_MOUSEBUTTONUP:
			switch (event.button.button)
			{
			case SDL_BUTTON_LEFT: cf_app->mouse.left_button = 0; break;
			case SDL_BUTTON_RIGHT: cf_app->mouse.right_button = 0; break;
			case SDL_BUTTON_MIDDLE: cf_app->mouse.middle_button = 0; break;
			}
			cf_app->mouse.x = event.button.x;
			cf_app->mouse.y = event.button.y;
			if (event.button.clicks == 1) {
				cf_app->mouse.click_type = CF_MOUSE_CLICK_SINGLE;
			} else if (event.button.clicks == 2) {
				cf_app->mouse.click_type = CF_MOUSE_CLICK_DOUBLE;
			}
			break;

		case SDL_MOUSEWHEEL:
			cf_app->mouse.wheel_motion = event.wheel.y;
			break;

		case SDL_CONTROLLERBUTTONUP:
		{
			SDL_JoystickID id = event.cbutton.which;
			cf_joypad_t* joypad = cf_s_joy(id);
			if (joypad) {
				int button = (int)event.cbutton.button;
				CUTE_ASSERT(button >= 0 && button < CF_JOYPAD_BUTTON_COUNT);
				joypad->buttons[button] = 0;
			}
		}	break;

		case SDL_CONTROLLERBUTTONDOWN:
		{
			SDL_JoystickID id = event.cbutton.which;
			cf_joypad_t* joypad = cf_s_joy(id);
			if (joypad) {
				int button = (int)event.cbutton.button;
				CUTE_ASSERT(button >= 0 && button < CF_JOYPAD_BUTTON_COUNT);
				joypad->buttons[button] = 1;
			}
		}	break;

		case SDL_CONTROLLERAXISMOTION:
		{
			SDL_JoystickID id = event.caxis.which;
			cf_joypad_t* joypad = cf_s_joy(id);
			if (joypad) {
				int axis = (int)event.caxis.axis;
				int value = (int)event.caxis.value;
				CUTE_ASSERT(axis >= 0 && axis < CF_JOYPAD_AXIS_COUNT);
				joypad->axes[axis] = value;
			}
		}	break;

		case SDL_FINGERDOWN:
		{
			uint64_t id = (uint64_t)event.tfinger.fingerId;
			cf_s_touch_remove(id);
			cf_touch_t& touch = cf_app->touches.add();
			touch.id = id;
			touch.pressure = event.tfinger.pressure;
			touch.x = event.tfinger.x * cf_app->w; // NOTE: Probably wrong for high-DPI.
			touch.y = event.tfinger.y * cf_app->h; // NOTE: Probably wrong for high-DPI.
		}	break;

		case SDL_FINGERMOTION:
		{
			uint64_t id = (uint64_t)event.tfinger.fingerId;
			cf_touch_t touch;
			if (cf_touch_get(id, &touch)) {
				touch.pressure = event.tfinger.pressure;
				touch.x = event.tfinger.x * cf_app->w; // NOTE: Probably wrong for high-DPI.
				touch.y = event.tfinger.y * cf_app->h; // NOTE: Probably wrong for high-DPI.
			} else {
				cf_touch_t& touch = cf_app->touches.add();
				touch.id = id;
				touch.pressure = event.tfinger.pressure;
				touch.x = event.tfinger.x * cf_app->w; // NOTE: Probably wrong for high-DPI.
				touch.y = event.tfinger.y * cf_app->h; // NOTE: Probably wrong for high-DPI.
			}
		}	break;

		case SDL_FINGERUP:
		{
			uint64_t id = (uint64_t)event.tfinger.fingerId;
			cf_s_touch_remove(id);
		}	break;
		}
	}

	// Keep track of key mod states (alt/shift etc).
	if (cf_key_is_down(CF_KEY_NUMLOCKCLEAR)) cf_app->key_mod |= CUTE_KEY_MOD_NUMLOCK;
	else cf_app->key_mod &= ~CUTE_KEY_MOD_NUMLOCK;
	if (cf_key_is_down(CF_KEY_CAPSLOCK)) cf_app->key_mod |= CUTE_KEY_MOD_CAPSLOCK;
	else cf_app->key_mod &= ~CUTE_KEY_MOD_CAPSLOCK;
	if (cf_key_is_down(CF_KEY_LGUI)) cf_app->key_mod |= CUTE_KEY_MOD_LGUI;
	else cf_app->key_mod &= ~CUTE_KEY_MOD_LGUI;
	if (cf_key_is_down(CF_KEY_RGUI)) cf_app->key_mod |= CUTE_KEY_MOD_RGUI;
	else cf_app->key_mod &= ~CUTE_KEY_MOD_RGUI;
	if (cf_key_is_down(CF_KEY_LCTRL)) cf_app->key_mod |= CUTE_KEY_MOD_LCTRL;
	else cf_app->key_mod &= ~CUTE_KEY_MOD_LCTRL;
	if (cf_key_is_down(CF_KEY_RCTRL)) cf_app->key_mod |= CUTE_KEY_MOD_RCTRL;
	else cf_app->key_mod &= ~CUTE_KEY_MOD_RCTRL;
	if (cf_key_is_down(CF_KEY_LSHIFT)) cf_app->key_mod |= CUTE_KEY_MOD_LSHIFT;
	else cf_app->key_mod &= ~CUTE_KEY_MOD_LSHIFT;
	if (cf_key_is_down(CF_KEY_RSHIFT)) cf_app->key_mod |= CUTE_KEY_MOD_RSHIFT;
	else cf_app->key_mod &= ~CUTE_KEY_MOD_RSHIFT;
	if (cf_key_is_down(CF_KEY_RALT)) cf_app->key_mod |= CUTE_KEY_MOD_RALT;
	else cf_app->key_mod &= ~CUTE_KEY_MOD_RALT;
	if (cf_key_is_down(CF_KEY_RALT)) cf_app->key_mod |= CUTE_KEY_MOD_RALT;
	else cf_app->key_mod &= ~CUTE_KEY_MOD_RALT;
}

namespace cute
{
	cf_array<cf_touch_t> CUTE_CALL touch_get_all() { return cf_app->touches; }
}
