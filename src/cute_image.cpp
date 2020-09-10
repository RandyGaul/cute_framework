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

#include <cute/cute_png.h>

#include <cute_image.h>
#include <cute_file_system.h>
#include <cute_c_runtime.h>
#include <cute_alloc.h>

// TODO -- Hookup allocator context to cute_png.h

namespace cute
{

CUTE_STATIC_ASSERT(sizeof(pixel_t) == sizeof(cp_pixel_t), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(image_t) == sizeof(cp_image_t), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(image_indexed_t) == sizeof(cp_indexed_image_t), "Must be equal.");

error_t image_load_png(const char* path, image_t* img, void* user_allocator_context)
{
	void* data;
	size_t sz;
	error_t err = file_system_read_entire_file_to_memory(path, &data, &sz, user_allocator_context);
	if (err.is_error()) return err;
	return image_load_png_mem(data, (int)sz, img, user_allocator_context);
}

// TODO - Use `user_allocator_context`.
error_t image_load_png_mem(const void* data, int size, image_t* img, void* user_allocator_context)
{
	cp_image_t cp_img = cp_load_png_mem(data, size);
	if (!cp_img.pix) return error_failure(cp_error_reason);
	img->w = cp_img.w;
	img->h = cp_img.h;
	img->pix = (pixel_t*)cp_img.pix;
	return error_success();
}

error_t image_load_png_wh(const void* data, int size, int* w, int* h)
{
	int ret = cp_load_png_wh(data, size, w, h);
	if (ret) return error_failure(cp_error_reason);
	else return error_success();
}

void image_free(image_t* img)
{
	CUTE_FREE(img->pix, NULL);
}

error_t image_load_png_indexed(const char* path, image_indexed_t* img, void* user_allocator_context)
{
	void* data;
	size_t sz;
	error_t err = file_system_read_entire_file_to_memory(path, &data, &sz, user_allocator_context);
	if (err.is_error()) return err;
	return image_load_png_mem_indexed(data, (int)sz, img);
}

error_t image_load_png_mem_indexed(const void* data, int size, image_indexed_t* img)
{
	cp_indexed_image_t cp_img = cp_load_indexed_png_mem(data, size);
	if (!cp_img.pix) return error_failure(cp_error_reason);
	img->w = cp_img.w;
	img->h = cp_img.h;
	img->pix = cp_img.pix;
	img->palette_len = cp_img.palette_len;
	CUTE_MEMCPY(img->palette, cp_img.palette, sizeof(img->palette[0]) * 256);
	return error_success();
}

void image_free(image_indexed_t* img)
{
	CUTE_FREE(img->pix, NULL);
}

image_t image_depallete(image_indexed_t* indexed_img)
{
	cp_image_t cp_img = cp_depallete_indexed_image((cp_indexed_image_t*)indexed_img);
	image_t img;
	img.w = cp_img.w;
	img.h = cp_img.h;
	img.pix = (pixel_t*)cp_img.pix;
	return img;
}

void image_premultiply(image_t* img)
{
	cp_premultiply((cp_image_t*)img);
}

void image_flip_horizontal(image_t* img)
{
	cp_flip_image_horizontal((cp_image_t*)img);
}

}
