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
#include "cute_font.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CUTE_API void CUTE_CALL cf_draw_sprite(const CF_Sprite* sprite);
CUTE_API void CUTE_CALL cf_draw_quad(CF_Aabb bb, float thickness);
CUTE_API void CUTE_CALL cf_draw_quad2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness);
CUTE_API void CUTE_CALL cf_draw_quad3(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3);
CUTE_API void CUTE_CALL cf_draw_quad_fill(CF_Aabb bb);
CUTE_API void CUTE_CALL cf_draw_quad_fill2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3);
CUTE_API void CUTE_CALL cf_draw_quad_fill3(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3);
CUTE_API void CUTE_CALL cf_draw_circle(CF_V2 p, float r, int iters, float thickness);
CUTE_API void CUTE_CALL cf_draw_circle_fill(CF_V2 p, float r, int iters);
CUTE_API void CUTE_CALL cf_draw_circle_arc(CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness);
CUTE_API void CUTE_CALL cf_draw_circle_arc_fill(CF_V2 p, CF_V2 center_of_arc, float range, int iters);
CUTE_API void CUTE_CALL cf_draw_capsule(CF_V2 p0, CF_V2 p1, float r, int iters, float thickness);
CUTE_API void CUTE_CALL cf_draw_capsule_fill(CF_V2 p0, CF_V2 p1, float r, int iters);
CUTE_API void CUTE_CALL cf_draw_tri(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness);
CUTE_API void CUTE_CALL cf_draw_tri_fill(CF_V2 p0, CF_V2 p1, CF_V2 p2);
CUTE_API void CUTE_CALL cf_draw_tri_fill2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_Color c0, CF_Color c1, CF_Color c2);
CUTE_API void CUTE_CALL cf_draw_line(CF_V2 p0, CF_V2 p1, float thickness);
CUTE_API void CUTE_CALL cf_draw_line2(CF_V2 p0, CF_V2 p1, float thickness, CF_Color c0, CF_Color c1);
CUTE_API void CUTE_CALL cf_draw_polyline(CF_V2* points, int count, float thickness, bool loop, int bevel_count);
CUTE_API void CUTE_CALL cf_draw_bezier_line(CF_V2 a, CF_V2 c0, CF_V2 b, int iters, float thickness);
CUTE_API void CUTE_CALL cf_draw_bezier_line2(CF_V2 a, CF_V2 c0, CF_V2 c1, CF_V2 b, int iters, float thickness);

CUTE_API void CUTE_CALL cf_draw_push_font(const char* font);
CUTE_API const char* CUTE_CALL cf_draw_pop_font();
CUTE_API const char* CUTE_CALL cf_draw_peek_font();
CUTE_API void CUTE_CALL cf_draw_push_font_size(float size);
CUTE_API float CUTE_CALL cf_draw_pop_font_size();
CUTE_API float CUTE_CALL cf_draw_peek_font_size();
CUTE_API void CUTE_CALL cf_draw_push_font_blur(int blur);
CUTE_API int CUTE_CALL cf_draw_pop_font_blur();
CUTE_API int CUTE_CALL cf_draw_peek_font_blur();
CUTE_API void CUTE_CALL cf_draw_text(const char* text, CF_V2 position);

CUTE_API void CUTE_CALL cf_draw_push_layer(int layer);
CUTE_API int CUTE_CALL cf_draw_pop_layer();
CUTE_API int CUTE_CALL cf_draw_peek_layer();
CUTE_API void CUTE_CALL cf_draw_push_color(CF_Color c);
CUTE_API CF_Color CUTE_CALL cf_draw_pop_color();
CUTE_API CF_Color CUTE_CALL cf_draw_peek_color();
CUTE_API void CUTE_CALL cf_draw_push_tint(CF_Color c);
CUTE_API CF_Color CUTE_CALL cf_draw_pop_tint();
CUTE_API CF_Color CUTE_CALL cf_draw_peek_tint();
CUTE_API void CUTE_CALL cf_draw_push_antialias(bool antialias);
CUTE_API bool CUTE_CALL cf_draw_pop_antialias();
CUTE_API bool CUTE_CALL cf_draw_peek_antialias();

CUTE_API void CUTE_CALL cf_render_settings_filter(CF_Filter filter);
CUTE_API void CUTE_CALL cf_render_settings_push_viewport(CF_Rect viewport);
CUTE_API CF_Rect CUTE_CALL cf_render_settings_pop_viewport();
CUTE_API CF_Rect CUTE_CALL cf_render_settings_peek_viewport();
CUTE_API void CUTE_CALL cf_render_settings_push_scissor(CF_Rect scissor);
CUTE_API CF_Rect CUTE_CALL cf_render_settings_pop_scissor();
CUTE_API CF_Rect CUTE_CALL cf_render_settings_peek_scissor();
CUTE_API void CUTE_CALL cf_render_settings_push_render_state(CF_RenderState render_state);
CUTE_API CF_RenderState CUTE_CALL cf_render_settings_pop_render_state();
CUTE_API CF_RenderState CUTE_CALL cf_render_settings_peek_render_state();

CUTE_API void CUTE_CALL cf_camera_dimensions(float w, float h);
CUTE_API void CUTE_CALL cf_camera_look_at(float x, float y);
CUTE_API void CUTE_CALL cf_camera_rotate(float radians);
CUTE_API void CUTE_CALL cf_camera_push();
CUTE_API void CUTE_CALL cf_camera_pop();

CUTE_API void CUTE_CALL cf_render_to(CF_Canvas canvas, bool clear);

/**
 * Temporal texture information for a sprite. Useful to render a sprite in an external system,
 * e.g. Dear ImGui. This struct is only valid until the next time `cf_render_to` or
 * `cf_app_present` is called.
 */
typedef struct CF_TemporaryImage
{
	CF_Texture tex; // A handle representing the texture for this image.
	int w;          // Width in pixels of the image.
	int h;          // Height in pixels of the image.
	CF_V2 u;        // u coordinate of the image in the texture.
	CF_V2 v;        // v coordinate of the image in the texture.
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
CUTE_INLINE void draw_quad(CF_Aabb bb, float thickness) { cf_draw_quad(bb, thickness); }
CUTE_INLINE void draw_quad(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness) { cf_draw_quad2(p0, p1, p2, p3, thickness); }
CUTE_INLINE void draw_quad(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3) { cf_draw_quad3(p0, p1, p2, p3, thickness, c0, c1, c2, c3); }
CUTE_INLINE void draw_quad_fill(CF_Aabb bb) { cf_draw_quad_fill(bb); }
CUTE_INLINE void draw_quad_fill(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3) { cf_draw_quad_fill2(p0, p1, p2, p3); }
CUTE_INLINE void draw_quad_fill(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3) { cf_draw_quad_fill3(p0, p1, p2, p3, c0, c1, c2, c3); }
CUTE_INLINE void draw_circle(CF_V2 p, float r, int iters, float thickness) { cf_draw_circle(p, r, iters, thickness); }
CUTE_INLINE void draw_circle_fill(CF_V2 p, float r, int iters) { cf_draw_circle_fill(p, r, iters); }
CUTE_INLINE void draw_circle_arc(CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness) { cf_draw_circle_arc(p, center_of_arc, range, iters, thickness); }
CUTE_INLINE void draw_circle_arc_fill(CF_V2 p, CF_V2 center_of_arc, float range, int iters) { draw_circle_arc_fill(p, center_of_arc, range, iters); }
CUTE_INLINE void draw_capsule(CF_V2 p0, CF_V2 p1, float r, int iters, float thickness) { cf_draw_capsule(p0, p1, r, iters, thickness); }
CUTE_INLINE void draw_capsule_fill(CF_V2 p0, CF_V2 p1, float r, int iters) { cf_draw_capsule_fill(p0, p1, r, iters); }
CUTE_INLINE void draw_tri(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness) { cf_draw_tri(p0, p1, p2, thickness); }
CUTE_INLINE void draw_tri_fill(CF_V2 p0, CF_V2 p1, CF_V2 p2) { cf_draw_tri_fill(p0, p1, p2); }
CUTE_INLINE void draw_tri_fill(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_Color c0, CF_Color c1, CF_Color c2) { cf_draw_tri_fill2(p0, p1, p2, c0, c1, c2); }
CUTE_INLINE void draw_line(CF_V2 p0, CF_V2 p1, float thickness) { cf_draw_line(p0, p1, thickness); }
CUTE_INLINE void draw_line(CF_V2 p0, CF_V2 p1, float thickness, CF_Color c0, CF_Color c1) { cf_draw_line2(p0, p1, thickness, c0, c1); }
CUTE_INLINE void draw_polyline(CF_V2* points, int count, float thickness, bool loop, int bevel_count) { cf_draw_polyline(points, count, thickness, loop, bevel_count); }

CUTE_INLINE void draw_push_layer(int layer) { cf_draw_push_layer(layer); }
CUTE_INLINE int draw_pop_layer() { return cf_draw_pop_layer(); }
CUTE_INLINE int draw_peek_layer() { return cf_draw_peek_layer(); }
CUTE_INLINE void draw_push_color(CF_Color c) { cf_draw_push_color(c); }
CUTE_INLINE CF_Color draw_pop_color() { return cf_draw_pop_color(); }
CUTE_INLINE CF_Color draw_peek_color() { return cf_draw_peek_color(); }
CUTE_INLINE void draw_push_tint(CF_Color c) { cf_draw_push_tint(c); }
CUTE_INLINE CF_Color draw_pop_tint() { return cf_draw_pop_tint(); }
CUTE_INLINE CF_Color draw_peek_tint() { return cf_draw_peek_tint(); }
CUTE_INLINE void draw_push_antialias(bool antialias) { cf_draw_push_antialias(antialias); }
CUTE_INLINE bool draw_pop_antialias() { return cf_draw_pop_antialias(); }
CUTE_INLINE bool draw_peek_antialias() { return cf_draw_peek_antialias(); }

CUTE_INLINE void render_settings_filter(CF_Filter filter) { cf_render_settings_filter(filter); }
CUTE_INLINE void render_settings_push_viewport(CF_Rect viewport) { cf_render_settings_push_viewport(viewport); }
CUTE_INLINE CF_Rect render_settings_pop_viewport() { return cf_render_settings_pop_viewport(); }
CUTE_INLINE CF_Rect render_settings_peek_viewport() { return cf_render_settings_peek_viewport(); }
CUTE_INLINE void render_settings_push_scissor(CF_Rect scissor) { cf_render_settings_push_scissor(scissor); }
CUTE_INLINE CF_Rect render_settings_pop_scissor() { return cf_render_settings_pop_scissor(); }
CUTE_INLINE CF_Rect render_settings_peek_scissor() { return cf_render_settings_peek_scissor(); }
CUTE_INLINE void render_settings_push_render_state(CF_RenderState render_state) { render_settings_push_render_state(render_state); }
CUTE_INLINE CF_RenderState render_settings_pop_render_state() { return render_settings_pop_render_state(); }
CUTE_INLINE CF_RenderState render_settings_peek_render_state() { return render_settings_peek_render_state(); }

CUTE_INLINE void camera_dimensions(float w, float h) { cf_camera_dimensions(w, h); }
CUTE_INLINE void camera_look_at(float x, float y) { cf_camera_look_at(x, y); }
CUTE_INLINE void camera_rotate(float radians) { cf_camera_rotate(radians); }
CUTE_INLINE void camera_push() { cf_camera_push(); }
CUTE_INLINE void camera_pop() { cf_camera_pop(); }

CUTE_INLINE void render_to(CF_Canvas canvas, bool clear = false) { cf_render_to(canvas, clear); }

CUTE_INLINE CF_TemporaryImage fetch_image(const CF_Sprite* sprite) { return cf_fetch_image(sprite); }

}

#endif // CUTE_CPP

#endif // CUTE_DRAW_H
