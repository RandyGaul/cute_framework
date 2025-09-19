/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_FONT_INTERNAL_H
#define CF_FONT_INTERNAL_H

#include <cute_array.h>
#include <cute_hashtable.h>
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
};

struct CF_Font
{
	uint8_t* file_data = NULL;
	stbtt_fontinfo info;
	Cute::Map<uint64_t, int> kerning;
	Cute::Map<uint64_t, CF_Glyph> glyphs;
	Cute::Array<uint64_t> image_ids;
	int ascent;
	int descent;
	int line_gap;
	int line_height;
	int width;
	int height;
};

CF_Font* cf_font_get(const char* font_name);
CF_Glyph* cf_font_get_glyph(CF_Font* font, int codepoint, float font_size, int blur);
float cf_font_get_kern(CF_Font* font, float font_size, int codepoint0, int codepoint1);

#define CF_KERN_KEY(cp0, cp1) (((uint64_t)cp0) << 32 | ((uint64_t)cp1))

struct CF_TextCode
{
	const char* effect_name;
	int index_in_string;
	int glyph_count;
	CF_TextEffectFn* fn;
	Cute::Map<const char*, CF_TextCodeVal> params;
};

struct TextEffect : public CF_TextEffect
{
	CF_INLINE bool on_start() const { return index_into_effect == 0; }
	CF_INLINE bool on_finish() const { return index_into_effect == glyph_count - 1; }

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
	const Cute::Map<const char*, CF_TextCodeVal>* params;
	CF_TextEffectFn* fn;
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
	Cute::Array<TextEffect> effects;
	Cute::Array<CF_TextCode> codes;
	Cute::Array<CF_TextCode> parse_stack;

	CF_INLINE void parse_add(CF_TextCode code) { parse_stack.add(code); }
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
