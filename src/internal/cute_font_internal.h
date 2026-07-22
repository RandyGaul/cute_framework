/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_FONT_INTERNAL_H
#define CF_FONT_INTERNAL_H

#include <cute_array.h>
#include <cute_map.h>
#include <cute_math.h>
#include <cute_color.h>
#include <cute_alloc.h>
#include <cute_draw.h>

#include <stb/stb_truetype.h>

struct CF_Glyph
{
	int index;
	uint64_t image_id;
	CF_V2 q0, q1;
	int w, h;
	float xadvance;
	bool visible;
	bool rendered; // Metrics (and, if visible, the atlas image) have been computed.
};

// Resolution-independent glyph outline for the curve text path (cf_push_text_curves):
// the glyph's quadratic Beziers encoded into a small atlas strip (3 RGBA8 texels per
// curve, two 16-bit fixed-point coords each, fractions of box_min..box_max in font
// units). One per glyph index -- size and zoom never invalidate it.
struct CF_CurveGlyph
{
	uint64_t image_id; // Atlas image id of the encoded strip (font id range), 0 when curve_count is 0.
	int curve_count;
	int strip_w;       // Strip width in texels: curve_count * 3.
	CF_V2 box_min;     // Outline bounds in font units (y-up), the quantization box.
	CF_V2 box_max;
};

struct CF_Font
{
	uint8_t* file_data = NULL;
	stbtt_fontinfo info;
	Cute::Map<int> kerning;
	Cute::Map<CF_Glyph> glyphs;
	Cute::Map<CF_CurveGlyph> curve_glyphs; // Keyed by glyph index (not size/blur -- scale-free).
	Cute::Array<uint64_t> image_ids;
	int ascent;
	int descent;
	int line_gap;
	int line_height;
	int width;
	int height;
	int x_height; // Measured from the 'x' glyph box; 0 when the font has no 'x'.
};

CF_API CF_Font* CF_CALL cf_font_get(const char* font_name);
CF_API CF_Glyph* CF_CALL cf_font_get_glyph(CF_Font* font, int codepoint, float font_size, int blur);
CF_API CF_CurveGlyph* CF_CALL cf_font_get_glyph_curves(CF_Font* font, int codepoint);
CF_API float CF_CALL cf_font_get_kern(CF_Font* font, float font_size, int codepoint0, int codepoint1);
CF_API float CF_CALL cf_font_scale_for_pixel_height(CF_Font* font, float pixel_height);

#define CF_KERN_KEY(cp0, cp1) (((uint64_t)cp0) << 32 | ((uint64_t)cp1))

// Registered text-effect definition: optional callback + optional font override.
// Font overrides are resolved at draw/measure time so `cf_text_effect_set_font`
// takes effect even for already-parsed strings.
struct CF_TextEffectDef
{
	CF_TextEffectFn* fn = NULL;
	const char* font_name = NULL; // interned; NULL means no override
};

struct CF_TextCode
{
	const char* effect_name;
	int index_in_string;
	int glyph_count;
	// Monotonic open order so nested codes that share the same start index still
	// resolve outer→inner (later opens win when applying font overrides).
	int parse_order;
	CF_TextEffectFn* fn;
	Cute::Map<CF_TextCodeVal> params;
};

struct TextEffect : public CF_TextEffect
{
	CF_INLINE bool on_start() const { return index_into_effect == 0; }
	CF_INLINE bool on_finish() const { return index_into_effect == glyph_count - 1; }
	CF_INLINE bool has(const char* key) const { return params->try_find(sintern(key)) != NULL; }

	CF_INLINE double get_number(const char* key, double default_val = 0) const
	{
		const CF_TextCodeVal* v = params->try_find(sintern(key));
		if (v && v->type == CF_TEXT_CODE_VAL_TYPE_NUMBER) {
			return v->u.number;
		} else {
			return default_val;
		}
	}

	CF_INLINE CF_Color get_color(const char* key, CF_Color default_val = cf_color_white()) const
	{
		const CF_TextCodeVal* v = params->try_find(sintern(key));
		if (v && v->type == CF_TEXT_CODE_VAL_TYPE_COLOR) {
			return v->u.color;
		} else {
			return default_val;
		}
	}

	CF_INLINE const char* get_string(const char* key, const char* default_val = NULL) const
	{
		const CF_TextCodeVal* v = params->try_find(sintern(key));
		if (v && v->type == CF_TEXT_CODE_VAL_TYPE_STRING) {
			return v->u.string;
		} else {
			return default_val;
		}
	}

	// "private" state -- don't touch.
	int initial_index;
	const Cute::Map<CF_TextCodeVal>* params;
	CF_TextEffectFn* fn;
	float strike_thickness = 0;
	float underline_thickness = 0;
	bool line_bound_init = false;
	CF_Aabb line_bound;
	Cute::Array<CF_Aabb> bounds;
};

struct CF_TextEffectState
{
	float elapsed = 0;
	bool alive = false;
};

struct CF_ParsedTextState
{
	Cute::String sanitized;
	uint64_t hash = 0;
	bool alive = false;
	int next_parse_order = 0;
	Cute::Array<TextEffect> effects;
	Cute::Array<CF_TextCode> codes;
	Cute::Array<CF_TextCode> parse_stack;

	CF_INLINE void parse_add(CF_TextCode code)
	{
		code.parse_order = next_parse_order++;
		parse_stack.add(code);
	}
	CF_INLINE bool parse_finish(const char* effect_name, int final_index)
	{
		for (int i = parse_stack.count() - 1; i >= 0; --i) {
			if (parse_stack[i].effect_name == effect_name) {
				CF_TextCode code = parse_stack[i];
				code.glyph_count = final_index - code.index_in_string;
				parse_stack.unordered_remove(i);
				codes.add(code);
				return true;
			}
		}
		return false;
	}
};

#endif // CF_FONT_INTERNAL_H
