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
#include "cute_result.h"
#include "cute_gfx.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CUTE_API cf_result_t CUTE_CALL cf_batch_set_GPU_buffer_configuration(size_t size_of_one_buffer);

CUTE_API void CUTE_CALL cf_batch_set_texture_wrap_mode(sg_wrap wrap_mode);
CUTE_API void CUTE_CALL cf_batch_set_texture_filter(sg_filter filter);
CUTE_API void CUTE_CALL cf_batch_set_projection(cf_matrix_t projection);
CUTE_API void CUTE_CALL cf_batch_outlines(bool use_outlines);
CUTE_API void CUTE_CALL cf_batch_outlines_use_corners(bool use_corners);
CUTE_API void CUTE_CALL cf_batch_outlines_color(cf_color_t c);

CUTE_API void CUTE_CALL cf_batch_push_m3x2(cf_m3x2 m);
CUTE_API void CUTE_CALL cf_batch_pop_m3x2();
CUTE_API void CUTE_CALL cf_batch_push_scissor_box(int x, int y, int w, int h);
CUTE_API void CUTE_CALL cf_batch_pop_scissor_box();
CUTE_API void CUTE_CALL cf_batch_push_depth_state(sg_depth_state depth_state);
CUTE_API void CUTE_CALL cf_batch_push_depth_defaults();
CUTE_API void CUTE_CALL cf_batch_pop_depth_state();
CUTE_API void CUTE_CALL cf_batch_push_stencil_state(sg_stencil_state depth_stencil_state);
CUTE_API void CUTE_CALL cf_batch_push_stencil_defaults();
CUTE_API void CUTE_CALL cf_batch_pop_stencil_state();
CUTE_API void CUTE_CALL cf_batch_push_blend_state(sg_blend_state blend_state);
CUTE_API void CUTE_CALL cf_batch_push_blend_defaults();
CUTE_API void CUTE_CALL cf_batch_pop_blend_state();
CUTE_API void CUTE_CALL cf_batch_push_tint(cf_color_t c);
CUTE_API void CUTE_CALL cf_batch_pop_tint();

CUTE_API void CUTE_CALL cf_batch_quad_aabb(cf_aabb_t bb, cf_color_t c);
CUTE_API void CUTE_CALL cf_batch_quad_verts(cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, cf_color_t c);
CUTE_API void CUTE_CALL cf_batch_quad_verts2(cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, cf_color_t c0, cf_color_t c1, cf_color_t c2, cf_color_t c3);
CUTE_API void CUTE_CALL cf_batch_quad_line(cf_aabb_t bb, float thickness, cf_color_t c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_quad_line2(cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, float thickness, cf_color_t c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_quad_line3(cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, float thickness, cf_color_t c0, cf_color_t c1, cf_color_t c2, cf_color_t c3);

CUTE_API void CUTE_CALL cf_batch_circle(cf_v2 p, float r, int iters, cf_color_t c);
CUTE_API void CUTE_CALL cf_batch_circle_line(cf_v2 p, float r, int iters, float thickness, cf_color_t c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_circle_arc(cf_v2 p, cf_v2 center_of_arc, float range, int iters, cf_color_t color);
CUTE_API void CUTE_CALL cf_batch_circle_arc_line(cf_v2 p, cf_v2 center_of_arc, float range, int iters, float thickness, cf_color_t color, bool antialias /*= false*/);

CUTE_API void CUTE_CALL cf_batch_capsule(cf_v2 p0, cf_v2 p1, float r, int iters, cf_color_t c);
CUTE_API void CUTE_CALL cf_batch_capsule_line(cf_v2 p0, cf_v2 p1, float r, int iters, float thickness, cf_color_t c, bool antialias /*= false*/);

CUTE_API void CUTE_CALL cf_batch_tri(cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_color_t c);
CUTE_API void CUTE_CALL cf_batch_tri2(cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_color_t c0, cf_color_t c1, cf_color_t c2);
CUTE_API void CUTE_CALL cf_batch_tri_line(cf_v2 p0, cf_v2 p1, cf_v2 p2, float thickness, cf_color_t c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_tri_line2(cf_v2 p0, cf_v2 p1, cf_v2 p2, float thickness, cf_color_t c0, cf_color_t c1, cf_color_t c2, bool antialias /*= false*/);

CUTE_API void CUTE_CALL cf_batch_line(cf_v2 p0, cf_v2 p1, float thickness, cf_color_t c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_line2(cf_v2 p0, cf_v2 p1, float thickness, cf_color_t c0, cf_color_t c1, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_polyline(cf_v2* points, int count, float thickness, cf_color_t c, bool loop /*= false*/, bool antialias /*= false*/, int bevel_count /*= 0*/);

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

CUTE_API cf_temporary_image_t CUTE_CALL cf_batch_fetch(cf_sprite_t sprite);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using temporary_image_t = cf_temporary_image_t;
using color_t = cf_color_t;
using m3x2 = cf_m3x2;
using aabb_t = cf_aabb_t;
using transform_t = cf_transform_t;

CUTE_INLINE void batch_set_texture_wrap_mode(sg_wrap wrap_mode) { cf_batch_set_texture_wrap_mode(wrap_mode); }
CUTE_INLINE void batch_set_texture_filter(sg_filter filter) { cf_batch_set_texture_filter(filter); }
CUTE_INLINE void batch_set_projection(matrix_t projection) { cf_batch_set_projection(projection); }
CUTE_INLINE void batch_outlines(bool use_outlines) { cf_batch_outlines(use_outlines); }
CUTE_INLINE void batch_outlines_use_corners(bool use_corners) { cf_batch_outlines_use_corners(use_corners); }
CUTE_INLINE void batch_outlines_color(color_t c) { cf_batch_outlines_color(c); }

CUTE_INLINE void batch_push_m3x2(m3x2 m) { cf_batch_push_m3x2(m); }
CUTE_INLINE void batch_pop_m3x2() { cf_batch_pop_m3x2(); }
CUTE_INLINE void batch_push_scissor_box(int x, int y, int w, int h) { cf_batch_push_scissor_box(x, y, w, h); }
CUTE_INLINE void batch_pop_scissor_box() { cf_batch_pop_scissor_box(); }
CUTE_INLINE void batch_push_depth_state(sg_depth_state depth_state) { cf_batch_push_depth_state(depth_state); }
CUTE_INLINE void batch_push_depth_defaults() { cf_batch_push_depth_defaults(); }
CUTE_INLINE void batch_pop_depth_state() { cf_batch_pop_depth_state(); }
CUTE_INLINE void batch_push_stencil_state(sg_stencil_state depth_stencil_state) { cf_batch_push_stencil_state(depth_stencil_state); }
CUTE_INLINE void batch_push_stencil_defaults() { cf_batch_push_stencil_defaults(); }
CUTE_INLINE void batch_pop_stencil_state() { cf_batch_pop_stencil_state(); }
CUTE_INLINE void batch_push_blend_state(sg_blend_state blend_state) { cf_batch_push_blend_state(blend_state); }
CUTE_INLINE void batch_push_blend_defaults() { cf_batch_push_blend_defaults(); }
CUTE_INLINE void batch_pop_blend_state() { cf_batch_pop_blend_state(); }
CUTE_INLINE void batch_push_tint(color_t c) { cf_batch_push_tint(c); }
CUTE_INLINE void batch_pop_tint() { cf_batch_pop_tint(); }

CUTE_INLINE void batch_quad(aabb_t bb, color_t c) { cf_batch_quad_aabb(bb, c); }
CUTE_INLINE void batch_quad(v2 p0, v2 p1, v2 p2, v2 p3, color_t c) { cf_batch_quad_verts(p0, p1, p2, p3, c); }
CUTE_INLINE void batch_quad(v2 p0, v2 p1, v2 p2, v2 p3, color_t c0, color_t c1, color_t c2, color_t c3) { cf_batch_quad_verts2(p0, p1, p2, p3, c0, c1, c2, c3); }
CUTE_INLINE void batch_quad_line(aabb_t bb, float thickness, color_t c, bool antialias = false) { cf_batch_quad_line(bb, thickness, c, antialias); }
CUTE_INLINE void batch_quad_line(v2 p0, v2 p1, v2 p2, v2 p3, float thickness, color_t c, bool antialias = false) { cf_batch_quad_line2(p0, p1, p2, p3, thickness, c, antialias); }
CUTE_INLINE void batch_quad_line(v2 p0, v2 p1, v2 p2, v2 p3, float thickness, color_t c0, color_t c1, color_t c2, color_t c3) { cf_batch_quad_line3(p0, p1, p2, p3, thickness, c0, c1, c2, c3); }

CUTE_INLINE void batch_circle(v2 p, float r, int iters, color_t c) { cf_batch_circle(p, r, iters, c); }
CUTE_INLINE void batch_circle_line(v2 p, float r, int iters, float thickness, color_t c, bool antialias = false) { cf_batch_circle_line(p, r, iters, thickness, c, antialias); }
CUTE_INLINE void batch_circle_arc(v2 p, v2 center_of_arc, float range, int iters, color_t color) { cf_batch_circle_arc(p, center_of_arc, range, iters, color); }
CUTE_INLINE void batch_circle_arc_line(v2 p, v2 center_of_arc, float range, int iters, float thickness, color_t color, bool antialias = false) { cf_batch_circle_arc_line(p, center_of_arc, range, iters, thickness, color, antialias); }

CUTE_INLINE void batch_capsule(v2 p0, v2 p1, float r, int iters, color_t c) { cf_batch_capsule(p0, p1, r, iters, c); }
CUTE_INLINE void batch_capsule_line(v2 p0, v2 p1, float r, int iters, float thickness, color_t c, bool antialias = false) { cf_batch_capsule_line(p0, p1, r, iters, thickness, c, antialias); }

CUTE_INLINE void batch_tri(v2 p0, v2 p1, v2 p2, color_t c) { cf_batch_tri(p0, p1, p2, c); }
CUTE_INLINE void batch_tri(v2 p0, v2 p1, v2 p2, color_t c0, color_t c1, color_t c2) { cf_batch_tri2(p0, p1, p2, c0, c1, c2); }
CUTE_INLINE void batch_tri_line(v2 p0, v2 p1, v2 p2, float thickness, color_t c, bool antialias = false) { cf_batch_tri_line(p0, p1, p2, thickness, c, antialias); }
CUTE_INLINE void batch_tri_line(v2 p0, v2 p1, v2 p2, float thickness, color_t c0, color_t c1, color_t c2, bool antialias = false) { cf_batch_tri_line2(p0, p1, p2, thickness, c0, c1, c2, antialias); }

CUTE_INLINE void batch_line(v2 p0, v2 p1, float thickness, color_t c, bool antialias = false) { cf_batch_line(p0, p1, thickness, c, antialias); }
CUTE_INLINE void batch_line(v2 p0, v2 p1, float thickness, color_t c0, color_t c1, bool antialias = false) { cf_batch_line2(p0, p1, thickness, c0, c1, antialias); }

CUTE_INLINE void batch_polyline(v2* points, int count, float thickness, color_t c, bool loop = false, bool antialias = false, int bevel_count = 0) { cf_batch_polyline((cf_v2*)points, count, thickness, c, loop, antialias, bevel_count); }

CUTE_INLINE temporary_image_t batch_fetch(sprite_t sprite) { return cf_batch_fetch(sprite); }

}

#endif // CUTE_CPP

#endif // CUTE_BATCH_H
