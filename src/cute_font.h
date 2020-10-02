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

#include <cute_defines.h>
#include <cute_math.h>
#include <cute_error.h>
#include <cute_color.h>

namespace cute
{

struct font_t;
struct app_t;
struct matrix_t;

CUTE_API font_t* CUTE_CALL font_load_bmfont(app_t* app, const char* font_path, const char* font_image_path);
CUTE_API void CUTE_CALL font_free(font_t* font);

CUTE_API const font_t* CUTE_CALL font_get_default(app_t* app);
CUTE_API void CUTE_CALL font_push_verts(app_t* app, const font_t* font, const char* text, float x, float y, float wrap_w, const aabb_t* clip_box = NULL);
CUTE_API void CUTE_CALL font_draw(app_t* app, const font_t* font, matrix_t mvp, color_t color = color_black());

CUTE_API void CUTE_CALL font_borders(app_t* app, bool use_borders);
CUTE_API void CUTE_CALL font_toggle_borders(app_t* app);
CUTE_API bool CUTE_CALL font_is_borders_on(app_t* app);
CUTE_API void CUTE_CALL font_border_color(app_t* app, color_t color);
CUTE_API void CUTE_CALL font_border_use_corners(app_t* app, bool use_corners);

CUTE_API int CUTE_CALL font_height(const font_t* font);
CUTE_API int CUTE_CALL font_line_height(const font_t* font);

CUTE_API int CUTE_CALL font_text_width(const font_t* font, const char* text);
CUTE_API int CUTE_CALL font_text_height(const font_t* font, const char* text);

// -------------------------------------------------------------------------------------------------
// These functions can be used to isolate vert rendering on different threads.

struct font_vert_buffer_t;

CUTE_API font_vert_buffer_t* CUTE_CALL font_vert_buffer_make(app_t* app, const font_t* font);
CUTE_API error_t CUTE_CALL font_push_verts(font_vert_buffer_t* verts, const font_t* font, const char* text, float x, float y, float wrap_w, const aabb_t* clip_box = NULL);
CUTE_API void CUTE_CALL font_draw(app_t* app, font_vert_buffer_t* verts, color_t color = color_black());

}

#endif // CUTE_FRAMEWORK_FONT_H
