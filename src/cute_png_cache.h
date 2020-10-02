/*
	Cute Framework
	Copyright (C) 2020 Randy Gaul https://randygaul.net

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

#ifndef CUTE_PNG_CACHE_H
#define CUTE_PNG_CACHE_H

#include <cute_defines.h>
#include <cute_error.h>
#include <cute_color.h>
#include <cute_sprite.h>
#include <cute_batch.h>

#include <cute/cute_png.h>
#include <mattiasgustavsson/strpool.h>

namespace cute
{

struct png_cache_t;

CUTE_API png_cache_t* CUTE_CALL png_cache_make(void* mem_ctx = NULL);
CUTE_API void CUTE_CALL png_cache_destroy(png_cache_t* cache);

struct png_t
{
	const char* path = NULL;
	uint64_t id = ~0;
	pixel_t* pix = NULL;
	int w = 0;
	int h = 0;
};

CUTE_API error_t CUTE_CALL png_cache_load(png_cache_t* cache, const char* png_path, png_t* png = NULL);
CUTE_API error_t CUTE_CALL png_cache_load_mem(png_cache_t* cache, const char* png_path, const void* memory, size_t size, png_t* png = NULL);
CUTE_API void CUTE_CALL png_cache_unload(png_cache_t* cache, png_t* png);

CUTE_API get_pixels_fn* CUTE_CALL png_cache_get_pixels_fn(png_cache_t* cache);
CUTE_API strpool_t* CUTE_CALL png_cache_get_strpool_ptr(png_cache_t* cache);

CUTE_API const animation_t* CUTE_CALL png_cache_make_animation(png_cache_t* cache, const char* name, const array<png_t>& pngs, const array<float>& delays);
CUTE_API const animation_t* CUTE_CALL png_cache_get_animation(png_cache_t* cache, const char* name);
CUTE_API const animation_table_t* CUTE_CALL png_cache_make_animation_table(png_cache_t* cache, const char* sprite_name, const array<const animation_t*>& animations);
CUTE_API const animation_table_t* CUTE_CALL png_cache_get_animation_table(png_cache_t* cache, const char* sprite_name);
CUTE_API sprite_t CUTE_CALL png_cache_make_sprite(png_cache_t* cache, const char* sprite_name, const animation_table_t* table = NULL);

}

#endif // CUTE_PNG_CACHE_H
