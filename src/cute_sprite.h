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

#ifndef CUTE_SPRITE_H
#define CUTE_SPRITE_H

#include <cute_defines.h>
#include <cute_math.h>
#include <cute_error.h>
#include <cute_gfx.h>

// TODO - Customizeability of the shader.

namespace cute
{

/**
 * Represents a single image rendered as a quad.
 */
struct sprite_t
{
	/**
	 * Unique identifier for this sprite's image.
	 * 
	 * If using LRU cache (see `sprite_batch_enable_disk_LRU_cache`) then this id lines up with the index
	 * of `image_paths`.
	 * 
	 * If using a custom pixel loader (see `sprite_batch_enable_custom_pixel_loader`) then which sprite
	 * this id maps to is completely up to you.
	 */
	uint64_t id;

	transform_t transform = make_transform(); // Position and location rotation of the sprite.
	int w; // Width in pixels of the source image.
	int h; // Height in pixels of the source image.
	float scale_x; // Scaling along the sprite's local x-axis in pixels.
	float scale_y; // Scaling along the sprite's local y-axis in pixels.

	int sort_bits = 0;
};

/**
 * The sprite batch is used to buffer up many different sprites and organize them into draw calls suitable for high-
 * performance rendering on the GPU. However, this sprite batch is not your typical sprite batcher. This one will
 * build texture atlases internally on the the fly, and periodically will need to fetch pixels to build atlases.
 * 
 * This means you don't have to worry about texture atlases at all, and can build and ship your game with separate
 * images on disk.
 * 
 * If you'd like to read more about the implementation of the batcher and why this is a good idea, go ahead and read
 * the documentation in `cute_spritebatch.h` in the `cute` folder.
 */
struct spritebatch_t;

extern CUTE_API spritebatch_t* CUTE_CALL sprite_batch_make(app_t* app);
extern CUTE_API void CUTE_CALL sprite_batch_destroy(spritebatch_t* sb);

/**
 * The easiest way to setup a spritebatch. Simply point to a `path` and all the png files within
 * this path will be recursively searched for and found. Each png will map to one sprite, and can
 * be found with the `sprite_batch_easy_sprite` function.
 *
 * This function is mostly just to get you started. You will may soon graduate to using 
 * `sprite_batch_enable_disk_LRU_cache` instead, by building your own list of image paths yourself.
 */
extern CUTE_API spritebatch_t* CUTE_CALL sprite_batch_easy_make(app_t* app, const char* path);

/**
 * Fills out a `sprite_t` struct for given .png path. This function can only be used if
 * `sprite_batch_easy_make` is called to setup the spritebatch.
 */
extern CUTE_API error_t CUTE_CALL sprite_batch_easy_sprite(spritebatch_t* sb, const char* path, sprite_t* sprite);

/**
 * Pushes `sprite` onto an internal buffer. Does no other logic.
 * 
 * To get your sprite rendered, see `sprite_batch_flush`.
 */
extern CUTE_API void CUTE_CALL sprite_batch_push(spritebatch_t* sb, sprite_t sprite);

/**
 * All sprites currently pushed onto the sprite batch (see `sprite_batch_push`) will be converted to an internal
 * `gfx_draw_call_t`, and the draw call will be pushed onto the `gfx` draw call buffer.
 * 
 * To then get your sprites drawn, simply call `gfx_flush` on the `gfx` instance originally used for `sprite_batch_make`.
 */
extern CUTE_API error_t CUTE_CALL sprite_batch_flush(spritebatch_t* sb);

enum sprite_shader_type_t
{
	SPRITE_SHADER_TYPE_DEFAULT,
	SPRITE_SHADER_TYPE_OUTLINE,
	SPRITE_SHADER_TYPE_TINT
};

extern CUTE_API void CUTE_CALL sprite_batch_set_shader_type(spritebatch_t* sb, sprite_shader_type_t type);
extern CUTE_API void CUTE_CALL sprite_batch_set_mvp(spritebatch_t* sb, gfx_matrix_t mvp);
extern CUTE_API void CUTE_CALL sprite_batch_set_scissor_box(spritebatch_t* sb, aabb_t scissor);
extern CUTE_API void CUTE_CALL sprite_batch_no_scissor_box(spritebatch_t* sb);
extern CUTE_API void CUTE_CALL sprite_batch_outlines_use_border(spritebatch_t* sb, int use_border);

/**
 * This is the recommended way to setup the sprite batch. The sprite batch periodically reads images from disk as-needed.
 * 
 * image_paths         - An array of strings, one string for each image on disk. The index of each image will be used to
 *                       uniquely identify each sprite. `image_paths` must persist in memory!
 * cache_size_in_bytes - Size in bytes of the internal cache used to store pixels in RAM. 1024 * 1024 * 250 (or, 250 MB)
 *                       is a good number to start with, which is about 1/4th of a gigabyte. The smaller the number means
 *                       the disk will be accessed more frequently, and the larger the number means the disk will be
 *                       accessed less often.
 * 
 * Sets up an LRU cache (least-recently-used cache) that will periodically fetch images from disk as-needed. The exact
 * size of the internal cache in RAM is set by `cache_size_in_bytes`. The idea is that pixels will be stored in RAM in
 * the cache, and whenever there isn't enough space the least recently used pixels will be evicted, and that memory
 * will be re-used for the next image that needs to be loaded into RAM.
 * 
 * An LRU cache like this is not the only way to handle loading pixels onto the GPU -- see `sprite_batch_enable_custom_pixel_loader`
 * if you want full control over how pixels are dealt with.
 */
extern CUTE_API error_t CUTE_CALL sprite_batch_enable_disk_LRU_cache(spritebatch_t* sb, const char** image_paths, int image_paths_count, size_t cache_size_in_bytes);

/**
 * This function is for advanced users. The idea is to allow optimizations to have more control over when the disk is accessed.
 * 
 * This function will load the image associated with `id` (the index into `image_paths` used when you called
 * `sprite_batch_enable_disk_LRU_cache`) into the LRU cache. The least recently used images will be evicted to make room.
 */
extern CUTE_API error_t CUTE_CALL sprite_batch_LRU_cache_prefetch(spritebatch_t* sb, uint64_t id);

/**
 * Unloads all images in the cache and frees up the RAM they used.
 */
extern CUTE_API void CUTE_CALL sprite_batch_LRU_cache_clear(spritebatch_t* sb);

/**
 * This is an advanced function. It is recommended to use `sprite_batch_enable_disk_LRU_cache`, unless you want to handle
 * memory in a custom way. `get_pixels_fn` will be called periodically from within `sprite_batch_flush` whenever access to
 * pixels in RAM are needed to construct internal texture atlases to be sent to the GPU.
 * 
 * `image_id`      - Uniquely maps to a single sprite, as specified by you.
 * `buffer`        - Pointer to the memory where you need to fill in pixel data.
 * `bytes_to_fill` - Number of bytes to write to `buffer`.
 * `udata`         - The `udata` pointer that was originally passed to `sprite_batch_enable_custom_pixel_loader`.
 */
extern CUTE_API error_t CUTE_CALL sprite_batch_enable_custom_pixel_loader(spritebatch_t* sb, void (*get_pixels_fn)(uint64_t image_id, void* buffer, int bytes_to_fill, void* udata), void* udata);

}

#endif // CUTE_SPRITE_H
