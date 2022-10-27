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

#ifndef CUTE_FRAMEWORK_FONT_H
#define CUTE_FRAMEWORK_FONT_H

#include "cute_defines.h"
#include "cute_math.h"
#include "cute_error.h"
#include "cute_color.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct cf_matrix_t cf_matrix_t;
typedef struct cf_font_t cf_font_t;

CUTE_API cf_font_t* CUTE_CALL cf_font_load_bmfont(const char* font_path, const char* font_image_path);
CUTE_API void CUTE_CALL cf_font_free(cf_font_t* font);

CUTE_API const cf_font_t* CUTE_CALL cf_font_get_default();
CUTE_API void CUTE_CALL cf_font_push_verts(const cf_font_t* font, const char* text, float x, float y, float wrap_w, const cf_aabb_t* clip_box /*= NULL*/);
CUTE_API void CUTE_CALL cf_font_draw(const cf_font_t* font, cf_matrix_t mvp, cf_color_t color /*= cf_color_black()*/);

CUTE_API void CUTE_CALL cf_font_borders(bool use_borders);
CUTE_API void CUTE_CALL cf_font_toggle_borders();
CUTE_API bool CUTE_CALL cf_font_is_borders_on();
CUTE_API void CUTE_CALL cf_font_border_color(cf_color_t color);
CUTE_API void CUTE_CALL cf_font_border_use_corners(bool use_corners);

CUTE_API int CUTE_CALL cf_font_height(const cf_font_t* font);
CUTE_API int CUTE_CALL cf_font_line_height(const cf_font_t* font);

CUTE_API int CUTE_CALL cf_font_text_width(const cf_font_t* font, const char* text);
CUTE_API int CUTE_CALL cf_font_text_height(const cf_font_t* font, const char* text);

// -------------------------------------------------------------------------------------------------
// These functions can be used to isolate vert rendering on different threads.

typedef struct cf_font_vert_buffer_t cf_font_vert_buffer_t;

CUTE_API cf_font_vert_buffer_t* CUTE_CALL cf_font_vert_buffer_make(const cf_font_t* font);
CUTE_API cf_error_t CUTE_CALL cf_font_push_verts2(cf_font_vert_buffer_t* verts, const cf_font_t* font, const char* text, float x, float y, float wrap_w, const cf_aabb_t* clip_box /*= NULL*/);
CUTE_API void CUTE_CALL cf_font_draw2(cf_font_vert_buffer_t* verts, cf_color_t color /*= cf_color_black()*/);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{
using matrix_t = cf_matrix_t;
using font_t = cf_font_t;
using font_vert_buffer_t = cf_font_vert_buffer_t;
using aabb_t = cf_aabb_t;

CUTE_INLINE font_t* font_load_bmfont(const char* font_path, const char* font_image_path) { return cf_font_load_bmfont(font_path, font_image_path); }
CUTE_INLINE void font_free(font_t* font) { cf_font_free(font); }

CUTE_INLINE const font_t* font_get_default() { return cf_font_get_default(); }
CUTE_INLINE void font_push_verts(const font_t* font, const char* text, float x, float y, float wrap_w, const aabb_t* clip_box = NULL) { cf_font_push_verts(font, text, x, y, wrap_w, clip_box); }
CUTE_API void CUTE_CALL font_draw(const cf_font_t* font, matrix_t mvp, color_t color = color_black());

CUTE_INLINE void font_borders(bool use_borders) { cf_font_borders(use_borders); }
CUTE_INLINE void font_toggle_borders() { cf_font_toggle_borders(); }
CUTE_INLINE bool font_is_borders_on() { return cf_font_is_borders_on(); }
CUTE_INLINE void font_border_color(color_t color) { return cf_font_border_color(color); }
CUTE_INLINE void font_border_use_corners(bool use_corners) { return cf_font_border_use_corners(use_corners); }

CUTE_INLINE int font_height(const font_t* font) { return cf_font_height(font); }
CUTE_INLINE int font_line_height(const font_t* font) { return cf_font_line_height(font); }

CUTE_INLINE int font_text_width(const font_t* font, const char* text) { return cf_font_text_width(font, text); }
CUTE_INLINE int font_text_height(const font_t* font, const char* text) { return cf_font_text_height(font, text); }

// -------------------------------------------------------------------------------------------------
// These functions can be used to isolate vert rendering on different threads.


CUTE_INLINE font_vert_buffer_t* font_vert_buffer_make(const font_t* font) { return cf_font_vert_buffer_make(font); }
CUTE_INLINE error_t font_push_verts(font_vert_buffer_t* verts, const font_t* font, const char* text, float x, float y, float wrap_w, const aabb_t* clip_box = NULL) { return cf_font_push_verts2(verts, font, text, x, y, wrap_w, clip_box); }
CUTE_INLINE void font_draw(font_vert_buffer_t* verts, color_t color = color_black()) { cf_font_draw2(verts, color); }
}

#endif // CUTE_CPP

#endif // CUTE_FRAMEWORK_FONT_H
