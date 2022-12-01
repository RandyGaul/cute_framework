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

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CUTE_API void CUTE_CALL cf_window_size(int* w, int* h);
CUTE_API void CUTE_CALL cf_window_position(int* x, int* y);

CUTE_API bool CUTE_CALL cf_window_was_size_changed();
CUTE_API bool CUTE_CALL cf_window_was_moved();

CUTE_API bool CUTE_CALL cf_window_keyboard_lost_focus();
CUTE_API bool CUTE_CALL cf_window_keyboard_gained_focus();
CUTE_API bool CUTE_CALL cf_window_keyboard_has_focus();

CUTE_API bool CUTE_CALL cf_window_was_minimized();
CUTE_API bool CUTE_CALL cf_window_was_maximized();
CUTE_API bool CUTE_CALL cf_window_is_minimized();
CUTE_API bool CUTE_CALL cf_window_is_maximized();
CUTE_API bool CUTE_CALL cf_window_was_restored();

CUTE_API bool CUTE_CALL cf_window_mouse_entered();
CUTE_API bool CUTE_CALL cf_window_mouse_exited();
CUTE_API bool CUTE_CALL cf_window_mouse_inside();

#define CF_WINDOW_MESSAGE_BOX_TYPE_DEFS \
	CF_ENUM(WINDOW_MESSAGE_BOX_TYPE_ERROR, 0) \
	CF_ENUM(WINDOW_MESSAGE_BOX_TYPE_WARNING, 1) \
	CF_ENUM(WINDOW_MESSAGE_BOX_TYPE_INFORMATION, 2) \

typedef enum CF_WindowMessageBoxType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_WINDOW_MESSAGE_BOX_TYPE_DEFS
	#undef CF_ENUM
} CF_WindowMessageBoxType;

CUTE_INLINE const char* cf_window_message_box_type_to_string(CF_WindowMessageBoxType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_WINDOW_MESSAGE_BOX_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CUTE_API void CUTE_CALL cf_window_message_box(CF_WindowMessageBoxType type, const char* title, const char* text);

#ifdef CUTE_DEBUG
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using WindowMessageBoxType = CF_WindowMessageBoxType;
#define CF_ENUM(K, V) CUTE_INLINE constexpr WindowMessageBoxType K = CF_##K;
CF_WINDOW_MESSAGE_BOX_TYPE_DEFS
#undef CF_ENUM

CUTE_INLINE const char* to_string(WindowMessageBoxType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_WINDOW_MESSAGE_BOX_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CUTE_INLINE void window_size(int* w, int* h) { return cf_window_size(w, h); }
CUTE_INLINE void window_position(int* x, int* y) { return cf_window_position(x, y); }
CUTE_INLINE bool window_was_size_changed() { return cf_window_was_size_changed(); }
CUTE_INLINE bool window_was_moved() { return cf_window_was_moved(); }
CUTE_INLINE bool window_keyboard_lost_focus() { return cf_window_keyboard_lost_focus(); }
CUTE_INLINE bool window_keyboard_gained_focus() { return cf_window_keyboard_gained_focus(); }
CUTE_INLINE bool window_keyboard_has_focus() { return cf_window_keyboard_has_focus(); }
CUTE_INLINE bool window_was_minimized() { return cf_window_was_minimized(); }
CUTE_INLINE bool window_was_maximized() { return cf_window_was_maximized(); }
CUTE_INLINE bool window_is_minimized() { return cf_window_is_minimized(); }
CUTE_INLINE bool window_is_maximized() { return cf_window_is_maximized(); }
CUTE_INLINE bool window_was_restored() { return cf_window_was_restored(); }
CUTE_INLINE bool window_mouse_entered() { return cf_window_mouse_entered(); }
CUTE_INLINE bool window_mouse_exited() { return cf_window_mouse_exited(); }
CUTE_INLINE bool window_mouse_inside() { return cf_window_mouse_inside(); }
CUTE_INLINE void window_message_box(WindowMessageBoxType type, const char* title, const char* text) { return cf_window_message_box(type, title, text); }

}

#endif // CUTE_CPP

#endif // CUTE_APP_WINDOW_H
