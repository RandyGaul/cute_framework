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

	transform_t transform; // Position and location rotation of the sprite.
	float scale_x; // Scaling along the sprite's local x-axis.
	float scale_y; // Scaling along the sprite's local y-axis.
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
struct sprite_batch_t;

sprite_batch_t* sprite_batch_make(gfx_t* gfx);
void sprite_batch_destroy(sprite_batch_t* sb);

/**
 * Pushes `sprite` onto an internal buffer. Does no other logic.
 *
 * To get your sprite rendered, see `sprite_batch_flush`.
 */
error_t sprite_batch_push(sprite_batch_t* sb, sprite_t sprite);

/**
 * All sprites currently pushed onto the sprite batch (see `sprite_batch_push`) will be converted to an internal
 * `gfx_draw_call_t`, and the draw call will be pushed onto the `gfx` draw call buffer.
 *
 * To then get your sprites drawn, simply call `gfx_flush` on the `gfx` instance originally used for `sprite_batch_make`.
 */
error_t sprite_batch_flush(sprite_batch_t* sb);

/**
 * Sets up an LRU cache (least-recently-used cache) that will periodically fetch images from disk as-needed. The exact
 * size of the internal cache in RAM is set by `cache_size_in_bytes`. The idea is that pixels will be stored in RAM in
 * the cache, and whenever there isn't enough space the least recently used pixels will be evicted, and that memory
 * will be re-used for the next image that needs to be loaded into RAM.
 * 
 * An LRU cache like this is not the only way to handle loading pixels onto the GPU -- see `sprite_batch_enable_custom_pixel_loader`
 * if you want full control over how pixels are dealt with.
 */
error_t sprite_batch_enable_disk_LRU_cache(sprite_batch_t* sb, const char** image_paths, size_t cache_size_in_bytes);

/**
 * This is an advanced function. It is recommended to use `sprite_batch_enable_disk_LRU_cache`, unless you want to handle
 * memory in a custom way. `get_pixels_fn` will be called periodically from within `sprite_batch_flush` whenever access to
 * pixels in RAM are needed to construct internal texture atlases to be sent to the GPU.
 * 
 * `image_id`      - Uniquely maps to a single sprite.
 * `buffer`        - Pointer to the memory where you need to fill in pixel data.
 * `bytes_to_fill` - Number of bytes to write to `buffer`.
 * `udata`         - The `udata` pointer that was originally passed to `sprite_batch_enable_custom_pixel_loader`.
 */
error_t sprite_batch_enable_custom_pixel_loader(sprite_batch_t* sb, void (*get_pixels_fn)(uint64_t image_id, void* buffer, int bytes_to_fill, void* udata), void* udata);

}

#endif // CUTE_SPRITE_H
