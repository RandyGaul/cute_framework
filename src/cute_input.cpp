/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_input.h>
#include <cute_c_runtime.h>
#include <cute_math.h>
#include <cute_time.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_input_internal.h>

#include <SDL3/SDL.h>

#include <imgui/backends/imgui_impl_sdl3.h>

using namespace Cute;

static int s_map_SDL_keys(int key)
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
		case SDLK_MEDIA_NEXT_TRACK: return CF_KEY_AUDIONEXT;
		case SDLK_MEDIA_PREVIOUS_TRACK: return CF_KEY_AUDIOPREV;
		case SDLK_MEDIA_STOP: return CF_KEY_AUDIOSTOP;
		case SDLK_MEDIA_PLAY: return CF_KEY_AUDIOPLAY;
		case SDLK_MEDIA_SELECT: return CF_KEY_MEDIASELECT;
		case SDLK_AC_SEARCH: return CF_KEY_AC_SEARCH;
		case SDLK_AC_HOME: return CF_KEY_AC_HOME;
		case SDLK_AC_BACK: return CF_KEY_AC_BACK;
		case SDLK_AC_FORWARD: return CF_KEY_AC_FORWARD;
		case SDLK_AC_STOP: return CF_KEY_AC_STOP;
		case SDLK_AC_REFRESH: return CF_KEY_AC_REFRESH;
		case SDLK_AC_BOOKMARKS: return CF_KEY_AC_BOOKMARKS;
		case SDLK_MEDIA_EJECT: return CF_KEY_EJECT;
		case SDLK_SLEEP: return CF_KEY_SLEEP;
	}
	return 0;
}

bool cf_key_down(CF_KeyButton key)
{
	CF_ASSERT(key >= 0 && key < 512);
	return app->keys[key];
}

bool cf_key_up(CF_KeyButton key)
{
	CF_ASSERT(key >= 0 && key < 512);
	return !app->keys[key];
}

bool cf_key_just_pressed(CF_KeyButton key)
{
	CF_ASSERT(key >= 0 && key < 512);

	return app->keys[key] & !app->keys_prev[key];
}

bool cf_key_just_released(CF_KeyButton key)
{
	CF_ASSERT(key >= 0 && key < 512);
	return !app->keys[key] && app->keys_prev[key];
}

bool cf_key_repeating(CF_KeyButton key)
{
	CF_ASSERT(key >= 0 && key < 512);

	double repeat_delay = 0.5;
	double repeat_rate = 0.035;

	if (app->keys[key]) {
		double t = app->keys_timestamp[key] + repeat_delay;
		if (CF_SECONDS > t) {
			return cf_on_interval((float)repeat_rate, (float)t);
		}
	}

	return false;
}

bool cf_key_ctrl()
{
	return app->keys[CF_KEY_LCTRL] | app->keys[CF_KEY_RCTRL];
}

bool cf_key_shift()
{
	return app->keys[CF_KEY_LSHIFT] | app->keys[CF_KEY_RSHIFT];
}

bool cf_key_alt()
{
	return app->keys[CF_KEY_LALT] | app->keys[CF_KEY_RALT];
}

bool cf_key_gui()
{
	return app->keys[CF_KEY_LGUI] | app->keys[CF_KEY_RGUI];
}

void cf_clear_key_states()
{
	CF_MEMSET(app->keys, 0, sizeof(app->keys));
	CF_MEMSET(app->keys_prev, 0, sizeof(app->keys_prev));
}

void cf_register_key_callback(void (*key_callback)(CF_KeyButton key, bool true_down_false_up))
{
	app->key_callback = key_callback;
}

float cf_mouse_x()
{
	return app->mouse.x;
}

float cf_mouse_y()
{
	return app->mouse.y;
}

bool cf_mouse_down(CF_MouseButton button)
{
	switch (button)
	{
	case CF_MOUSE_BUTTON_LEFT:   return app->mouse.left_button;
	case CF_MOUSE_BUTTON_RIGHT:  return app->mouse.right_button;
	case CF_MOUSE_BUTTON_MIDDLE: return app->mouse.middle_button;
	}
	return 0;
}

bool cf_mouse_just_pressed(CF_MouseButton button)
{
	switch (button)
	{
	case CF_MOUSE_BUTTON_LEFT:   return app->mouse.left_button   && !app->mouse_prev.left_button;
	case CF_MOUSE_BUTTON_RIGHT:  return app->mouse.right_button  && !app->mouse_prev.right_button;
	case CF_MOUSE_BUTTON_MIDDLE: return app->mouse.middle_button && !app->mouse_prev.middle_button;
	}
	return 0;
}

bool cf_mouse_just_released(CF_MouseButton button)
{
	switch (button)
	{
	case CF_MOUSE_BUTTON_LEFT:   return !app->mouse.left_button   && app->mouse_prev.left_button;
	case CF_MOUSE_BUTTON_RIGHT:  return !app->mouse.right_button  && app->mouse_prev.right_button;
	case CF_MOUSE_BUTTON_MIDDLE: return !app->mouse.middle_button && app->mouse_prev.middle_button;
	}
	return 0;
}

float cf_mouse_wheel_motion()
{
	return app->mouse.wheel_motion;
}

bool cf_mouse_double_click_held(CF_MouseButton button)
{
	return cf_mouse_down(button) && app->mouse.click_type == CF_MOUSE_CLICK_DOUBLE;
}

bool cf_mouse_double_clicked(CF_MouseButton button)
{
	return cf_mouse_just_pressed(button) && app->mouse.click_type == CF_MOUSE_CLICK_DOUBLE;
}

void cf_mouse_hide(bool true_to_hide)
{
	if (true_to_hide) {
		SDL_HideCursor();
	} else {
		SDL_ShowCursor();
	}
}

bool cf_mouse_hidden()
{
	return SDL_CursorVisible();
}

void cf_mouse_lock_inside_window(bool true_to_lock)
{
	SDL_SetWindowMouseGrab(app->window, true_to_lock);
	SDL_WINDOWPOS_CENTERED_DISPLAY(3);
}

void cf_clear_all_mouse_state()
{
	CF_MEMSET(&app->mouse, 0, sizeof(app->mouse));
	CF_MEMSET(&app->mouse_prev, 0, sizeof(app->mouse_prev));
}

void cf_input_text_add_utf8(const char* text)
{
	while (*text) {
		int cp;
		text = cf_decode_UTF8(text, &cp);
		app->input_text.add((int)cp);
	}
}

int cf_input_text_pop_utf32()
{
	return app->input_text.pop();
}

bool cf_input_text_has_data()
{
	return app->input_text.count() > 0 ? true : false;
}

bool cf_input_text_get_buffer(CF_InputTextBuffer* buffer)
{
	buffer->len = app->input_text.count();
	buffer->codepoints = app->input_text.data();
	return app->input_text.count() > 0;
}

void cf_input_text_clear()
{
	app->input_text.clear();
}
void cf_input_enable_ime()
{
	SDL_StartTextInput(app->window);
}

void cf_input_disable_ime()
{
	SDL_StopTextInput(app->window);
}

bool cf_input_is_ime_enabled()
{
	return SDL_TextInputActive(app->window);
}

bool cf_input_has_ime_keyboard_support()
{
	return SDL_HasScreenKeyboardSupport();
}

bool cf_input_is_ime_keyboard_shown()
{
	return SDL_ScreenKeyboardShown(app->window);
}

void cf_input_set_ime_rect(int x, int y, int w, int h)
{
	SDL_Rect r = { x, y, w, h };
	SDL_SetTextInputArea(app->window, &r, 0);
}

bool cf_input_get_ime_composition(CF_ImeComposition* composition)
{
	composition->composition = app->ime_composition.data();
	composition->cursor = app->ime_composition_cursor;
	composition->selection_len = app->ime_composition_selection_len;
	return app->ime_composition.count() ? true : false;
}

static void s_touch_remove(uint64_t id)
{
	for (int i = 0; i < app->touches.size(); ++i) {
		if (app->touches[i].id == id) {
			app->touches.unordered_remove(i);
			break;
		}
	}
}

int cf_touch_get_all(CF_Touch** touches)
{
	if (touches) {
		*touches = app->touches.data();
	}
	return app->touches.count();
}

bool cf_touch_get(uint64_t id, CF_Touch* touch)
{
	for (int i = 0; i < app->touches.size(); ++i) {
		if (app->touches[i].id == id) {
			*touch = app->touches[i];
			return true;
		}
	}
	return false;
}

void cf_begin_frame_input()
{
	// Clear any necessary single-frame state and copy to `prev` states.
	app->mouse.xrel = 0;
	app->mouse.yrel = 0;
	CF_MEMCPY(app->keys_prev, app->keys, sizeof(app->keys));
	CF_MEMCPY(&app->mouse_prev, &app->mouse, sizeof(app->mouse));
	CF_MEMCPY(&app->window_state_prev, &app->window_state, sizeof(app->window_state));
	app->mouse.wheel_motion = 0;
	app->window_state.moved = false;
	app->window_state.restored = false;
	app->window_state.resized = false;
	cf_joypad_update();

	// Update key durations to simulate "press and hold" style for `key_repeating`.
	for (int i = 0; i < 512; ++i) {
		if (!cf_key_down((CF_KeyButton)i)) {
			app->keys_timestamp[i] = 0;
		}
	}

	// Support held timer on KEY_ANY.
	bool none_pressed = true;
	for (int i = 0; i < CF_ARRAY_SIZE(app->keys); ++i) {
		if (i != CF_KEY_ANY && app->keys[i]) {
			none_pressed = false;
			break;
		}
	}
	if (none_pressed) {
		app->keys[CF_KEY_ANY] = 0;
	}
}

void cf_pump_input_msgs()
{
	// Handle SDL messages.
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (app->using_imgui) {
			ImGui_ImplSDL3_ProcessEvent(&event);
		}

		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			app->running = false;
			break;

		case SDL_EVENT_WINDOW_RESIZED:
			app->window_state.resized = true;
			app->w = event.window.data1;
			app->h = event.window.data2;
			break;

		case SDL_EVENT_WINDOW_MOVED:
			app->window_state.moved = true;
			app->x = event.window.data1;
			app->y = event.window.data2;
			break;

		case SDL_EVENT_WINDOW_MINIMIZED:
			app->window_state.minimized = true;
			break;

		case SDL_EVENT_WINDOW_MAXIMIZED:
			app->window_state.maximized = true;
			break;

		case SDL_EVENT_WINDOW_RESTORED:
			app->window_state.restored = true;
			break;

		case SDL_EVENT_WINDOW_MOUSE_ENTER:
			app->window_state.mouse_inside_window = true;
			break;

		case SDL_EVENT_WINDOW_MOUSE_LEAVE:
			app->window_state.mouse_inside_window = false;
			break;

		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			app->window_state.has_keyboard_focus = true;
			break;

		case SDL_EVENT_WINDOW_FOCUS_LOST:
			app->window_state.has_keyboard_focus = false;
			break;

		case SDL_EVENT_KEY_DOWN:
		{
			if (event.key.repeat) continue;
			int key = SDL_GetKeyFromScancode(event.key.scancode, event.key.mod, true);
			key = s_map_SDL_keys(key);
			CF_ASSERT(key >= 0 && key < 512);
			app->keys[key] = 1;
			app->keys[CF_KEY_ANY] = 1;
			app->keys_timestamp[key] = app->keys_timestamp[CF_KEY_ANY] = CF_SECONDS;
			if (app->key_callback) app->key_callback((CF_KeyButton)key, true);
		}	break;

		case SDL_EVENT_KEY_UP:
		{
			if (event.key.repeat) continue;
			int key = SDL_GetKeyFromScancode(event.key.scancode, event.key.mod, true);
			key = s_map_SDL_keys(key);
			CF_ASSERT(key >= 0 && key < 512);
			app->keys[key] = 0;
			if (app->key_callback) app->key_callback((CF_KeyButton)key, false);
		}	break;

		case SDL_EVENT_TEXT_INPUT:
		{
			cf_input_text_add_utf8(event.text.text);
			app->ime_composition.clear();
			app->ime_composition_cursor = 0;
			app->ime_composition_selection_len = 0;
		}	break;

		case SDL_EVENT_TEXT_EDITING:
		{
			app->ime_composition.clear();
			const char* text = event.edit.text;
			while (*text) app->ime_composition.add(*text++);
			app->ime_composition.add(0);
			app->ime_composition_cursor = event.edit.start;
			app->ime_composition_selection_len = event.edit.length;
		}	break;

		case SDL_EVENT_MOUSE_MOTION:
			app->mouse.x = event.motion.x;
			app->mouse.y = event.motion.y;
			app->mouse.xrel = event.motion.xrel;
			app->mouse.yrel = -event.motion.yrel;
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			switch (event.button.button)
			{
			case SDL_BUTTON_LEFT: app->mouse.left_button = 1; break;
			case SDL_BUTTON_RIGHT: app->mouse.right_button = 1; break;
			case SDL_BUTTON_MIDDLE: app->mouse.middle_button = 1; break;
			}
			app->mouse.x = event.button.x;
			app->mouse.y = event.button.y;
			if (event.button.clicks == 1) {
				app->mouse.click_type = CF_MOUSE_CLICK_SINGLE;
			} else if (event.button.clicks == 2) {
				app->mouse.click_type = CF_MOUSE_CLICK_DOUBLE;
			}
			break;

		case SDL_EVENT_MOUSE_BUTTON_UP:
			switch (event.button.button)
			{
			case SDL_BUTTON_LEFT: app->mouse.left_button = 0; break;
			case SDL_BUTTON_RIGHT: app->mouse.right_button = 0; break;
			case SDL_BUTTON_MIDDLE: app->mouse.middle_button = 0; break;
			}
			app->mouse.x = event.button.x;
			app->mouse.y = event.button.y;
			if (event.button.clicks == 1) {
				app->mouse.click_type = CF_MOUSE_CLICK_SINGLE;
			} else if (event.button.clicks == 2) {
				app->mouse.click_type = CF_MOUSE_CLICK_DOUBLE;
			}
			break;

		case SDL_EVENT_MOUSE_WHEEL:
			app->mouse.wheel_motion = event.wheel.y;
			break;

		case SDL_EVENT_GAMEPAD_BUTTON_UP:
		{
			SDL_JoystickID id = event.gbutton.which;
			cf_joypad_on_button_up(id, (int)event.gbutton.button);
		}	break;

		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		{
			SDL_JoystickID id = event.gbutton.which;
			cf_joypad_on_button_down(id, (int)event.gbutton.button);
		}	break;

		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		{
			SDL_JoystickID id = event.gaxis.which;
			cf_joypad_on_axis_motion(id, (int)event.gaxis.axis, (int)event.gaxis.value);
		}	break;

		case SDL_EVENT_FINGER_DOWN:
		{
			uint64_t id = (uint64_t)event.tfinger.fingerID;
			s_touch_remove(id);
			CF_Touch& touch = app->touches.add();
			touch.id = id;
			touch.pressure = event.tfinger.pressure;
			touch.x = event.tfinger.x * app->w; // NOTE: Probably wrong for high-DPI.
			touch.y = event.tfinger.y * app->h; // NOTE: Probably wrong for high-DPI.
		}	break;

		case SDL_EVENT_FINGER_MOTION:
		{
			uint64_t id = (uint64_t)event.tfinger.fingerID;
			CF_Touch touch;
			if (cf_touch_get(id, &touch)) {
				touch.pressure = event.tfinger.pressure;
				touch.x = event.tfinger.x * app->w; // NOTE: Probably wrong for high-DPI.
				touch.y = event.tfinger.y * app->h; // NOTE: Probably wrong for high-DPI.
			} else {
				CF_Touch& touch = app->touches.add();
				touch.id = id;
				touch.pressure = event.tfinger.pressure;
				touch.x = event.tfinger.x * app->w; // NOTE: Probably wrong for high-DPI.
				touch.y = event.tfinger.y * app->h; // NOTE: Probably wrong for high-DPI.
			}
		}	break;

		case SDL_EVENT_FINGER_UP:
		{
			uint64_t id = (uint64_t)event.tfinger.fingerID;
			s_touch_remove(id);
		}	break;
		}
	}
}

namespace Cute
{
	Array<CF_Touch> CF_CALL touch_get_all() { return app->touches; }
}
