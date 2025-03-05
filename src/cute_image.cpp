/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute/cute_png.h>

#include <cute_image.h>
#include <cute_file_system.h>
#include <cute_c_runtime.h>

#include <internal/cute_alloc_internal.h>

CF_STATIC_ASSERT(sizeof(CF_Pixel) == sizeof(cp_pixel_t), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Image) == sizeof(cp_image_t), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_ImageIndexed) == sizeof(cp_indexed_image_t), "Must be equal.");

CF_Result cf_image_load_png(const char* path, CF_Image* img)
{
	size_t sz;
	void* data = cf_fs_read_entire_file_to_memory(path, &sz);
	if (!data) {
		char* details = NULL;
		sfmt(details, "Unable to open png file: %s", path);
		return cf_result_error(details);
	}
	CF_Result err = cf_image_load_png_from_memory(data, (int)sz, img);
	CF_FREE(data);
	return err;
}

CF_Result cf_image_load_png_from_memory(const void* data, int size, CF_Image* img)
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
	if (!data) {
		char* details = NULL;
		sfmt(details, "Unable to open png file: %s", path);
		return cf_result_error(details);
	}
	return cf_image_load_png_from_memory_indexed(data, (int)sz, img);
}

CF_Result cf_image_load_png_from_memory_indexed(const void* data, int size, CF_ImageIndexed* img)
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

void cf_debug_dump_greyscale_pixels(const char* path, uint8_t* pixels, int w, int h)
{
	cp_image_t img;
	img.w = w;
	img.h = h;
	img.pix = (cp_pixel_t*)CF_ALLOC(sizeof(cp_pixel_t) * w * h);
	for (int i = 0; i < w * h; ++i) {
		cp_pixel_t pix;
		pix.r = pix.g = pix.b = pixels[i];
		pix.a = 255;
		img.pix[i] = pix;
	}
	cp_save_png(path, &img);
	CF_FREE(img.pix);
}

void cf_debug_dump_pixels(const char* path, CF_Pixel* pixels, int w, int h)
{
	cp_image_t img;
	img.w = w;
	img.h = h;
	img.pix = (cp_pixel_t*)CF_ALLOC(sizeof(cp_pixel_t) * w * h);
	for (int i = 0; i < w * h; ++i) {
		cp_pixel_t pix;
		pix.r = pixels[i].colors.r;
		pix.g = pixels[i].colors.g;
		pix.b = pixels[i].colors.b;
		pix.a = pixels[i].colors.a;
		img.pix[i] = pix;
	}
	cp_save_png(path, &img);
	CF_FREE(img.pix);
}
