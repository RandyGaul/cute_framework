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

#ifndef CUTE_BATCH_H
#define CUTE_BATCH_H

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
struct batch_quad_t
{
	/**
	 * Unique identifier for this quad's image, as determined by you.
	 */
	uint64_t id;

	transform_t transform = make_transform(); // Position and location rotation of the quad.
	int w; // Width in pixels of the source image.
	int h; // Height in pixels of the source image.
	float scale_x; // Scaling along the quad's local x-axis in pixels.
	float scale_y; // Scaling along the quad's local y-axis in pixels.
	float alpha = 1.0f; // Applies additional alpha to this quad.

	int sort_bits = 0;
};

/**
 * The batch is used to buffer up many different drawable things and organize them into draw calls suitable for high-
 * performance rendering on the GPU. However, this batch is not your typical batcher. This one will build texture
 * atlases internally on the the fly, and periodically will need to fetch pixels to build atlases.
 * 
 * This means you don't have to worry about texture atlases at all, and can build and ship your game with separate
 * images on disk.
 * 
 * If you'd like to read more about the implementation of the batcher and why this is a good idea, go ahead and read
 * the documentation in `cute_spritebatch.h` in the `cute` folder.
 */
struct batch_t;

/**
 * `get_pixels_fn` will be called periodically from within `batch_flush` whenever access to pixels in RAM are
 * needed to construct internal texture atlases to be sent to the GPU.
 * 
 * `image_id`      - Uniquely maps to a single image, as determined by you.
 * `buffer`        - Pointer to the memory where you need to fill in pixel data.
 * `bytes_to_fill` - Number of bytes to write to `buffer`.
 * `udata`         - The `udata` pointer that was originally passed to `batch_enable_custom_pixel_loader`.
 */
typedef void (get_pixels_fn)(uint64_t image_id, void* buffer, int bytes_to_fill, void* udata);

extern CUTE_API batch_t* CUTE_CALL batch_make(app_t* app, get_pixels_fn* get_pixels, void* get_pixels_udata);
extern CUTE_API void CUTE_CALL batch_destroy(batch_t* b);

/**
 * Pushes quad onto an internal buffer. Does no other logic.
 * 
 * To get your quad rendered, see `batch_flush`.
 */
extern CUTE_API void CUTE_CALL batch_push(batch_t* b, batch_quad_t quad);

/**
 * All quads currently pushed onto the batch (see `batch_push`) will be converted to an internal draw call.
 */
extern CUTE_API error_t CUTE_CALL batch_flush(batch_t* b);

enum batch_quad_shader_type_t
{
	BATCH_QUAD_SHADER_TYPE_DEFAULT,
	BATCH_QUAD_SHADER_TYPE_OUTLINE,
	BATCH_QUAD_SHADER_TYPE_TINT
};

extern CUTE_API void CUTE_CALL batch_set_shader_type(batch_t* b, batch_quad_shader_type_t type);
extern CUTE_API void CUTE_CALL batch_set_mvp(batch_t* b, matrix_t mvp);
extern CUTE_API void CUTE_CALL batch_set_scissor_box(batch_t* b, int x, int y, int w, int h);
extern CUTE_API void CUTE_CALL batch_no_scissor_box(batch_t* b);
extern CUTE_API void CUTE_CALL batch_outlines_use_border(batch_t* b, bool use_border);
extern CUTE_API void CUTE_CALL batch_set_depth_stencil_state(batch_t* b, const sg_depth_stencil_state& depth_stencil_state);
extern CUTE_API void CUTE_CALL batch_set_depth_stencil_defaults(batch_t* b);
extern CUTE_API void CUTE_CALL batch_set_blend_state(batch_t* b, const sg_blend_state& blend_state);
extern CUTE_API void CUTE_CALL batch_set_blend_defaults(batch_t* b);

}

#endif // CUTE_BATCH_H
