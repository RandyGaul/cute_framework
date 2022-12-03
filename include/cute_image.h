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
#include "cute_result.h"
#include "cute_graphics.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct CF_Image
{
	int w;
	int h;
	CF_Pixel* pix;
} CF_Image;

typedef struct CF_ImageIndexed
{
	int w;
	int h;
	uint8_t* pix;
	uint8_t palette_len;
	CF_Pixel palette[256];
} CF_ImageIndexed;

// -------------------------------------------------------------------------------------------------
// PNG loading.

CUTE_API CF_Result CUTE_CALL cf_image_load_png(const char* virtual_path, CF_Image* img);
CUTE_API CF_Result CUTE_CALL cf_image_load_png_mem(const void* data, int size, CF_Image* img);
CUTE_API CF_Result CUTE_CALL cf_image_load_png_wh(const void* data, int size, int* w, int* h);
CUTE_API void CUTE_CALL cf_image_free(CF_Image* img);

CUTE_API CF_Result CUTE_CALL cf_image_load_png_indexed(const char* virtual_path, CF_ImageIndexed* img);
CUTE_API CF_Result CUTE_CALL cf_image_load_png_mem_indexed(const void* data, int size, CF_ImageIndexed* img);
CUTE_API void CUTE_CALL cf_image_free_indexed(CF_ImageIndexed* img);

// -------------------------------------------------------------------------------------------------
// Image operations.

CUTE_API CF_Image CUTE_CALL cf_image_depallete(CF_ImageIndexed* img);
CUTE_API void CUTE_CALL cf_image_premultiply(CF_Image* img);
CUTE_API void CUTE_CALL cf_image_flip_horizontal(CF_Image* img);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using Image = CF_Image;
using ImageIndexed = CF_ImageIndexed;

// -------------------------------------------------------------------------------------------------
// PNG loading.

CUTE_INLINE Result image_load_png(const char* virtual_path, Image* img) { return cf_image_load_png(virtual_path, img); }
CUTE_INLINE Result image_load_png_mem(const void* data, int size, Image* img) { return cf_image_load_png_mem(data, size, img); }
CUTE_INLINE Result image_load_png_wh(const void* data, int size, int* w, int* h) { return cf_image_load_png_wh(data, size, w, h); }
CUTE_INLINE void image_free(Image* img) { cf_image_free(img); }

CUTE_INLINE Result image_load_png_indexed(const char* virtual_path, ImageIndexed* img) { return cf_image_load_png_indexed(virtual_path, img); }
CUTE_INLINE Result image_load_png_mem_indexed(const void* data, int size, ImageIndexed* img) { return cf_image_load_png_mem_indexed(data, size, img); }
CUTE_INLINE void image_free(ImageIndexed* img) { cf_image_free_indexed(img); }

// -------------------------------------------------------------------------------------------------
// Image operations.

CUTE_INLINE Image image_depallete(ImageIndexed* img) { return cf_image_depallete(img); }
CUTE_INLINE void image_premultiply(Image* img) { cf_image_premultiply(img); }
CUTE_INLINE void image_flip_horizontal(Image* img) { cf_image_flip_horizontal(img); }

}

#endif // CUTE_CPP

#endif // CUTE_IMAGE_H
