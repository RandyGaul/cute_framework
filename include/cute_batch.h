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
#include "cute_graphics.h"
#include "cute_sprite.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CUTE_API void CUTE_CALL cf_batch_render(CF_Matrix4x4 projection);
CUTE_API void CUTE_CALL cf_batch_set_texture_wrap_mode(CF_WrapMode wrap_mode);
CUTE_API void CUTE_CALL cf_batch_set_texture_filter(CF_Filter filter);
CUTE_API void CUTE_CALL cf_batch_outlines(bool use_outlines);
CUTE_API void CUTE_CALL cf_batch_outlines_use_corners(bool use_corners);
CUTE_API void CUTE_CALL cf_batch_outlines_color(CF_Color c);

CUTE_API void CUTE_CALL cf_batch_push_m3x2(cf_m3x2 m);
CUTE_API cf_m3x2 CUTE_CALL cf_batch_pop_m3x2();
CUTE_API cf_m3x2 CUTE_CALL cf_batch_peek_m3x2();
CUTE_API void CUTE_CALL cf_batch_push_scissor_box(int x, int y, int w, int h);
CUTE_API void CUTE_CALL cf_batch_pop_scissor_box();
CUTE_API void CUTE_CALL cf_batch_peek_scissor_box(int* x, int* y, int* w, int* h);
CUTE_API void CUTE_CALL cf_batch_push_render_state(CF_RenderState render_state); // Todo.
CUTE_API CF_RenderState CUTE_CALL cf_batch_pop_render_state(); // Todo.
CUTE_API CF_RenderState CUTE_CALL cf_batch_peek_render_state(); // Todo.
CUTE_API void CUTE_CALL cf_batch_push_tint(CF_Color c);
CUTE_API CF_Color CUTE_CALL cf_batch_pop_tint();
CUTE_API CF_Color CUTE_CALL cf_batch_peek_tint();
CUTE_API void CUTE_CALL cf_batch_push_layer(int layer);
CUTE_API int CUTE_CALL cf_batch_pop_layer();
CUTE_API int CUTE_CALL cf_batch_peek_layer();

CUTE_API void CUTE_CALL cf_batch_sprite(const CF_Sprite* sprite);
CUTE_API void CUTE_CALL cf_batch_sprite_tf(const CF_Sprite* sprite, CF_Transform transform);

CUTE_API void CUTE_CALL cf_batch_quad_aabb(CF_Aabb bb, CF_Color c);
CUTE_API void CUTE_CALL cf_batch_quad_verts(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c);
CUTE_API void CUTE_CALL cf_batch_quad_verts2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3);
CUTE_API void CUTE_CALL cf_batch_quad_line(CF_Aabb bb, float thickness, CF_Color c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_quad_line2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_quad_line3(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3);

CUTE_API void CUTE_CALL cf_batch_circle(CF_V2 p, float r, int iters, CF_Color c);
CUTE_API void CUTE_CALL cf_batch_circle_line(CF_V2 p, float r, int iters, float thickness, CF_Color c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_circle_arc(CF_V2 p, CF_V2 center_of_arc, float range, int iters, CF_Color color);
CUTE_API void CUTE_CALL cf_batch_circle_arc_line(CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness, CF_Color color, bool antialias /*= false*/);

CUTE_API void CUTE_CALL cf_batch_capsule(CF_V2 p0, CF_V2 p1, float r, int iters, CF_Color c);
CUTE_API void CUTE_CALL cf_batch_capsule_line(CF_V2 p0, CF_V2 p1, float r, int iters, float thickness, CF_Color c, bool antialias /*= false*/);

CUTE_API void CUTE_CALL CF_Batchri(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_Color c);
CUTE_API void CUTE_CALL CF_Batchri2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_Color c0, CF_Color c1, CF_Color c2);
CUTE_API void CUTE_CALL CF_Batchri_line(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, CF_Color c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL CF_Batchri_line2(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, bool antialias /*= false*/);

CUTE_API void CUTE_CALL cf_batch_line(CF_V2 p0, CF_V2 p1, float thickness, CF_Color c, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_line2(CF_V2 p0, CF_V2 p1, float thickness, CF_Color c0, CF_Color c1, bool antialias /*= false*/);
CUTE_API void CUTE_CALL cf_batch_polyline(CF_V2* points, int count, float thickness, CF_Color c, bool loop /*= false*/, bool antialias /*= false*/, int bevel_count /*= 0*/);

/**
 * Temporal texture information for a sprite. Is valid until the next call to `batch_flush`
 * is issued. Useful to render a sprite in an external system, e.g. Dear ImGui. This struct
 * is only valid until the next time `cf_app_present` is called.
 */
typedef struct CF_TemporaryImage
{
	CF_Texture tex; // A handle representing the texture for this image.
	int w; // Width in pixels of the image.
	int h; // Height in pixels of the image.
	CF_V2 u; // u coordinate of the image in the texture.
	CF_V2 v; // v coordinate of the image in the texture.
} CF_TemporaryImage;

CUTE_API CF_TemporaryImage CUTE_CALL cf_batch_fetch(const CF_Sprite* sprite);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using temporary_image_t = CF_TemporaryImage;
using color_t = CF_Color;
using m3x2 = cf_m3x2;
using aabb_t = CF_Aabb;
using Transform = CF_Transform;

CUTE_INLINE void batch_render(Matrix4x4 projection) { cf_batch_render(projection); }
CUTE_INLINE void batch_set_texture_wrap_mode(CF_WrapMode wrap_mode) { cf_batch_set_texture_wrap_mode(wrap_mode); }
CUTE_INLINE void batch_set_texture_filter(CF_Filter filter) { cf_batch_set_texture_filter(filter); }
CUTE_INLINE void batch_outlines(bool use_outlines) { cf_batch_outlines(use_outlines); }
CUTE_INLINE void batch_outlines_use_corners(bool use_corners) { cf_batch_outlines_use_corners(use_corners); }
CUTE_INLINE void batch_outlines_color(color_t c) { cf_batch_outlines_color(c); }

CUTE_INLINE void batch_push_m3x2(m3x2 m) { cf_batch_push_m3x2(m); }
CUTE_INLINE void batch_pop_m3x2() { cf_batch_pop_m3x2(); }
CUTE_INLINE void batch_push_scissor_box(int x, int y, int w, int h) { cf_batch_push_scissor_box(x, y, w, h); }
CUTE_INLINE void batch_pop_scissor_box() { cf_batch_pop_scissor_box(); }
CUTE_INLINE void batch_push_render_state(CF_RenderState render_state) { cf_batch_push_render_state(render_state); }
CUTE_INLINE void batch_pop_render_state() { cf_batch_pop_render_state(); }
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

CUTE_INLINE void batch_tri(v2 p0, v2 p1, v2 p2, color_t c) { CF_Batchri(p0, p1, p2, c); }
CUTE_INLINE void batch_tri(v2 p0, v2 p1, v2 p2, color_t c0, color_t c1, color_t c2) { CF_Batchri2(p0, p1, p2, c0, c1, c2); }
CUTE_INLINE void batch_tri_line(v2 p0, v2 p1, v2 p2, float thickness, color_t c, bool antialias = false) { CF_Batchri_line(p0, p1, p2, thickness, c, antialias); }
CUTE_INLINE void batch_tri_line(v2 p0, v2 p1, v2 p2, float thickness, color_t c0, color_t c1, color_t c2, bool antialias = false) { CF_Batchri_line2(p0, p1, p2, thickness, c0, c1, c2, antialias); }

CUTE_INLINE void batch_line(v2 p0, v2 p1, float thickness, color_t c, bool antialias = false) { cf_batch_line(p0, p1, thickness, c, antialias); }
CUTE_INLINE void batch_line(v2 p0, v2 p1, float thickness, color_t c0, color_t c1, bool antialias = false) { cf_batch_line2(p0, p1, thickness, c0, c1, antialias); }

CUTE_INLINE void batch_polyline(v2* points, int count, float thickness, color_t c, bool loop = false, bool antialias = false, int bevel_count = 0) { cf_batch_polyline((CF_V2*)points, count, thickness, c, loop, antialias, bevel_count); }

CUTE_INLINE temporary_image_t batch_fetch(const Sprite* sprite) { return cf_batch_fetch(sprite); }

}

#endif // CUTE_CPP

#endif // CUTE_BATCH_H
