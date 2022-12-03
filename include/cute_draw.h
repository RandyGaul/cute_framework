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

#ifndef CUTE_DRAW_H
#define CUTE_DRAW_H

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

CUTE_API void CUTE_CALL cf_draw_sprite(const CF_Sprite* sprite);
CUTE_API void CUTE_CALL cf_draw_sprite_tf(const CF_Sprite* sprite, CF_Transform transform);
CUTE_API void CUTE_CALL cf_draw_quad(CF_Aabb bb, float thickness, CF_Color c, bool antialias);
CUTE_API void CUTE_CALL cf_draw_quad2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c, bool antialias);
CUTE_API void CUTE_CALL cf_draw_quad3(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3);
CUTE_API void CUTE_CALL cf_draw_quad_fill(CF_Aabb bb, CF_Color c);
CUTE_API void CUTE_CALL cf_draw_quad_fill2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c);
CUTE_API void CUTE_CALL cf_draw_quad_fill3(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3);
CUTE_API void CUTE_CALL cf_draw_circle(CF_V2 p, float r, int iters, float thickness, CF_Color c, bool antialias);
CUTE_API void CUTE_CALL cf_draw_circle_fill(CF_V2 p, float r, int iters, CF_Color c);
CUTE_API void CUTE_CALL cf_draw_circle_arc(CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness, CF_Color color, bool antialias);
CUTE_API void CUTE_CALL cf_draw_circle_arc_fill(CF_V2 p, CF_V2 center_of_arc, float range, int iters, CF_Color color);
CUTE_API void CUTE_CALL cf_draw_capsule(CF_V2 p0, CF_V2 p1, float r, int iters, float thickness, CF_Color c, bool antialias);
CUTE_API void CUTE_CALL cf_draw_capsule_fill(CF_V2 p0, CF_V2 p1, float r, int iters, CF_Color c);
CUTE_API void CUTE_CALL cf_draw_tri(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, CF_Color c, bool antialias);
CUTE_API void CUTE_CALL cf_draw_tri2(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, bool antialias);
CUTE_API void CUTE_CALL cf_draw_tri_fill(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_Color c);
CUTE_API void CUTE_CALL cf_draw_tri_fill2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_Color c0, CF_Color c1, CF_Color c2);
CUTE_API void CUTE_CALL cf_draw_line(CF_V2 p0, CF_V2 p1, float thickness, CF_Color c, bool antialias);
CUTE_API void CUTE_CALL cf_draw_line2(CF_V2 p0, CF_V2 p1, float thickness, CF_Color c0, CF_Color c1, bool antialias);
CUTE_API void CUTE_CALL cf_draw_polyline(CF_V2* points, int count, float thickness, CF_Color c, bool loop, bool antialias, int bevel_count);

CUTE_API void CUTE_CALL cf_draw_push_m3x2(CF_M3x2 m);
CUTE_API CF_M3x2 CUTE_CALL cf_draw_pop_m3x2();
CUTE_API CF_M3x2 CUTE_CALL cf_draw_peek_m3x2();
CUTE_API void CUTE_CALL cf_draw_push_tint(CF_Color c);
CUTE_API CF_Color CUTE_CALL cf_draw_pop_tint();
CUTE_API CF_Color CUTE_CALL cf_draw_peek_tint();
CUTE_API void CUTE_CALL cf_draw_push_layer(int layer);
CUTE_API int CUTE_CALL cf_draw_pop_layer();
CUTE_API int CUTE_CALL cf_draw_peek_layer();

CUTE_API void CUTE_CALL cf_draw_settings_projection(CF_Matrix4x4 projection);
CUTE_API void CUTE_CALL cf_draw_settings_texture_wrap_mode(CF_WrapMode wrap_mode);
CUTE_API void CUTE_CALL cf_draw_settings_texture_filter(CF_Filter filter);
CUTE_API void CUTE_CALL cf_draw_settings_outlines(bool use_outlines);
CUTE_API void CUTE_CALL cf_draw_settings_outlines_use_corners(bool use_corners);
CUTE_API void CUTE_CALL cf_draw_settings_outlines_color(CF_Color c);
CUTE_API void CUTE_CALL cf_draw_settings_push_scissor(CF_Rect rect);
CUTE_API CF_Rect CUTE_CALL cf_draw_settings_pop_scissor();
CUTE_API CF_Rect CUTE_CALL cf_draw_settings_peek_scissor();
CUTE_API void CUTE_CALL cf_draw_settings_push_render_state(CF_RenderState render_state);
CUTE_API CF_RenderState CUTE_CALL cf_draw_settings_pop_render_state();
CUTE_API CF_RenderState CUTE_CALL cf_draw_settings_peek_render_state();

CUTE_API void CUTE_CALL cf_render_to(CF_Canvas canvas);

/**
 * Temporal texture information for a sprite. Is valid until the next call to `draw_flush`
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

CUTE_API CF_TemporaryImage CUTE_CALL cf_fetch_image(const CF_Sprite* sprite);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using TemporaryImage = CF_TemporaryImage;

CUTE_INLINE void draw_sprite(const CF_Sprite* sprite) { cf_draw_sprite(sprite); }
CUTE_INLINE void draw_sprite_tf(const CF_Sprite* sprite, CF_Transform transform) { cf_draw_sprite_tf(sprite, transform); }
CUTE_INLINE void draw_quad(CF_Aabb bb, float thickness, CF_Color c, bool antialias) { cf_draw_quad(bb, thickness, c, antialias); }
CUTE_INLINE void draw_quad2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c, bool antialias) { cf_draw_quad2(p0, p1, p2, p3, thickness, c, antialias); }
CUTE_INLINE void draw_quad3(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3) { cf_draw_quad3(p0, p1, p2, p3, thickness, c0, c1, c2, c3); }
CUTE_INLINE void draw_quad_fill(CF_Aabb bb, CF_Color c) { cf_draw_quad_fill(bb, c); }
CUTE_INLINE void draw_quad_fill2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c) { cf_draw_quad_fill2(p0, p1, p2, p3, c); }
CUTE_INLINE void draw_quad_fill3(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3) { cf_draw_quad_fill3(p0, p1, p2, p3, c0, c1, c2, c3); }
CUTE_INLINE void draw_circle(CF_V2 p, float r, int iters, float thickness, CF_Color c, bool antialias) { cf_draw_circle(p, r, iters, thickness, c, antialias); }
CUTE_INLINE void draw_circle_fill(CF_V2 p, float r, int iters, CF_Color c) { cf_draw_circle_fill(p, r, iters, c); }
CUTE_INLINE void draw_circle_arc(CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness, CF_Color color, bool antialias) { cf_draw_circle_arc(p, center_of_arc, range, iters, thickness, color, antialias); }
CUTE_INLINE void draw_circle_arc_fill(CF_V2 p, CF_V2 center_of_arc, float range, int iters, CF_Color color) { draw_circle_arc_fill(p, center_of_arc, range, iters, color); }
CUTE_INLINE void draw_capsule(CF_V2 p0, CF_V2 p1, float r, int iters, float thickness, CF_Color c, bool antialias) { cf_draw_capsule(p0, p1, r, iters, thickness, c, antialias); }
CUTE_INLINE void draw_capsule_fill(CF_V2 p0, CF_V2 p1, float r, int iters, CF_Color c) { cf_draw_capsule_fill(p0, p1, r, iters, c); }
CUTE_INLINE void draw_tri(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, CF_Color c, bool antialias) { cf_draw_tri(p0, p1, p2, thickness, c, antialias); }
CUTE_INLINE void draw_tri2(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, bool antialias) { cf_draw_tri2(p0, p1, p2, thickness, c0, c1, c2, antialias); }
CUTE_INLINE void draw_tri_fill(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_Color c) { cf_draw_tri_fill(p0, p1, p2, c); }
CUTE_INLINE void draw_tri_fill2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_Color c0, CF_Color c1, CF_Color c2) { cf_draw_tri_fill2(p0, p1, p2, c0, c1, c2); }
CUTE_INLINE void draw_line(CF_V2 p0, CF_V2 p1, float thickness, CF_Color c, bool antialias) { cf_draw_line(p0, p1, thickness, c, antialias); }
CUTE_INLINE void draw_line2(CF_V2 p0, CF_V2 p1, float thickness, CF_Color c0, CF_Color c1, bool antialias) { cf_draw_line2(p0, p1, thickness, c0, c1, antialias); }
CUTE_INLINE void draw_polyline(CF_V2* points, int count, float thickness, CF_Color c, bool loop, bool antialias, int bevel_count) { cf_draw_polyline(points, count, thickness, c, loop, antialias, bevel_count); }

CUTE_INLINE void draw_push_m3x2(CF_M3x2 m) { cf_draw_push_m3x2(m); }
CUTE_INLINE CF_M3x2 draw_pop_m3x2() { return cf_draw_pop_m3x2(); }
CUTE_INLINE CF_M3x2 draw_peek_m3x2() { return cf_draw_peek_m3x2(); }
CUTE_INLINE void draw_push_tint(CF_Color c) { cf_draw_push_tint(c); }
CUTE_INLINE CF_Color draw_pop_tint() { return cf_draw_pop_tint(); }
CUTE_INLINE CF_Color draw_peek_tint() { return cf_draw_peek_tint(); }
CUTE_INLINE void draw_push_layer(int layer) { cf_draw_push_layer(layer); }
CUTE_INLINE int draw_pop_layer() { return cf_draw_pop_layer(); }
CUTE_INLINE int draw_peek_layer() { return cf_draw_peek_layer(); }

CUTE_INLINE void draw_settings_projection(CF_Matrix4x4 projection) { cf_draw_settings_projection(projection); }
CUTE_INLINE void draw_settings_texture_wrap_mode(CF_WrapMode wrap_mode) { cf_draw_settings_texture_wrap_mode(wrap_mode); }
CUTE_INLINE void draw_settings_texture_filter(CF_Filter filter) { cf_draw_settings_texture_filter(filter); }
CUTE_INLINE void draw_settings_outlines(bool use_outlines) { cf_draw_settings_outlines(use_outlines); }
CUTE_INLINE void draw_settings_outlines_use_corners(bool use_corners) { cf_draw_settings_outlines_use_corners(use_corners); }
CUTE_INLINE void draw_settings_outlines_color(CF_Color c) { cf_draw_settings_outlines_color(c); }
CUTE_INLINE void draw_settings_push_scissor(CF_Rect rect) { cf_draw_settings_push_scissor(rect); }
CUTE_INLINE CF_Rect draw_settings_pop_scissor() { return cf_draw_settings_pop_scissor(); }
CUTE_INLINE CF_Rect draw_settings_peek_scissor() { return cf_draw_settings_peek_scissor(); }
CUTE_INLINE void draw_settings_push_render_state(CF_RenderState render_state) { draw_settings_push_render_state(render_state); }
CUTE_INLINE CF_RenderState draw_settings_pop_render_state() { return draw_settings_pop_render_state(); }
CUTE_INLINE CF_RenderState draw_settings_peek_render_state() { return draw_settings_peek_render_state(); }

CUTE_INLINE void render_to(CF_Canvas canvas) { cf_render_to(canvas); }

CUTE_INLINE CF_TemporaryImage fetch_image(const CF_Sprite* sprite) { return cf_fetch_image(sprite); }

}

#endif // CUTE_CPP

#endif // CUTE_DRAW_H
