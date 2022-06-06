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

struct cf_matrix_t;

namespace cute
{

struct cf_font_t;

CUTE_API cf_font_t* CUTE_CALL cf_font_load_bmfont(const char* font_path, const char* font_image_path);
CUTE_API void CUTE_CALL cf_font_free(cf_font_t* font);

CUTE_API const cf_font_t* CUTE_CALL cf_font_get_default();
CUTE_API void CUTE_CALL cf_font_push_verts(const cf_font_t* font, const char* text, float x, float y, float wrap_w, const cf_aabb_t* clip_box = NULL);
CUTE_API void CUTE_CALL cf_font_draw(const cf_font_t* font, cf_matrix_t mvp, cf_color_t color = cf_color_black());

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

struct cf_font_vert_buffer_t;

CUTE_API cf_font_vert_buffer_t* CUTE_CALL cf_font_vert_buffer_make(const cf_font_t* font);
CUTE_API cf_error_t CUTE_CALL cf_font_push_verts(cf_font_vert_buffer_t* verts, const cf_font_t* font, const char* text, float x, float y, float wrap_w, const cf_aabb_t* clip_box = NULL);
CUTE_API void CUTE_CALL cf_font_draw(cf_font_vert_buffer_t* verts, cf_color_t color = cf_color_black());

}

#endif // CUTE_FRAMEWORK_FONT_H
