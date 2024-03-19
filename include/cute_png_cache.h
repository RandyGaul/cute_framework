/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
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
 * @struct   CF_Png
 * @category png_cache
 * @brief    A single image of raw pixels, loaded from a png cache.
 * @remarks  The png cache is used to load png images from disk in order to make sprites. The png cache
 *           system is an advanced option for people who want lower level access to creating their own
 *           custom sprites, for example by loading sprites from their own custom animation format. If you just
 *           want a really easy way to load sprites, look at cute_sprite.h for the easy sprite API (search for
 *           `easy_make_sprite` or `make_sprite` functions).
 *           
 *           You will mostly just care about these three functions.
 *           
 *               cf_png_cache_load
 *               cf_png_cache_unload
 *               cf_make_png_cache_sprite
 *           
 *           It's a cache, which means it actually caches images loaded in RAM, so subsequent
 *           calls to `cf_png_cache_load` won't have to fetch the image off of disk, as long as
 *           the image is currently cached in RAM.
 * @related  CF_Png cf_png_defaults cf_png_cache_load cf_make_png_cache_animation cf_make_png_cache_sprite
 */
typedef struct CF_Png
{
	/* @member Path to the associated png on-disk. */
	const char* path;

	/* @member Unique identifier assigned by the cache. */
	uint64_t id;

	/* @member Pointer to the pixels of the png image. */
	CF_Pixel* pix;

	/* @member Width of the image in pixels. */
	int w;

	/* @member Height of the image in pixels. */
	int h;
} CF_Png;
// @end

/**
 * @function cf_png_defaults
 * @category png_cache
 * @brief    Initialize an empty `CF_Png` struct.
 * @return   Returns an empty `CF_Png` struct.
 * @related  CF_Png cf_png_defaults cf_png_cache_load cf_make_png_cache_animation cf_make_png_cache_sprite
 */
CF_INLINE CF_Png cf_png_defaults()
{
	CF_Png result = { 0 };
	result.id = ~0;
	return result;
}

/**
 * @function cf_png_cache_load
 * @category png_cache
 * @brief    Returns an image `CF_Png` from the cache.
 * @remarks  If it does not exist in the cache, it is loaded from disk and placed into the cache.
 * @related  CF_Png cf_png_defaults cf_png_cache_load cf_make_png_cache_animation cf_make_png_cache_sprite
 */
CF_API CF_Result CF_CALL cf_png_cache_load(const char* png_path, CF_Png* png /*= NULL*/);

/**
 * @function cf_png_cache_load_from_memory
 * @category png_cache
 * @brief    Returns an image `CF_Png` from the cache.
 * @remarks  If it does not exist in the cache, it is loaded from memory and placed into the cache.
 * @related  CF_Png cf_png_defaults cf_png_cache_load cf_make_png_cache_animation cf_make_png_cache_sprite
 */
CF_API CF_Result CF_CALL cf_png_cache_load_from_memory(const char* png_path, const void* memory, size_t size, CF_Png* png /*= NULL*/);

/**
 * @function cf_png_cache_unload
 * @category png_cache
 * @brief    Unloads an image from the cache.
 * @related  CF_Png cf_png_defaults cf_png_cache_load cf_make_png_cache_animation cf_make_png_cache_sprite
 */
CF_API void CF_CALL cf_png_cache_unload(CF_Png png);

/**
 * @function cf_make_png_cache_animation
 * @category png_cache
 * @brief    Constructs an animation out of an array of frames, along with their delays in milliseconds, or returns one from the cache if it already exists.
 * @param    name          A unique name for the animation.
 * @param    pngs          An array of pngs, see `CF_Png`.
 * @param    pngs_count    The number of images in the `pngs` array.
 * @param    delays        A delay in milliseconds for each frame of the animation.
 * @param    delays_count  The number of floats in the `delays` array. This must match `pngs_count`.
 * @return   Returns a `CF_Animation`, representing a single animation sequence.
 * @remarks  Since png files do not contain any kind of animation information (frame delays or sets of frames)
 *           you must specify all of the animation data yourself in order to make sprites. To flip between different
 *           animations you can construct an animation table with `cf_make_png_cache_animation_table`, which returns
 *           a table of unique animation names to individual `CF_Animation`'s.
 * @related  CF_Png cf_png_cache_load cf_make_png_cache_animation cf_make_png_cache_animation_table cf_make_png_cache_sprite
 */
CF_API const CF_Animation* CF_CALL cf_make_png_cache_animation(const char* name, const CF_Png* pngs, int pngs_count, const float* delays, int delays_count);

/**
 * @function cf_png_cache_get_animation
 * @category png_cache
 * @brief    Looks up an animation within the png cache by name.
 * @param    name          A unique name for the animation.
 * @return   Returns a `CF_Animation`.
 * @remarks  You may use this function if you wish to implement your own sprites. However, it's recommended to instead use
 *           `cf_make_png_cache_sprite` and `CF_Sprite`.
 * @related  CF_Png cf_png_cache_load cf_make_png_cache_animation cf_make_png_cache_animation_table cf_make_png_cache_sprite
 */
CF_API const CF_Animation* CF_CALL cf_png_cache_get_animation(const char* name);

/**
 * @function cf_make_png_cache_animation_table
 * @category png_cache
 * @brief    Constructs an animation table given an array of animations, or returns one from the cache if it already exists.
 * @param    sprite_name  A unique name for the animation table.
 * @return   Returns a `CF_Animation` hashtable, see `htbl`.
 * @remarks  The table returned is a map of animation names to individual `CF_Animation`'s. This is represents the guts of a sprite
 *           implementation. You may use this function if you wish to implement your own sprites. However, it's recommended to instead use
 *           `cf_make_png_cache_sprite` and `CF_Sprite`.
 * @related  CF_Png cf_png_cache_load cf_make_png_cache_animation cf_make_png_cache_animation_table cf_make_png_cache_sprite
 */
CF_API const htbl CF_Animation** CF_CALL cf_make_png_cache_animation_table(const char* sprite_name, const CF_Animation* const* animations, int animations_count);

/**
 * @function cf_png_cache_get_animation_table
 * @category png_cache
 * @brief    Looks up an animation table within the png cache by name.
 * @param    sprite_name  A unique name for the animation table.
 * @return   Returns a hashtable of unique sprite names to `CF_Animation`'s, see `cf_make_png_cache_sprite`.
 * @related  CF_Png cf_png_cache_load cf_make_png_cache_animation cf_make_png_cache_animation_table cf_make_png_cache_sprite
 */
CF_API const htbl CF_Animation** CF_CALL cf_png_cache_get_animation_table(const char* sprite_name);

/**
 * Makes a sprite. Each sprite must refer to an animation table previously constructed by `cf_make_png_cache_animation_table`.
 * You can supply the pointer to the animation table yourself in `table`, or just leave it NULL.
 * If table is `NULL` then `sprite_name` should be the path to your png file instead of the sprite's actual name.
 */
/**
 * @function cf_make_png_cache_sprite
 * @category png_cache
 * @brief    Constructs an `CF_Sprite` out of your own custom built animation table.
 * @param    sprite_name  A unique name for the sprite.
 * @param    table        An animation table.
 * @return   Returns a `CF_Sprite`, ready to use in e.g. `draw_sprite`.
 * @related  CF_Png cf_png_cache_load cf_make_png_cache_animation cf_make_png_cache_animation_table cf_make_png_cache_sprite
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
CF_API const Animation** CF_CALL make_png_cache_animation_table(const char* sprite_name, const Array<const Animation*>& animations);
CF_INLINE const Animation** png_cache_get_animation_table(const char* sprite_name) { return cf_png_cache_get_animation_table(sprite_name); }
CF_INLINE Sprite make_png_cache_sprite(const char* sprite_name, const CF_Animation** table = NULL) { return cf_make_png_cache_sprite(sprite_name, table); }

}

#endif // CF_CPP

#endif // CF_PNG_CACHE_H
