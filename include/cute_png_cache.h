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

#ifndef CF_PNG_CACHE_H
#define CF_PNG_CACHE_H

#include "cute_defines.h"
#include "cute_result.h"
#include "cute_color.h"
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

/**
 * A single image of raw pixels, loaded from a png cache.
 */
typedef struct CF_Png
{
	const char* path;
	uint64_t id;
	CF_Pixel* pix;
	int w;
	int h;
} CF_Png;

CF_INLINE CF_Png cf_png_defaults()
{
	CF_Png result = { 0 };
	result.id = ~0;
	return result;
}

/**
 * Returns an image from the cache. If it does not exist in the cache, it is loaded from disk
 * and placed into the cache.
 */
CF_API CF_Result CF_CALL cf_png_cache_load(const char* png_path, CF_Png* png /*= NULL*/);

/**
 * Returns an image from the cache. If it does not exist in the cache, it is loaded from memory
 * and placed into the cache.
 */
CF_API CF_Result CF_CALL cf_png_cache_load_from_memory(const char* png_path, const void* memory, size_t size, CF_Png* png /*= NULL*/);

/**
 * Unloads an image from the cache. This function can be used to control your RAM usage, for example
 * when switching from one level/area to another can be a good time to unload images that will no
 * longer be used.
 */
CF_API void CF_CALL cf_png_cache_unload(CF_Png png);

//--------------------------------------------------------------------------------------------------
// Animation and sprite functions.
// Since png files do not contain any kind of animation information (frame delays or sets of frames)
// you must specify all of the animation data yourself in order to make sprites. The various functions
// in this section are for setting up animation data.

/**
 * Constructs an animation out of an array of frames, along with their delays in milliseconds.
 * The animation is stored within the png cache.
 */
CF_API const CF_Animation* CF_CALL cf_make_png_cache_animation(const char* name, const CF_Png* pngs, int pngs_count, const float* delays, int delays_count);

/**
 * Looks up an animation within the png cache by name.
 */
CF_API const CF_Animation* CF_CALL cf_png_cache_get_animation(const char* name);

/**
 * Constructs an animation table given an array of animations. The table is stored within the png cache.
 */
CF_API const CF_Animation** CF_CALL cf_make_png_cache_animation_table(const char* sprite_name, const CF_Animation* const* animations, int animations_count);

/**
 * Looks up an animation table within the png cache by name.
 */
CF_API const CF_Animation** CF_CALL cf_png_cache_get_animation_table(const char* sprite_name);

/**
 * Makes a sprite. Each sprite must refer to an animation table previously constructed by `cf_make_png_cache_animation_table`.
 * You can supply the pointer to the animation table yourself in `table`, or just leave it NULL.
 * If table is `NULL` then `sprite_name` should be the path to your png file instead of the sprite's actual name.
 */
CF_API CF_Sprite CF_CALL cf_make_png_cache_sprite(const char* sprite_name, const CF_Animation** table /*= NULL*/);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

#include "cute_array.h"

namespace Cute
{

using Animation = CF_Animation;

struct Png : public CF_Png
{
	Png() { *(CF_Png*)this = cf_png_defaults(); }
	Png(CF_Png png) { *(CF_Png*)this = png; }
};

CF_INLINE Result png_cache_load(const char* png_path, Png* png = NULL) { return cf_png_cache_load(png_path, (CF_Png*)png); }
CF_INLINE Result png_cache_load_mem(const char* png_path, const void* memory, size_t size, CF_Png* png = NULL) { return cf_png_cache_load_from_memory(png_path, memory, size, png); }
CF_INLINE void png_cache_unload(Png png) { cf_png_cache_unload(png); }
CF_API const Animation* CF_CALL make_png_cache_animation(const char* name, const Array<CF_Png>& pngs, const Array<float>& delays);
CF_INLINE const Animation* png_cache_get_animation(const char* name) { return cf_png_cache_get_animation(name); }
CF_API const CF_Animation** CF_CALL make_png_cache_animation_table(const char* sprite_name, const Array<const Animation*>& animations);
CF_INLINE const CF_Animation** png_cache_get_animation_table(const char* sprite_name) { return cf_png_cache_get_animation_table(sprite_name); }

}

#endif // CF_CPP

#endif // CF_PNG_CACHE_H
