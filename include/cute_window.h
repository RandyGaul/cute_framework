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

#ifndef CUTE_APP_WINDOW_H
#define CUTE_APP_WINDOW_H

#include <cute_defines.h>

namespace cute
{

CUTE_API void CUTE_CALL window_size(app_t* app, int* w, int* h);
CUTE_API void CUTE_CALL window_position(app_t* app, int* x, int* y);

CUTE_API bool CUTE_CALL window_was_size_changed(app_t* app);
CUTE_API bool CUTE_CALL window_was_moved(app_t* app);

CUTE_API bool CUTE_CALL window_keyboard_lost_focus(app_t* app);
CUTE_API bool CUTE_CALL window_keyboard_gained_focus(app_t* app);
CUTE_API bool CUTE_CALL window_keyboard_has_focus(app_t* app);

CUTE_API bool CUTE_CALL window_was_minimized(app_t* app);
CUTE_API bool CUTE_CALL window_was_maximized(app_t* app);
CUTE_API bool CUTE_CALL window_is_minimized(app_t* app);
CUTE_API bool CUTE_CALL window_is_maximized(app_t* app);
CUTE_API bool CUTE_CALL window_was_restored(app_t* app);

CUTE_API bool CUTE_CALL window_mouse_entered(app_t* app);
CUTE_API bool CUTE_CALL window_mouse_exited(app_t* app);
CUTE_API bool CUTE_CALL window_mouse_inside(app_t* app);

enum window_message_box_type_t
{
	WINDOW_MESSAGE_BOX_TYPE_ERROR,
	WINDOW_MESSAGE_BOX_TYPE_WARNING,
	WINDOW_MESSAGE_BOX_TYPE_INFORMATION,
};

CUTE_API void CUTE_CALL window_message_box(app_t* app, window_message_box_type_t type, const char* title, const char* text);

}

#endif // CUTE_APP_WINDOW_H
