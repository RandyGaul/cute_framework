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

#include <cute_window.h>

#include <internal/cute_app_internal.h>
#include <SDL.h>

void cf_window_size(int* w, int* h)
{
	if (w) *w = app->w;
	if (h) *h = app->h;
}

void cf_window_position(int* x, int* y)
{
	if (x) *x = app->x;
	if (y) *y = app->y;
}

bool cf_window_was_size_changed()
{
	return app->window_state.resized;
}

bool cf_window_was_moved()
{
	return app->window_state.moved;
}

bool cf_window_keyboard_lost_focus()
{
	return !app->window_state.has_keyboard_focus && app->window_state_prev.has_keyboard_focus;
}

bool cf_window_keyboard_gained_focus()
{
	return app->window_state.has_keyboard_focus && !app->window_state_prev.has_keyboard_focus;
}

bool cf_window_keyboard_has_focus()
{
	return app->window_state.has_keyboard_focus;
}

bool cf_window_was_minimized()
{
	return app->window_state.minimized && !app->window_state_prev.minimized;
}

bool cf_window_was_maximized()
{
	return app->window_state.maximized && !app->window_state_prev.maximized;
}

bool cf_window_is_minimized()
{
	return app->window_state.minimized;
}

bool cf_window_is_maximized()
{
	return app->window_state.maximized;
}

bool cf_window_was_restored()
{
	return app->window_state.restored && !app->window_state_prev.restored;
}

bool cf_window_mouse_entered()
{
	return app->window_state.mouse_inside_window && !app->window_state_prev.mouse_inside_window;
}

bool cf_window_mouse_exited()
{
	return !app->window_state.mouse_inside_window && app->window_state_prev.mouse_inside_window;
}

bool cf_window_mouse_inside()
{
	return app->window_state.mouse_inside_window;
}

static int s_message_box_flags(CF_WindowMessageBoxType type)
{
	switch (type)
	{
	case CF_WINDOW_MESSAGE_BOX_TYPE_ERROR: return SDL_MESSAGEBOX_ERROR;
	case CF_WINDOW_MESSAGE_BOX_TYPE_WARNING: return SDL_MESSAGEBOX_WARNING;
	case CF_WINDOW_MESSAGE_BOX_TYPE_INFORMATION: return SDL_MESSAGEBOX_INFORMATION;
	}
	return SDL_MESSAGEBOX_ERROR;
}

void cf_window_message_box(CF_WindowMessageBoxType type, const char* title, const char* text)
{
	SDL_ShowSimpleMessageBox(s_message_box_flags(type), title, text, app->window);
}

