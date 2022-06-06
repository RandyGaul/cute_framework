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

#ifndef CUTE_IMAGE_H
#define CUTE_IMAGE_H

#include "cute_defines.h"
#include "cute_error.h"
#include "cute_gfx.h"

struct cf_image_t
{
	int w;
	int h;
	cf_pixel_t* pix;
};

struct cf_image_indexed_t
{
	int w;
	int h;
	uint8_t* pix;
	uint8_t palette_len;
	cf_pixel_t palette[256];
};

// -------------------------------------------------------------------------------------------------
// PNG loading.

CUTE_API cf_error_t CUTE_CALL cf_image_load_png(const char* virtual_path, cf_image_t* img, void* user_allocator_context = NULL);
CUTE_API cf_error_t CUTE_CALL cf_image_load_png_mem(const void* data, int size, cf_image_t* img, void* user_allocator_context = NULL);
CUTE_API cf_error_t CUTE_CALL cf_image_load_png_wh(const void* data, int size, int* w, int* h);
CUTE_API void CUTE_CALL cf_cf_image_free(cf_image_t* img);

CUTE_API cf_error_t CUTE_CALL cf_image_load_png_indexed(const char* virtual_path, cf_image_indexed_t* img, void* user_allocator_context = NULL);
CUTE_API cf_error_t CUTE_CALL cf_image_load_png_mem_indexed(const void* data, int size, cf_image_indexed_t* img);
CUTE_API void CUTE_CALL cf_cf_image_free(cf_image_indexed_t* img);

// -------------------------------------------------------------------------------------------------
// Image operations.

CUTE_API cf_image_t CUTE_CALL cf_image_depallete(cf_image_indexed_t* img);
CUTE_API void CUTE_CALL cf_image_premultiply(cf_image_t* img);
CUTE_API void CUTE_CALL cf_image_flip_horizontal(cf_image_t* img);

#ifdef CUTE_CPP

namespace cute
{

}

#endif // CUTE_CPP

#endif // CUTE_IMAGE_H
