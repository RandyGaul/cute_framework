/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_IMAGE_H
#define CF_IMAGE_H

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

CF_API CF_Result CF_CALL cf_image_load_png(const char* virtual_path, CF_Image* img);
CF_API CF_Result CF_CALL cf_image_load_png_from_memory(const void* data, int size, CF_Image* img);
CF_API CF_Result CF_CALL cf_image_load_png_wh(const void* data, int size, int* w, int* h);
CF_API void CF_CALL cf_image_free(CF_Image* img);

CF_API CF_Result CF_CALL cf_image_load_png_indexed(const char* virtual_path, CF_ImageIndexed* img);
CF_API CF_Result CF_CALL cf_image_load_png_from_memory_indexed(const void* data, int size, CF_ImageIndexed* img);
CF_API void CF_CALL cf_image_free_indexed(CF_ImageIndexed* img);

// -------------------------------------------------------------------------------------------------
// Image operations.

CF_API CF_Image CF_CALL cf_image_depallete(CF_ImageIndexed* img);
CF_API void CF_CALL cf_image_premultiply(CF_Image* img);
CF_API void CF_CALL cf_image_flip_horizontal(CF_Image* img);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Image = CF_Image;
using ImageIndexed = CF_ImageIndexed;

// -------------------------------------------------------------------------------------------------
// PNG loading.

CF_INLINE Result image_load_png(const char* virtual_path, Image* img) { return cf_image_load_png(virtual_path, img); }
CF_INLINE Result image_load_png_mem(const void* data, int size, Image* img) { return cf_image_load_png_from_memory(data, size, img); }
CF_INLINE Result image_load_png_wh(const void* data, int size, int* w, int* h) { return cf_image_load_png_wh(data, size, w, h); }
CF_INLINE void image_free(Image* img) { cf_image_free(img); }

CF_INLINE Result image_load_png_indexed(const char* virtual_path, ImageIndexed* img) { return cf_image_load_png_indexed(virtual_path, img); }
CF_INLINE Result image_load_png_mem_indexed(const void* data, int size, ImageIndexed* img) { return cf_image_load_png_from_memory_indexed(data, size, img); }
CF_INLINE void image_free(ImageIndexed* img) { cf_image_free_indexed(img); }

// -------------------------------------------------------------------------------------------------
// Image operations.

CF_INLINE Image image_depallete(ImageIndexed* img) { return cf_image_depallete(img); }
CF_INLINE void image_premultiply(Image* img) { cf_image_premultiply(img); }
CF_INLINE void image_flip_horizontal(Image* img) { cf_image_flip_horizontal(img); }

}

#endif // CF_CPP

#endif // CF_IMAGE_H
