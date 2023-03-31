/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#ifndef CF_METAL_H
#define CF_METAL_H

#include "cute_defines.h"
#include "sokol/sokol_gfx.h"

void cf_metal_init(void* sdl_window, int w, int h, int sample_count);
sg_context_desc cf_metal_get_context();
const void *cf_metal_get_render_pass_descriptor();
const void* cf_metal_get_drawable();
void cf_metal_present(bool vsync);
float cf_metal_get_dpi_scale();
void cf_metal_get_drawable_size(int* w, int* h);

#endif // CF_METAL_H
