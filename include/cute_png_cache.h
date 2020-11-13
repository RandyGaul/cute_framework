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
#include <cute_batch.h>
#include <cute_sprite.h>

#include <cute/cute_png.h>

namespace cute
{

/**
 * The png cache is used to load png images from disk in order to make sprites.
 * 
 * You will mostly just care about these three functions.
 * 
 *     png_cache_load
 *     png_cache_unload
 *     png_cache_make_sprite
 * 
 * It's a cache, which means it actually caches images loaded in RAM, so subsequent
 * calls to `png_cache_load` won't have to fetch the image off of disk, as long as
 * the image is currently cached in RAM.
 */
struct png_cache_t;
struct strpool_t;

/**
 * Constructs a new png cache. Destroy it with `png_cache_destroy` when done with it.
 */
CUTE_API png_cache_t* CUTE_CALL png_cache_make(void* mem_ctx = NULL);

/**
 * Destroys a png cache previously made with `png_cache_make`.
 */
CUTE_API void CUTE_CALL png_cache_destroy(png_cache_t* cache);

/**
 * A single image of raw pixels, loaded from a png cache.
 */
struct png_t
{
	const char* path = NULL;
	uint64_t id = ~0;
	pixel_t* pix = NULL;
	int w = 0;
	int h = 0;
};

/**
 * Returns an image from the cache. If it does not exist in the cache, it is loaded from disk
 * and placed into the cache.
 */
CUTE_API error_t CUTE_CALL png_cache_load(png_cache_t* cache, const char* png_path, png_t* png = NULL);

/**
 * Returns an image from the cache. If it does not exist in the cache, it is loaded from memory
 * and placed into the cache.
 */
CUTE_API error_t CUTE_CALL png_cache_load_mem(png_cache_t* cache, const char* png_path, const void* memory, size_t size, png_t* png = NULL);

/**
 * Unloads an image from the cache. This function can be used to control your RAM usage, for example
 * when switching from one level/area to another can be a good time to unload images that will no
 * longer be used.
 */
CUTE_API void CUTE_CALL png_cache_unload(png_cache_t* cache, png_t* png);

/**
 * `png_cache_get_pixels_fn` is needed to hook up to `batch_t` in order to draw sprites.
 * The return value gets passed to `batch_make`.
 */
CUTE_API get_pixels_fn* CUTE_CALL png_cache_get_pixels_fn(png_cache_t* cache);

/**
 * This is a low-level function, just in case anyone wants to get access to the internal string pool.
 * Only use this function if you know what you're doing.
 */
CUTE_API strpool_t* CUTE_CALL png_cache_get_strpool_ptr(png_cache_t* cache);

//--------------------------------------------------------------------------------------------------
// Animation and sprite functions.
// Since png files do not contain any kind of animation information (frame delays or sets of frames)
// you must specify all of the animation data yourself in order to make sprites. The various functions
// in this section are for setting up animation data.

/**
 * Constructs an animation out of an array of frames, along with their delays in milliseconds.
 * The animation is stored within the png cache.
 */
CUTE_API const animation_t* CUTE_CALL png_cache_make_animation(png_cache_t* cache, const char* name, const array<png_t>& pngs, const array<float>& delays);

/**
 * Looks up an animation within the png cache by name.
 */
CUTE_API const animation_t* CUTE_CALL png_cache_get_animation(png_cache_t* cache, const char* name);

/**
 * Constructs an animation table given an array of animations. The table is stored within the png cache.
 */
CUTE_API const animation_table_t* CUTE_CALL png_cache_make_animation_table(png_cache_t* cache, const char* sprite_name, const array<const animation_t*>& animations);

/**
 * Looks up an animation table within the png cache by name.
 */
CUTE_API const animation_table_t* CUTE_CALL png_cache_get_animation_table(png_cache_t* cache, const char* sprite_name);

/**
 * Makes a sprite. Each sprite must refer to an animation table previously constructed by `png_cache_make_animation_table`.
 * You can supply the pointer to the animation table yourself in `table`, or just leave it NULL.
 * If table is `NULL` then `sprite_name` is used to lookup the table within the png cache.
 */
CUTE_API sprite_t CUTE_CALL png_cache_make_sprite(png_cache_t* cache, const char* sprite_name, const animation_table_t* table = NULL);

}

#endif // CUTE_PNG_CACHE_H
