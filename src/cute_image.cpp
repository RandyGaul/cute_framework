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

#include <cute/cute_png.h>

#include <cute_image.h>
#include <cute_file_system.h>
#include <cute_c_runtime.h>
#include <cute_alloc.h>

// TODO -- Hookup allocator context to cute_png.h

CF_STATIC_ASSERT(sizeof(CF_Pixel) == sizeof(cp_pixel_t), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Image) == sizeof(cp_image_t), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_ImageIndexed) == sizeof(cp_indexed_image_t), "Must be equal.");

CF_Result cf_image_load_png(const char* path, CF_Image* img)
{
	size_t sz;
	void* data = cf_fs_read_entire_file_to_memory(path, &sz);
	if (!data) return cf_result_error("Unable to open png file.");
	CF_Result err = cf_image_load_png_mem(data, (int)sz, img);
	CF_FREE(data);
	return err;
}

CF_Result cf_image_load_png_mem(const void* data, int size, CF_Image* img)
{
	cp_image_t cp_img = cp_load_png_mem(data, size);
	if (!cp_img.pix) return cf_result_error(cp_error_reason);
	img->w = cp_img.w;
	img->h = cp_img.h;
	img->pix = (CF_Pixel*)cp_img.pix;
	return cf_result_success();
}

CF_Result cf_image_load_png_wh(const void* data, int size, int* w, int* h)
{
	cp_load_png_wh(data, size, w, h);
	return cf_result_success();
}

void cf_image_free(CF_Image* img)
{
	CF_FREE(img->pix);
}

CF_Result cf_image_load_png_indexed(const char* path, CF_ImageIndexed* img)
{
	size_t sz;
	void* data = cf_fs_read_entire_file_to_memory(path, &sz);
	if (!data) return cf_result_error("Unable to open png file.");
	return cf_image_load_png_mem_indexed(data, (int)sz, img);
}

CF_Result cf_image_load_png_mem_indexed(const void* data, int size, CF_ImageIndexed* img)
{
	cp_indexed_image_t cp_img = cp_load_indexed_png_mem(data, size);
	if (!cp_img.pix) return cf_result_error(cp_error_reason);
	img->w = cp_img.w;
	img->h = cp_img.h;
	img->pix = cp_img.pix;
	img->palette_len = cp_img.palette_len;
	CF_MEMCPY(img->palette, cp_img.palette, sizeof(img->palette[0]) * 256);
	return cf_result_success();
}

void cf_image_free_indexed(CF_ImageIndexed* img)
{
	CF_FREE(img->pix);
}

CF_Image cf_image_depallete(CF_ImageIndexed* indexed_img)
{
	cp_image_t cp_img = cp_depallete_indexed_image((cp_indexed_image_t*)indexed_img);
	CF_Image img;
	img.w = cp_img.w;
	img.h = cp_img.h;
	img.pix = (CF_Pixel*)cp_img.pix;
	return img;
}

void cf_image_premultiply(CF_Image* img)
{
	cp_premultiply((cp_image_t*)img);
}

void cf_image_flip_horizontal(CF_Image* img)
{
	cp_flip_image_horizontal((cp_image_t*)img);
}

