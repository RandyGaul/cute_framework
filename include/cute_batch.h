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

#include "cute_defines.h"
#include "cute_math.h"
#include "cute_error.h"
#include "cute_gfx.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// TODO - Customizeability of the shader.

/**
 * Represents a single image rendered as a quad.
 */
typedef struct cf_batch_sprite_t
{
	/**
	 * Unique identifier for this quad's image, as determined by you.
	 */
	uint64_t id;

	cf_transform_t transform; // Position and location rotation of the quad. Default: cf_make_transform() 
	int w; // Width in pixels of the source image.
	int h; // Height in pixels of the source image.
	float scale_x; // Scaling along the quad's local x-axis in pixels.
	float scale_y; // Scaling along the quad's local y-axis in pixels.
	float alpha; // Applies additional alpha to this quad. Default: 1.0f

	int sort_bits; /*= 0;*/
} cf_batch_sprite_t;

/**
* Defaults
*	uint64_t id;
*	transform_t transform = cf_make_transform();
*	int w;
*	int h;
*	float scale_x;
*	float scale_y;
*	float alpha = 1.0f;
*	int sort_bits = 0;
*/
CUTE_INLINE cf_batch_sprite_t cf_batch_sprite_defaults()
{
	cf_batch_sprite_t result = { 0 };
	result.transform = cf_make_transform();
	result.alpha = 1.0f;
	return result;
}

/**
 * The batch is used to buffer up many different drawable things and organize them into draw calls suitable for high-
 * performance rendering on the GPU. However, this batch is not your typical batcher. This one will build texture
 * atlases internally on the the fly, and periodically needs to fetch pixels to build atlases.
 *
 * You don't have to worry about texture atlases at all and can build and ship your game with separate
 * images on disk.
 *
 * If you'd like to read more about the implementation of the batcher and why this is a good idea, go ahead and read
 * the documentation in `cute_spritebatch.h` in the `cute` folder.
 */
typedef struct cf_batch_t cf_batch_t;

/**
 * `get_pixels_fn` will be called periodically from within `batch_flush` whenever access to pixels in RAM are
 * needed to construct internal texture atlases to be sent to the GPU.
 *
 * `image_id`      - Uniquely maps to a single image, as determined by you.
 * `buffer`        - Pointer to the memory where you need to fill in pixel data.
 * `bytes_to_fill` - Number of bytes to write to `buffer`.
 * `udata`         - The `udata` pointer that was originally passed to `cf_batch_make` as `cf_get_pixels_udata`.
 */
typedef void (cf_get_pixels_fn)(uint64_t image_id, void* buffer, int bytes_to_fill, void* udata);

CUTE_API cf_batch_t* CUTE_CALL cf_batch_make(cf_get_pixels_fn* get_pixels, void* get_pixels_udata, void* mem_ctx /*= NULL*/);
CUTE_API void CUTE_CALL cf_batch_destroy(cf_batch_t* b);

/**
 * Attempts to resize internal graphics buffers. They are double buffered, so the
 * memory used on the GPU will be at least 2x `size_of_one_buffer`.
 */
CUTE_API cf_error_t CUTE_CALL cf_batch_set_GPU_buffer_configuration(cf_batch_t* b, size_t size_of_one_buffer);

/**
 * Pushes sprite quad onto an internal buffer. Does no other logic.
 *
 * To get your quad rendered, see `batch_flush`.
 * Don't forget to call `cf_batch_update` at the beginning of each game loop.
 */
CUTE_API void CUTE_CALL cf_batch_push(cf_batch_t* b, cf_batch_sprite_t sprite);

/**
 * All quads currently pushed onto the batch (see `cf_batch_push`) will be converted to an internal draw call.
 */
CUTE_API cf_error_t CUTE_CALL cf_batch_flush(cf_batch_t* b);

/**
 * Call this once at the beginning of each game loop.
 */
CUTE_API void CUTE_CALL cf_batch_update(cf_batch_t* b);

CUTE_API void CUTE_CALL cf_batch_set_texture_wrap_mode(cf_batch_t* b, sg_wrap wrap_mode);
CUTE_API void CUTE_CALL cf_batch_set_texture_filter(cf_batch_t* b, sg_filter filter);
CUTE_API void CUTE_CALL cf_batch_set_projection(cf_batch_t* b, cf_matrix_t projection);
CUTE_API void CUTE_CALL cf_batch_outlines(cf_batch_t* b, bool use_outlines);
CUTE_API void CUTE_CALL cf_batch_outlines_use_corners(cf_batch_t* b, bool use_corners);
CUTE_API void CUTE_CALL cf_batch_outlines_color(cf_batch_t* b, cf_color_t c);

CUTE_API void CUTE_CALL cf_batch_push_m3x2(cf_batch_t* b, cf_m3x2 m);
CUTE_API void CUTE_CALL cf_batch_pop_m3x2(cf_batch_t* b);
CUTE_API void CUTE_CALL cf_batch_push_scissor_box(cf_batch_t* b, int x, int y, int w, int h);
CUTE_API void CUTE_CALL cf_batch_pop_scissor_box(cf_batch_t* b);
CUTE_API void CUTE_CALL cf_batch_push_depth_state(cf_batch_t* b, const sg_depth_state* depth_state);
CUTE_API void CUTE_CALL cf_batch_push_depth_defaults(cf_batch_t* b);
CUTE_API void CUTE_CALL cf_batch_pop_depth_state(cf_batch_t* b);
CUTE_API void CUTE_CALL cf_batch_push_stencil_state(cf_batch_t* b, const sg_stencil_state* depth_stencil_state);
CUTE_API void CUTE_CALL cf_batch_push_stencil_defaults(cf_batch_t* b);
CUTE_API void CUTE_CALL cf_batch_pop_stencil_state(cf_batch_t* b);
CUTE_API void CUTE_CALL cf_batch_push_blend_state(cf_batch_t* b, const sg_blend_state* blend_state);
CUTE_API void CUTE_CALL cf_batch_push_blend_defaults(cf_batch_t* b);
CUTE_API void CUTE_CALL cf_batch_pop_blend_state(cf_batch_t* b);
CUTE_API void CUTE_CALL cf_batch_push_tint(cf_batch_t* b, cf_color_t c);
CUTE_API void CUTE_CALL cf_batch_pop_tint(cf_batch_t* b);

CUTE_API void CUTE_CALL cf_batch_quad_aabb(cf_batch_t* b, cf_aabb_t bb, cf_color_t c);
CUTE_API void CUTE_CALL cf_batch_quad_verts(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, cf_color_t c);
CUTE_API void CUTE_CALL cf_batch_quad_verts2(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, cf_color_t c0, cf_color_t c1, cf_color_t c2, cf_color_t c3);
CUTE_API void CUTE_CALL cf_batch_quad_line(cf_batch_t* b, cf_aabb_t bb, float thickness, cf_color_t c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_quad_line2(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, float thickness, cf_color_t c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_quad_line3(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, float thickness, cf_color_t c0, cf_color_t c1, cf_color_t c2, cf_color_t c3);

CUTE_API void CUTE_CALL cf_batch_circle(cf_batch_t* b, cf_v2 p, float r, int iters, cf_color_t c);
CUTE_API void CUTE_CALL cf_batch_circle_line(cf_batch_t* b, cf_v2 p, float r, int iters, float thickness, cf_color_t c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_circle_arc(cf_batch_t* b, cf_v2 p, cf_v2 center_of_arc, float range, int iters, cf_color_t color);
CUTE_API void CUTE_CALL cf_batch_circle_arc_line(cf_batch_t* b, cf_v2 p, cf_v2 center_of_arc, float range, int iters, float thickness, cf_color_t color, bool antialias /*= false*/);

CUTE_API void CUTE_CALL cf_batch_capsule(cf_batch_t* b, cf_v2 p0, cf_v2 p1, float r, int iters, cf_color_t c);
CUTE_API void CUTE_CALL cf_batch_capsule_line(cf_batch_t* b, cf_v2 p0, cf_v2 p1, float r, int iters, float thickness, cf_color_t c, bool antialias /*= false*/);

CUTE_API void CUTE_CALL cf_batch_tri(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_color_t c);
CUTE_API void CUTE_CALL cf_batch_tri2(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_color_t c0, cf_color_t c1, cf_color_t c2);
CUTE_API void CUTE_CALL cf_batch_tri_line(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, float thickness, cf_color_t c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_tri_line2(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, float thickness, cf_color_t c0, cf_color_t c1, cf_color_t c2, bool antialias /*= false*/);

CUTE_API void CUTE_CALL cf_batch_line(cf_batch_t* b, cf_v2 p0, cf_v2 p1, float thickness, cf_color_t c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_line2(cf_batch_t* b, cf_v2 p0, cf_v2 p1, float thickness, cf_color_t c0, cf_color_t c1, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_polyline(cf_batch_t* b, cf_v2* points, int count, float thickness, cf_color_t c, bool loop /*= false*/, bool antialias /*= false*/, int bevel_count /*= 0*/);


/**
 * Temporal texture information for a sprite. Is valid until the next call to `batch_flush`
 * is issued. Useful to render a sprite in an external system, e.g. Dear ImGui.
 */
typedef struct cf_temporary_image_t
{
	cf_texture_t texture_id; // A handle representing the texture for this image.
	int w; // Width in pixels of the image.
	int h; // Height in pixels of the image.
	cf_v2 u; // u coordinate of the image in the texture.
	cf_v2 v; // v coordinate of the image in the texture.
} cf_temporary_image_t;

CUTE_API cf_temporary_image_t CUTE_CALL cf_batch_fetch(cf_batch_t* b, cf_batch_sprite_t sprite);

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef CUTE_CPP

namespace cute
{

using batch_t = cf_batch_t;
using get_pixels_fn = cf_get_pixels_fn;
using temporary_image_t = cf_temporary_image_t;
using color_t = cf_color_t;
using m3x2 = cf_m3x2;
using aabb_t = cf_aabb_t;
using transform_t = cf_transform_t;

struct batch_sprite_t : public cf_batch_sprite_t
{
	batch_sprite_t()
	{
		transform = cf_make_transform();
		alpha = 1.0f;
		sort_bits = 0;
	}

	batch_sprite_t(cf_batch_sprite_t bs)
	{
		*this = bs;
	}
};


CUTE_INLINE batch_t* batch_make(get_pixels_fn* get_pixels, void* get_pixels_udata, void* mem_ctx = NULL) { cf_batch_make(get_pixels, get_pixels_udata, mem_ctx); }

CUTE_INLINE void batch_destroy(batch_t* b) { cf_batch_destroy(b); }
CUTE_INLINE void batch_push(batch_t* b, batch_sprite_t sprite) { cf_batch_push(b, sprite); }
CUTE_INLINE error_t batch_flush(batch_t* b) { return cf_batch_flush(b); }
CUTE_INLINE void batch_update(batch_t* b) { cf_batch_update(b); }

CUTE_INLINE void batch_set_texture_wrap_mode(batch_t* b, sg_wrap wrap_mode) { cf_batch_set_texture_wrap_mode(b, wrap_mode); }
CUTE_INLINE void batch_set_texture_filter(batch_t* b, sg_filter filter) { cf_batch_set_texture_filter(b, filter); }
CUTE_INLINE void batch_set_projection(batch_t* b, matrix_t projection) { cf_batch_set_projection(b, projection); }
CUTE_INLINE void batch_outlines(batch_t* b, bool use_outlines) { cf_batch_outlines(b, use_outlines); }
CUTE_INLINE void batch_outlines_use_corners(batch_t* b, bool use_corners) { cf_batch_outlines_use_corners(b, use_corners); }
CUTE_INLINE void batch_outlines_color(batch_t* b, color_t c) { cf_batch_outlines_color(b, c); }

CUTE_INLINE void batch_push_m3x2(batch_t* b, m3x2 m) { cf_batch_push_m3x2(b, m); }
CUTE_INLINE void batch_pop_m3x2(batch_t* b) { cf_batch_pop_m3x2(b); }
CUTE_INLINE void batch_push_scissor_box(batch_t* b, int x, int y, int w, int h) { cf_batch_push_scissor_box(b, x, y, w, h); }
CUTE_INLINE void batch_pop_scissor_box(batch_t* b) { cf_batch_pop_scissor_box(b); }
CUTE_INLINE void batch_push_depth_state(batch_t* b, const sg_depth_state& depth_state) { cf_batch_push_depth_state(b, &depth_state); }
CUTE_INLINE void batch_push_depth_defaults(batch_t* b) { cf_batch_push_depth_defaults(b); }
CUTE_INLINE void batch_pop_depth_state(batch_t* b) { cf_batch_pop_depth_state(b); }
CUTE_INLINE void batch_push_stencil_state(batch_t* b, const sg_stencil_state& depth_stencil_state) { cf_batch_push_stencil_state(b, &depth_stencil_state); }
CUTE_INLINE void batch_push_stencil_defaults(batch_t* b) { cf_batch_push_stencil_defaults(b); }
CUTE_INLINE void batch_pop_stencil_state(batch_t* b) { cf_batch_pop_stencil_state(b); }
CUTE_INLINE void batch_push_blend_state(batch_t* b, const sg_blend_state& blend_state) { cf_batch_push_blend_state(b, &blend_state); }
CUTE_INLINE void batch_push_blend_defaults(batch_t* b) { cf_batch_push_blend_defaults(b); }
CUTE_INLINE void batch_pop_blend_state(batch_t* b) { cf_batch_pop_blend_state(b); }
CUTE_INLINE void batch_push_tint(batch_t* b, color_t c) { cf_batch_push_tint(b, c); }
CUTE_INLINE void batch_pop_tint(batch_t* b) { cf_batch_pop_tint(b); }

CUTE_INLINE void batch_quad(batch_t* b, aabb_t bb, color_t c) { cf_batch_quad_aabb(b, bb, c); }
CUTE_INLINE void batch_quad(batch_t* b, v2 p0, v2 p1, v2 p2, v2 p3, color_t c) { cf_batch_quad_verts(b, p0, p1, p2, p3, c); }
CUTE_INLINE void batch_quad(batch_t* b, v2 p0, v2 p1, v2 p2, v2 p3, color_t c0, color_t c1, color_t c2, color_t c3) { cf_batch_quad_verts2(b, p0, p1, p2, p3, c0, c1, c2, c3); }
CUTE_INLINE void batch_quad_line(batch_t* b, aabb_t bb, float thickness, color_t c, bool antialias = false) { cf_batch_quad_line(b, bb, thickness, c, antialias); }
CUTE_INLINE void batch_quad_line(batch_t* b, v2 p0, v2 p1, v2 p2, v2 p3, float thickness, color_t c, bool antialias = false) { cf_batch_quad_line2(b, p0, p1, p2, p3, thickness, c, antialias); }
CUTE_INLINE void batch_quad_line(batch_t* b, v2 p0, v2 p1, v2 p2, v2 p3, float thickness, color_t c0, color_t c1, color_t c2, color_t c3) { cf_batch_quad_line3(b, p0, p1, p2, p3, thickness, c0, c1, c2, c3); }

CUTE_INLINE void batch_circle(batch_t* b, v2 p, float r, int iters, color_t c) { cf_batch_circle(b, p, r, iters, c); }
CUTE_INLINE void batch_circle_line(batch_t* b, v2 p, float r, int iters, float thickness, color_t c, bool antialias = false) { cf_batch_circle_line(b, p, r, iters, thickness, c, antialias); }
CUTE_INLINE void batch_circle_arc(batch_t* b, v2 p, v2 center_of_arc, float range, int iters, color_t color) { cf_batch_circle_arc(b, p, center_of_arc, range, iters, color); }
CUTE_INLINE void batch_circle_arc_line(batch_t* b, v2 p, v2 center_of_arc, float range, int iters, float thickness, color_t color, bool antialias = false) { cf_batch_circle_arc_line(b, p, center_of_arc, range, iters, thickness, color, antialias); }

CUTE_INLINE void batch_capsule(batch_t* b, v2 p0, v2 p1, float r, int iters, color_t c) { cf_batch_capsule(b, p0, p1, r, iters, c); }
CUTE_INLINE void batch_capsule_line(batch_t* b, v2 p0, v2 p1, float r, int iters, float thickness, color_t c, bool antialias = false) { cf_batch_capsule_line(b, p0, p1, r, iters, thickness, c, antialias); }

CUTE_INLINE void batch_tri(batch_t* b, v2 p0, v2 p1, v2 p2, color_t c) { cf_batch_tri(b, p0, p1, p2, c); }
CUTE_INLINE void batch_tri(batch_t* b, v2 p0, v2 p1, v2 p2, color_t c0, color_t c1, color_t c2) { cf_batch_tri2(b, p0, p1, p2, c0, c1, c2); }
CUTE_INLINE void batch_tri_line(batch_t* b, v2 p0, v2 p1, v2 p2, float thickness, color_t c, bool antialias = false) { cf_batch_tri_line(b, p0, p1, p2, thickness, c, antialias); }
CUTE_INLINE void batch_tri_line(batch_t* b, v2 p0, v2 p1, v2 p2, float thickness, color_t c0, color_t c1, color_t c2, bool antialias = false) { cf_batch_tri_line2(b, p0, p1, p2, thickness, c0, c1, c2, antialias); }

CUTE_INLINE void batch_line(batch_t* b, v2 p0, v2 p1, float thickness, color_t c, bool antialias = false) { cf_batch_line(b, p0, p1, thickness, c, antialias); }
CUTE_INLINE void batch_line(batch_t* b, v2 p0, v2 p1, float thickness, color_t c0, color_t c1, bool antialias = false) { cf_batch_line2(b, p0, p1, thickness, c0, c1, antialias); }

CUTE_INLINE void batch_polyline(batch_t* b, v2* points, int count, float thickness, color_t c, bool loop = false, bool antialias = false, int bevel_count = 0) { cf_batch_polyline(b, (cf_v2*)points, count, thickness, c, loop, antialias, bevel_count); }

CUTE_INLINE temporary_image_t batch_fetch(batch_t* b, batch_sprite_t sprite) { cf_batch_fetch(b, sprite); }
}

#endif // CUTE_CPP

#endif // CUTE_BATCH_H
