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

#ifndef CUTE_FONT_INTERNAL_H
#define CUTE_FONT_INTERNAL_H

#include <cute_array.h>
#include <cute_hashtable.h>
#include <cute_math.h>
#include <cute_color.h>
#include <cute_alloc.h>

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
	Cute::Array<int> backups;
	Cute::Dictionary<uint64_t, int> kerning;
	Cute::Dictionary<uint64_t, CF_Glyph> glyphs;
	Cute::Array<uint64_t> image_ids;
	int ascent;
	int descent;
	int line_gap;
	int line_height;
	int height;
};

CF_Font* cf_font_get(const char* font_name);
CF_Glyph* cf_font_get_glyph(CF_Font* font, int codepoint, float font_size, int blur);
float cf_font_get_kern(CF_Font* font, float font_size, int codepoint0, int codepoint1);

#define CF_KERN_KEY(cp0, cp1) (((uint64_t)cp0) << 32 | ((uint64_t)cp1))

enum CF_TextCodeValType
{
	CF_TEXT_CODE_VAL_TYPE_NONE,
	CF_TEXT_CODE_VAL_TYPE_COLOR,
	CF_TEXT_CODE_VAL_TYPE_NUMBER,
	CF_TEXT_CODE_VAL_TYPE_STRING,
};

struct CF_TextCodeVal
{
	CF_TextCodeValType type;
	union
	{
		CF_Color color;
		double number;
		const char* string;
	} u;
};

typedef bool (CF_TextEffectFn)(struct CF_TextEffect* effect);

struct CF_TextCode
{
	const char* effect_name;
	int index_in_string;
	int glyph_count;
	CF_TextEffectFn* fn;
	Cute::Dictionary<const char*, CF_TextCodeVal> params;
};

struct CF_TextEffect
{
	const char* effect_name;
	int character;
	int index_into_string;
	int index_into_effect;
	int glyph_count;
	float elapsed;
	CF_V2 center;
	CF_V2 q0, q1;
	int w, h;
	CF_Color color;
	float opacity;
	float xadvance;
	bool visible;
	float font_size;
	const Cute::Dictionary<const char*, CF_TextCodeVal>* params;
	CF_TextEffectFn* fn;

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
};

struct CF_TextEffectState
{
	Cute::String sanitized;
	uint64_t hash = 0;
	float elapsed = 0;
	bool alive = false;
	Cute::Array<CF_TextEffect> effects;
	Cute::Array<CF_TextCode> codes;
	Cute::Array<CF_TextCode> parse_stack;

	CUTE_INLINE void parse_add(CF_TextCode code) { parse_stack.add(code); }
	CUTE_INLINE bool parse_finish(const char* effect_name, int final_index)
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

void cf_text_effect_register(const char* name, CF_TextEffectFn* fn);

#endif // CUTE_FONT_INTERNAL_H
