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

#ifndef CUTE_FONT_H
#define CUTE_FONT_H

#include <cute_defines.h>
#include <cute_math.h>

namespace cute
{

struct font_t;
struct app_t;

extern CUTE_API font_t* CUTE_CALL font_load_bmfont(app_t* app, const char* font_path, const char* font_image_path);
extern CUTE_API void CUTE_CALL font_free(font_t* font);

extern CUTE_API error_t CUTE_CALL font_push_verts(const font_t* font, const char* text, float x, float y, float wrap_w, const aabb_t* clip_box = NULL);
extern CUTE_API void CUTE_CALL font_submit_draw_call(app_t* app, const font_t* font, color_t color = color_black());

// -------------------------------------------------------------------------------------------------
// These functions can be used to isolate vert rendering on different threads.

struct font_vert_buffer_t;

extern CUTE_API font_vert_buffer_t* CUTE_CALL font_vert_buffer_make(app_t* app, const font_t* font);
extern CUTE_API error_t CUTE_CALL font_push_verts(font_vert_buffer_t* verts, const font_t* font, const char* text, float x, float y, float wrap_w, const aabb_t* clip_box = NULL);
extern CUTE_API void CUTE_CALL font_submit_draw_call(app_t* app, font_vert_buffer_t* verts, color_t color = color_black())

}

#endif // CUTE_FONT_H