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

#ifndef CUTE_ASEPRITE_CACHE_H
#define CUTE_ASEPRITE_CACHE_H

#include <cute_defines.h>
#include <cute_error.h>
#include <cute_batch.h>

#include <cute/cute_aseprite.h>

namespace cute
{

/**
 * The aseprite cache is used to load ase files from disk in order to make sprites.
 * 
 * You will mostly just care about these three functions.
 * 
 *     aseprite_cache_load
 *     aseprite_cache_unload
 *     aseprite_cache_make_sprite
 * 
 * It's a cache, which means it actually caches aseprites loaded in RAM, so subsequent
 * calls to `aseprite_cache_load` won't have to fetch the image off of disk, as long as
 * the image is currently cached in RAM.
 */
struct aseprite_cache_t;
struct sprite_t;
struct strpool_t;

/**
 * Constructs a new aseprite cache. Destroy it with `aseprite_cache_destroy` when done with it.
 */
CUTE_API aseprite_cache_t* CUTE_CALL aseprite_cache_make(void* mem_ctx = NULL);

/**
 * Destroys a aseprite cache previously made with `aseprite_cache_make`.
 */
CUTE_API void CUTE_CALL aseprite_cache_destroy(aseprite_cache_t* cache);

/**
 * Returns a sprite from the cache. If it does not exist in the cache, it is loaded from disk
 * and placed into the cache.
 */
CUTE_API error_t CUTE_CALL aseprite_cache_load(aseprite_cache_t* cache, const char* aseprite_path, sprite_t* sprite);

/**
 * Returns a sprite from the cache. If it does not exist in the cache, it is loaded from memory
 * and placed into the cache.
 */
CUTE_API void CUTE_CALL aseprite_cache_unload(aseprite_cache_t* cache, const char* aseprite_path);

/**
 * A low-level function used to return an `ase_t` from the cache. If it does not exist within the cache
 * it is loaded from disk.
 * 
 * This function is typically not necessary to call. You might be looking for `aseprite_cache_load`
 * instead.
 * 
 * Only call this function if you know what you're doing.
 */
CUTE_API error_t CUTE_CALL aseprite_cache_load_ase(aseprite_cache_t* cache, const char* aseprite_path, ase_t** ase);

/**
 * `png_cache_get_pixels_fn` is needed to hook up to `batch_t` in order to draw sprites.
 * The return value gets passed to `batch_make`.
 */
CUTE_API get_pixels_fn* CUTE_CALL aseprite_cache_get_pixels_fn(aseprite_cache_t* cache);

/**
 * This is a low-level function, just in case anyone wants to get access to the internal string pool.
 * Only use this function if you know what you're doing.
 */
CUTE_API strpool_t* CUTE_CALL aseprite_cache_get_strpool_ptr(aseprite_cache_t* cache);

}

#endif // CUTE_ASEPRITE_CACHE_H
