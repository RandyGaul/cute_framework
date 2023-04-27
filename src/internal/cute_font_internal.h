/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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
	Cute::Array<int> backups;
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

struct CF_TextEffectState
{
	Cute::String sanitized;
	uint64_t hash = 0;
	float elapsed = 0;
	bool alive = false;
	Cute::Array<Cute::TextEffect> effects;
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
