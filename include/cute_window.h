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

#include "cute_defines.h"

namespace cute
{

CUTE_API void CUTE_CALL window_size(int* w, int* h);
CUTE_API void CUTE_CALL window_position(int* x, int* y);

CUTE_API bool CUTE_CALL window_was_size_changed();
CUTE_API bool CUTE_CALL window_was_moved();

CUTE_API bool CUTE_CALL window_keyboard_lost_focus();
CUTE_API bool CUTE_CALL window_keyboard_gained_focus();
CUTE_API bool CUTE_CALL window_keyboard_has_focus();

CUTE_API bool CUTE_CALL window_was_minimized();
CUTE_API bool CUTE_CALL window_was_maximized();
CUTE_API bool CUTE_CALL window_is_minimized();
CUTE_API bool CUTE_CALL window_is_maximized();
CUTE_API bool CUTE_CALL window_was_restored();

CUTE_API bool CUTE_CALL window_mouse_entered();
CUTE_API bool CUTE_CALL window_mouse_exited();
CUTE_API bool CUTE_CALL window_mouse_inside();

enum window_message_box_type_t
{
	WINDOW_MESSAGE_BOX_TYPE_ERROR,
	WINDOW_MESSAGE_BOX_TYPE_WARNING,
	WINDOW_MESSAGE_BOX_TYPE_INFORMATION,
};

CUTE_API void CUTE_CALL window_message_box(window_message_box_type_t type, const char* title, const char* text);

}

#endif // CUTE_APP_WINDOW_H
