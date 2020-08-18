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

#ifndef CUTE_FONT_INTERNAL_H
#define CUTE_FONT_INTERNAL_H

#include <cute/cute_font.h>
#include <cute_gfx.h>
#include <cute_color.h>
#include <cute_math.h>

namespace cute
{

using font_vertex_t = cute_font_vert_t;

struct font_vs_uniforms_t
{
	matrix_t mvp;
};

struct font_fs_uniforms_t
{
	color_t u_text_color;
	color_t u_border_color;
	v2 u_texel_size;
	float use_border;
};

void font_init(app_t* app);

}

#endif // CUTE_FONT_INTERNAL_H
