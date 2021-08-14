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

namespace cute
{

void window_size(app_t* app, int* w, int* h)
{
	if (w) *w = app->w;
	if (h) *h = app->h;
}

void window_position(app_t* app, int* x, int* y)
{
	if (x) *x = app->x;
	if (y) *y = app->y;
}

bool window_was_size_changed(app_t* app)
{
	return app->window_state.resized;
}

bool window_was_moved(app_t* app)
{
	return app->window_state.moved;
}

bool window_keyboard_lost_focus(app_t* app)
{
	return !app->window_state.has_keyboard_focus && app->window_state_prev.has_keyboard_focus;
}

bool window_keyboard_gained_focus(app_t* app)
{
	return app->window_state.has_keyboard_focus && !app->window_state_prev.has_keyboard_focus;
}

bool window_keyboard_has_focus(app_t* app)
{
	return app->window_state.has_keyboard_focus;
}

bool window_was_minimized(app_t* app)
{
	return app->window_state.minimized && !app->window_state_prev.minimized;
}

bool window_was_maximized(app_t* app)
{
	return app->window_state.maximized && !app->window_state_prev.maximized;
}

bool window_is_minimized(app_t* app)
{
	return app->window_state.minimized;
}

bool window_is_maximized(app_t* app)
{
	return app->window_state.maximized;
}

bool window_was_restored(app_t* app)
{
	return app->window_state.restored && !app->window_state_prev.restored;
}

bool window_mouse_entered(app_t* app)
{
	return app->window_state.mouse_inside_window && !app->window_state_prev.mouse_inside_window;
}

bool window_mouse_exited(app_t* app)
{
	return !app->window_state.mouse_inside_window && app->window_state_prev.mouse_inside_window;
}

bool window_mouse_inside(app_t* app)
{
	return app->window_state.mouse_inside_window;
}

static int s_message_box_flags(window_message_box_type_t type)
{
	switch (type)
	{
	case WINDOW_MESSAGE_BOX_TYPE_ERROR: return SDL_MESSAGEBOX_ERROR;
	case WINDOW_MESSAGE_BOX_TYPE_WARNING: return SDL_MESSAGEBOX_WARNING;
	case WINDOW_MESSAGE_BOX_TYPE_INFORMATION: return SDL_MESSAGEBOX_INFORMATION;
	}
	return SDL_MESSAGEBOX_ERROR;
}

void window_message_box(app_t* app, window_message_box_type_t type, const char* title, const char* text)
{
	SDL_ShowSimpleMessageBox(s_message_box_flags(type), title, text, app->window);
}

}
