/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
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
