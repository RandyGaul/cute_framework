/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_DX11_H
#define CF_DX11_H

#include "cute_defines.h"
#include "sokol/sokol_gfx.h"

void cf_dx11_init(void* hwnd, int w, int h, int sample_count);
sg_context_desc cf_dx11_get_context();
void cf_dx11_present(bool vsync);
void cf_dx11_shutdown();

#endif // CF_DX11_H
