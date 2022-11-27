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

#include "cute_defines.h"
#include "cute_result.h"
#include "cute_color.h"
#include "cute_batch.h"
#include "cute_sprite.h"

#include "cute/cute_png.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * The png cache is used to load png images from disk in order to make sprites. The png cache
 * system is an advanced option for people who want lower level access to creating their own
 * custom sprites, for example by loading sprites from their own custom animation format. If you just
 * want a really easy way to load sprites, look at cute_sprite.h for the easy sprite API (search for
 * `easy_make_sprite` or `make_sprite` functions).
 *
 * You will mostly just care about these three functions.
 *
 *     cf_png_cache_load
 *     cf_png_cache_unload
 *     cf_make_png_cache_sprite
 *
 * It's a cache, which means it actually caches images loaded in RAM, so subsequent
 * calls to `cf_png_cache_load` won't have to fetch the image off of disk, as long as
 * the image is currently cached in RAM.
 */
typedef struct cf_png_cache_t cf_png_cache_t;

/**
 * Constructs a new png cache. Destroy it with `cf_destroy_png_cache` when done with it.
 */
CUTE_API cf_png_cache_t* CUTE_CALL cf_make_png_cache();

/**
 * Destroys a png cache previously made with `cf_make_png_cache`.
 */
CUTE_API void CUTE_CALL cf_destroy_png_cache(cf_png_cache_t* cache);

/**
 * A single image of raw pixels, loaded from a png cache.
 */
typedef struct cf_png_t
{
	const char* path; /*= NULL*/
	uint64_t id; /*= ~0*/
	CF_Pixel* pix; /*= NULL*/
	int w; /*= 0*/
	int h; /*= 0*/
} cf_png_t;

/*
* 	path = NULL;
*	id = ~0;
*	pix = NULL;
*	w = 0;
*	h = 0;
*/
CUTE_INLINE cf_png_t cf_png_defaults()
{
	cf_png_t result = { 0 };
	result.id = ~0;
	return result;
}

/**
 * Returns an image from the cache. If it does not exist in the cache, it is loaded from disk
 * and placed into the cache.
 */
CUTE_API cf_result_t CUTE_CALL cf_png_cache_load(cf_png_cache_t* cache, const char* png_path, cf_png_t* png /*= NULL*/);

/**
 * Returns an image from the cache. If it does not exist in the cache, it is loaded from memory
 * and placed into the cache.
 */
CUTE_API cf_result_t CUTE_CALL cf_png_cache_load_mem(cf_png_cache_t* cache, const char* png_path, const void* memory, size_t size, cf_png_t* png /*= NULL*/);

/**
 * Unloads an image from the cache. This function can be used to control your RAM usage, for example
 * when switching from one level/area to another can be a good time to unload images that will no
 * longer be used.
 */
CUTE_API void CUTE_CALL cf_png_cache_unload(cf_png_cache_t* cache, cf_png_t png);

//--------------------------------------------------------------------------------------------------
// Animation and sprite functions.
// Since png files do not contain any kind of animation information (frame delays or sets of frames)
// you must specify all of the animation data yourself in order to make sprites. The various functions
// in this section are for setting up animation data.

/**
 * Constructs an animation out of an array of frames, along with their delays in milliseconds.
 * The animation is stored within the png cache.
 */
CUTE_API const cf_animation_t* CUTE_CALL cf_make_png_cache_animation(cf_png_cache_t* cache, const char* name, const cf_png_t* pngs, int pngs_count, const float* delays, int delays_count);

/**
 * Looks up an animation within the png cache by name.
 */
CUTE_API const cf_animation_t* CUTE_CALL cf_png_cache_get_animation(cf_png_cache_t* cache, const char* name);

/**
 * Constructs an animation table given an array of animations. The table is stored within the png cache.
 */
CUTE_API const cf_animation_t** CUTE_CALL cf_make_png_cache_animation_table(cf_png_cache_t* cache, const char* sprite_name, const cf_animation_t* const* animations, int animations_count);

/**
 * Looks up an animation table within the png cache by name.
 */
CUTE_API const cf_animation_t** CUTE_CALL cf_png_cache_get_animation_table(cf_png_cache_t* cache, const char* sprite_name);

/**
 * Makes a sprite. Each sprite must refer to an animation table previously constructed by `cf_make_png_cache_animation_table`.
 * You can supply the pointer to the animation table yourself in `table`, or just leave it NULL.
 * If table is `NULL` then `sprite_name` is used to lookup the table within the png cache.
 */
CUTE_API cf_sprite_t CUTE_CALL cf_make_png_cache_sprite(cf_png_cache_t* cache, const char* sprite_name, const cf_animation_t** table /*= NULL*/);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

#include "cute_array.h"

namespace cute
{

using png_cache_t = cf_png_cache_t;
using animation_t = cf_animation_t;

struct png_t : public cf_png_t
{
	png_t() { *(cf_png_t*)this = cf_png_defaults(); }
	png_t(cf_png_t png) { *(cf_png_t*)this = png; }
};

CUTE_INLINE png_cache_t* make_png_cache() { return cf_make_png_cache(); }
CUTE_INLINE void destroy_png_cache(png_cache_t* cache) { return cf_destroy_png_cache(cache); }
CUTE_INLINE result_t png_cache_load(png_cache_t* cache, const char* png_path, png_t* png = NULL) { return cf_png_cache_load(cache, png_path, (cf_png_t*)png); }
CUTE_INLINE result_t png_cache_load_mem(png_cache_t* cache, const char* png_path, const void* memory, size_t size, cf_png_t* png = NULL) { return cf_png_cache_load_mem(cache, png_path, memory, size, png); }
CUTE_INLINE void png_cache_unload(png_cache_t* cache, png_t png) { cf_png_cache_unload(cache, png); }
CUTE_API const animation_t* CUTE_CALL make_png_cache_animation(png_cache_t* cache, const char* name, const array<cf_png_t>& pngs, const array<float>& delays);
CUTE_INLINE const animation_t* png_cache_get_animation(png_cache_t* cache, const char* name) { return cf_png_cache_get_animation(cache, name); }
CUTE_API const cf_animation_t** CUTE_CALL make_png_cache_animation_table(png_cache_t* cache, const char* sprite_name, const array<const animation_t*>& animations);
CUTE_INLINE const cf_animation_t** png_cache_get_animation_table(png_cache_t* cache, const char* sprite_name) { return cf_png_cache_get_animation_table(cache, sprite_name); }
CUTE_INLINE sprite_t make_png_cache_sprite(png_cache_t* cache, const char* sprite_name, const cf_animation_t** table = NULL) { return cf_make_png_cache_sprite(cache, sprite_name, table); }

}

#endif // CUTE_CPP

#endif // CUTE_PNG_CACHE_H
