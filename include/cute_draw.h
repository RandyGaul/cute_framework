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

CUTE_API void CUTE_CALL cf_make_font(const char* path, const char* font_name, CF_Result* result_out);
CUTE_API void CUTE_CALL cf_make_font_mem(void* data, int size, const char* font_name, CF_Result* result_out);
CUTE_API void CUTE_CALL cf_destroy_font(const char* font_name);
CUTE_API void CUTE_CALL cf_font_add_backup_codepoints(const char* font_name, int* codepoints, int count);
CUTE_API void CUTE_CALL cf_push_font(const char* font);
CUTE_API const char* CUTE_CALL cf_pop_font();
CUTE_API const char* CUTE_CALL cf_peek_font();
CUTE_API void CUTE_CALL cf_push_font_size(float size);
CUTE_API float CUTE_CALL cf_pop_font_size();
CUTE_API float CUTE_CALL cf_peek_font_size();
CUTE_API void CUTE_CALL cf_push_font_blur(int blur);
CUTE_API int CUTE_CALL cf_pop_font_blur();
CUTE_API int CUTE_CALL cf_peek_font_blur();
CUTE_API void CUTE_CALL cf_push_text_wrap_width(float width);
CUTE_API float CUTE_CALL cf_pop_text_wrap_width();
CUTE_API float CUTE_CALL cf_peek_text_wrap_width();
CUTE_API void CUTE_CALL cf_push_text_clip_box(CF_Aabb clip_box);
CUTE_API CF_Aabb CUTE_CALL cf_pop_text_clip_box();
CUTE_API CF_Aabb CUTE_CALL cf_peek_text_clip_box();
CUTE_API void CUTE_CALL cf_push_text_vertical_layout(bool layout_vertically);
CUTE_API bool CUTE_CALL cf_pop_text_vertical_layout();
CUTE_API bool CUTE_CALL cf_peek_text_vertical_layout();
CUTE_API float CUTE_CALL cf_text_width(const char* text);
CUTE_API float CUTE_CALL cf_text_height(const char* text);
CUTE_API void CUTE_CALL cf_draw_text(const char* text, CF_V2 position);

// Fields marked with a star * can be modified in `CF_TextEffectFn` for
// custom text effects.
typedef struct CF_TextEffect
{
	const char* effect_name; // Name of this effect, as registered by `cf_text_effect_register`.
	int character;           // UTF8 codepoint of the current character.
	int index_into_string;   // The index into the string in `cf_draw_text` currently affected.
	int index_into_effect;   // Starts at 0 and increments for each character affected.
	int glyph_count;         // The number of glyphs spanning the entire effect.
	float elapsed;           // How long this effect has persisted for.
	CF_V2 center;            // Center of this glyp's space -- not the same as the center of the glyph quad.
	CF_V2 q0, q1;            // * This glyph's renderable quad. q0 is the min vertex, while q1 is the max vertex.
	int w, h;                // Width and height of the glyph.
	CF_Color color;          // * The color to render this glyph with.
	float opacity;           // * The opacity to render this glyph with.
	float xadvance;          // * How far the text will advance along the x-axis (only applicable for non-vertical layout mode).
	bool visible;            // * Whether or not this glyph is visibly rendered (e.g. not visible for spaces ' ').
	float font_size;         // The last size passed to `cf_push_font_size`.
} CF_TextEffect;
typedef bool (CF_TextEffectFn)(CF_TextEffect* fx);


/**
 * Custom text effects can be registered here. Each text effect will have this callback
 * called once per glyph during rendering. There are a few built-in text effects. The
 * below table shows each effect name and their parameters. Each parameter has a default.
 * 
 *  - color
 *     + example : "Here's some <color=#2c5ee8>blue text</color>."
 *     +         : default (white) - The color to render text with.
 *  - shake
 *     + example : "<shake freq=30 x=3 y=3>This text is all shaky.</shake>"
 *     + example : "<shake y=20>Shake this text with default values, but override for a big height.</shake>"
 *     + freq    : default (35)    - Number of times per second to shake.
 *     + x       : default (2)     - Max +/- distance to shake on x-axis.
 *     + y       : default (2)     - Max +/- distance to shake on y-axis.
 *  - fade
 *     + example : "<fade speed=10 span=3>Fading some text like a ghost~</fade>"
 *     + example : "<fade>Fading some text like a ghost~</fade>"
 *     + speed   : default (2)     - Number of times per second to find in and then out.
 *     + span    : default (5)     - Number of characters long for the fade to loop.
 *  - wave
 *     + example : "<wave>Wobbly wave text.</wave>"
 *     + speed   : default (5)     - Number of times per second to bob up and down.
 *     + span    : default (10)    - Number of characters long for the wave to loop.
 *     + height  : default (5)     - How many characters high the wave will go.
 *  - strike
 *     + example : "<strike>Strikethrough</strike>"
 *     + example : "<strike=10>Thick Strikethrough</strike>"
 *     +         : default (font_height / 20) - The thickness of the strike line.
 * 
 * When registering a custom text effect, any parameters in the string will be stored for you
 * automatically. You only need to fetch them with the appropriate cf_text_effect_get*** function.
 * 
 * Be sure to carefully read the documentation at CF_TextEffect to see which kinds of
 * fields are read-only, and which are modifiable.
 */
CUTE_API void CUTE_CALL cf_text_effect_register(const char* name, CF_TextEffectFn* fn);
CUTE_API bool CUTE_CALL cf_text_effect_on_start(CF_TextEffect* fx);
CUTE_API bool CUTE_CALL cf_text_effect_on_finish(CF_TextEffect* fx);
CUTE_API double CUTE_CALL cf_text_effect_get_number(CF_TextEffect* fx, const char* key, double default_val);
CUTE_API CF_Color CUTE_CALL cf_text_effect_get_color(CF_TextEffect* fx, const char* key, CF_Color default_val);
CUTE_API const char* CUTE_CALL cf_text_effect_get_string(CF_TextEffect* fx, const char* key, const char* default_val);

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
 * `cf_app_draw_onto_screen` is called.
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

//--------------------------------------------------------------------------------------------------
// "Hidden" API -- Just here for some inline C++ functions below.

enum CF_TextCodeValType
{
	CF_TEXT_CODE_VAL_TYPE_NONE,
	CF_TEXT_CODE_VAL_TYPE_COLOR,
	CF_TEXT_CODE_VAL_TYPE_NUMBER,
	CF_TEXT_CODE_VAL_TYPE_STRING,
};

struct CF_TextCodeVal
{
	enum CF_TextCodeValType type;
	union
	{
		CF_Color color;
		double number;
		const char* string;
	} u;
};

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

CUTE_INLINE void make_font(const char* path, const char* font_name, CF_Result* result_out = NULL) { cf_make_font(path, font_name, result_out); }
CUTE_INLINE void make_font_mem(void* data, int size, const char* font_name, CF_Result* result_out = NULL) { cf_make_font_mem(data, size, font_name, result_out); }
CUTE_INLINE void destroy_font(const char* font_name) { cf_destroy_font(font_name); }
CUTE_INLINE void font_add_backup_codepoints(const char* font_name, int* codepoints, int count) { cf_font_add_backup_codepoints(font_name, codepoints, count); }
CUTE_INLINE void push_font(const char* font_name) { cf_push_font(font_name); }
CUTE_INLINE const char* pop_font() { return cf_pop_font(); }
CUTE_INLINE const char* peek_font() { return cf_peek_font(); }
CUTE_INLINE void push_font_size(float size) { cf_push_font_size(size); }
CUTE_INLINE float pop_font_size() { return cf_pop_font_size(); }
CUTE_INLINE float peek_font_size() { return cf_peek_font_size(); }
CUTE_INLINE void push_font_blur(int blur) { cf_push_font_blur(blur); }
CUTE_INLINE int pop_font_blur() { return cf_pop_font_blur(); }
CUTE_INLINE int peek_font_blur() { return cf_peek_font_blur(); }
CUTE_INLINE void push_text_wrap_width(float width) { cf_push_text_wrap_width(width); }
CUTE_INLINE float pop_text_wrap_width() { return cf_pop_text_wrap_width(); }
CUTE_INLINE float peek_text_wrap_width() { return cf_peek_text_wrap_width(); }
CUTE_INLINE void push_text_clip_box(CF_Aabb clip_box) { cf_push_text_clip_box(clip_box); }
CUTE_INLINE CF_Aabb pop_text_clip_box() { return cf_pop_text_clip_box(); }
CUTE_INLINE CF_Aabb peek_text_clip_box() { return cf_peek_text_clip_box(); }
CUTE_INLINE float text_width(const char* text) { return cf_text_width(text); }
CUTE_INLINE float text_height(const char* text) { return cf_text_height(text); }
CUTE_INLINE void draw_text(const char* text, CF_V2 position) { cf_draw_text(text, position); }

struct TextEffect : public CF_TextEffect
{
	CUTE_INLINE bool on_start() const { return index_into_effect == 0; }
	CUTE_INLINE bool on_finish() const { return index_into_effect == glyph_count - 1; }

	CUTE_INLINE double get_number(const char* key, double default_val = 0)
	{
		const CF_TextCodeVal* v = params->try_find(sintern(key));
		if (v && v->type == CF_TEXT_CODE_VAL_TYPE_NUMBER) {
			return v->u.number;
		} else {
			return default_val;
		}
	}
	
	CUTE_INLINE CF_Color get_color(const char* key, CF_Color default_val = cf_color_white())
	{
		const CF_TextCodeVal* v = params->try_find(sintern(key));
		if (v && v->type == CF_TEXT_CODE_VAL_TYPE_COLOR) {
			return v->u.color;
		} else {
			return default_val;
		}
	}
	
	CUTE_INLINE const char* get_string(const char* key, const char* default_val = NULL)
	{
		const CF_TextCodeVal* v = params->try_find(sintern(key));
		if (v && v->type == CF_TEXT_CODE_VAL_TYPE_STRING) {
			return v->u.string;
		} else {
			return default_val;
		}
	}

	// "private" state -- don't touch.
	const Cute::Dictionary<const char*, CF_TextCodeVal>* params;
	CF_TextEffectFn* fn;
};

typedef bool (TextEffectFn)(TextEffect* fx);

CUTE_INLINE void text_effect_register(const char* name, TextEffectFn* fn) { cf_text_effect_register(name, (CF_TextEffectFn*)fn); }

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
