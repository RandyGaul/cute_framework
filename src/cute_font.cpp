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

#include <cute_defines.h>

#include <cute_font.h>
#include <cute_c_runtime.h>
#include <cute_file_system.h>
#include <cute_math.h>
#include <cute_defer.h>
#include <internal/cute_app_internal.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#include <cute/cute_png.h>

using namespace cute;

struct glyph_t
{
	v2 u, v;
	int w, h;
	v2 offset;
	float xadvance;
	bool visible;
};

struct font_internal_t
{
	uint8_t* file_data = NULL;
	stbtt_fontinfo info;
	array<codepoint_range_t> ranges;
	dictionary<int, glyph_t> glyphs;
	uint8_t* pixels = NULL;
	float size;
	float scale;
	float ascent;
	float descent;
	float line_gap;
	float line_height;
	float height;
};

// These character range functions are from Dear ImGui v1.89.
// https://github.com/ocornut/imgui

// Dear ImGui uses a simpler function to unpack ranges called `UnpackAccumulativeOffsetsIntoRanges`.
// This one is used instead to reduce the number of duplicate ranges reported. It's a very minor
// difference only added for fun, not really for practical reasons. For example, the function
// `chinese_simplified_common` would normally reported ~2800 codepoint ranges, but with this
// tweaked function reports ~1920 codepoint ranges. The ranges are functionally equivalent; this
// does not change the number of glyphs that get used.
static int s_unpack_ranges(int base, const short* deltas, int deltas_count, codepoint_range_t* out_ranges)
{
	int i = 1;
	int lo = base + *deltas;
	int hi = lo;
	int out_count = 0;
	while (i <= deltas_count) {
		int delta = deltas[i++];
		if (delta == 1) {
			++hi;
			continue;
		}
		*out_ranges++ = { lo, hi };
		lo = hi + delta;
		hi = lo;
		out_count++;
	}
	return out_count;
}

codepoint_set_t cf_ascii_latin()
{
	static codepoint_range_t ranges[] = {
		{ 0x0020, 0x00FF, }, // Basic Latin + Latin Supplement
		{ 0xFFFD, 0xFFFD }  // Replacement character
	};
	codepoint_set_t set = { CUTE_ARRAY_SIZE(ranges), ranges };
	return set;
}

codepoint_set_t cf_greek()
{
	static codepoint_range_t ranges[] = {
		{ 0x0020, 0x00FF, }, // Basic Latin + Latin Supplement
		{ 0x0370, 0x03FF, }, // Greek and Coptic
		{ 0xFFFD, 0xFFFD }  // Replacement character
	};
	codepoint_set_t set = { CUTE_ARRAY_SIZE(ranges), ranges };
	return set;
}

codepoint_set_t cf_korean()
{
	static codepoint_range_t ranges[] = {
		{ 0x0020, 0x00FF }, // Basic Latin + Latin Supplement
		{ 0x3131, 0x3163 }, // Korean alphabets
		{ 0xAC00, 0xD7A3 }, // Korean characters
		{ 0xFFFD, 0xFFFD }, // Replacement character
	};
	codepoint_set_t set = { CUTE_ARRAY_SIZE(ranges), ranges };
	return set;
}

codepoint_set_t cf_chinese_full()
{
	static codepoint_range_t ranges[] = {
		{ 0x0020, 0x00FF }, // Basic Latin + Latin Supplement
		{ 0x2000, 0x206F }, // General Punctuation
		{ 0x3000, 0x30FF }, // CJK Symbols and Punctuations, Hiragana, Katakana
		{ 0x31F0, 0x31FF }, // Katakana Phonetic Extensions
		{ 0xFF00, 0xFFEF }, // Half-width characters
		{ 0xFFFD, 0xFFFD }, // Replacement character
		{ 0x4e00, 0x9FAF }, // CJK Ideograms
	};
	codepoint_set_t set = { CUTE_ARRAY_SIZE(ranges), ranges };
	return set;
}

codepoint_set_t cf_chinese_simplified_common()
{
	// Store 2500 regularly used characters for Simplified Chinese.
	// Sourced from https://zh.wiktionary.org/wiki/%E9%99%84%E5%BD%95:%E7%8E%B0%E4%BB%A3%E6%B1%89%E8%AF%AD%E5%B8%B8%E7%94%A8%E5%AD%97%E8%A1%A8
	// This table covers 97.97% of all characters used during the month in July, 1987.
	// Stored as prefix difference array with the initial offset of 0x4E00. This encoding is designed to reduce source code size.
	static const short differences_from_0x4E00[] =
	{
		0,1,2,4,1,1,1,1,2,1,3,2,1,2,2,1,1,1,1,1,5,2,1,2,3,3,3,2,2,4,1,1,1,2,1,5,2,3,1,2,1,2,1,1,2,1,1,2,2,1,4,1,1,1,1,5,10,1,2,19,2,1,2,1,2,1,2,1,2,
		1,5,1,6,3,2,1,2,2,1,1,1,4,8,5,1,1,4,1,1,3,1,2,1,5,1,2,1,1,1,10,1,1,5,2,4,6,1,4,2,2,2,12,2,1,1,6,1,1,1,4,1,1,4,6,5,1,4,2,2,4,10,7,1,1,4,2,4,
		2,1,4,3,6,10,12,5,7,2,14,2,9,1,1,6,7,10,4,7,13,1,5,4,8,4,1,1,2,28,5,6,1,1,5,2,5,20,2,2,9,8,11,2,9,17,1,8,6,8,27,4,6,9,20,11,27,6,68,2,2,1,1,
		1,2,1,2,2,7,6,11,3,3,1,1,3,1,2,1,1,1,1,1,3,1,1,8,3,4,1,5,7,2,1,4,4,8,4,2,1,2,1,1,4,5,6,3,6,2,12,3,1,3,9,2,4,3,4,1,5,3,3,1,3,7,1,5,1,1,1,1,2,
		3,4,5,2,3,2,6,1,1,2,1,7,1,7,3,4,5,15,2,2,1,5,3,22,19,2,1,1,1,1,2,5,1,1,1,6,1,1,12,8,2,9,18,22,4,1,1,5,1,16,1,2,7,10,15,1,1,6,2,4,1,2,4,1,6,
		1,1,3,2,4,1,6,4,5,1,2,1,1,2,1,10,3,1,3,2,1,9,3,2,5,7,2,19,4,3,6,1,1,1,1,1,4,3,2,1,1,1,2,5,3,1,1,1,2,2,1,1,2,1,1,2,1,3,1,1,1,3,7,1,4,1,1,2,1,
		1,2,1,2,4,4,3,8,1,1,1,2,1,3,5,1,3,1,3,4,6,2,2,14,4,6,6,11,9,1,15,3,1,28,5,2,5,5,3,1,3,4,5,4,6,14,3,2,3,5,21,2,7,20,10,1,2,19,2,4,28,28,2,3,
		2,1,14,4,1,26,28,42,12,40,3,52,79,5,14,17,3,2,2,11,3,4,6,3,1,8,2,23,4,5,8,10,4,2,7,3,5,1,1,6,3,1,2,2,2,5,28,1,1,7,7,20,5,3,29,3,17,26,1,8,4,
		27,3,6,11,23,5,3,4,6,13,24,16,6,5,10,25,35,7,3,2,3,3,14,3,6,2,6,1,4,2,3,8,2,1,1,3,3,3,4,1,1,13,2,2,4,5,2,1,14,14,1,2,2,1,4,5,2,3,1,14,3,12,
		3,17,2,16,5,1,2,1,8,9,3,19,4,2,2,4,17,25,21,20,28,75,1,10,29,103,4,1,2,1,1,4,2,4,1,2,3,24,2,2,2,1,1,2,1,3,8,1,1,1,2,1,1,3,1,1,1,6,1,5,3,1,1,
		1,3,4,1,1,5,2,1,5,6,13,9,16,1,1,1,1,3,2,3,2,4,5,2,5,2,2,3,7,13,7,2,2,1,1,1,1,2,3,3,2,1,6,4,9,2,1,14,2,14,2,1,18,3,4,14,4,11,41,15,23,15,23,
		176,1,3,4,1,1,1,1,5,3,1,2,3,7,3,1,1,2,1,2,4,4,6,2,4,1,9,7,1,10,5,8,16,29,1,1,2,2,3,1,3,5,2,4,5,4,1,1,2,2,3,3,7,1,6,10,1,17,1,44,4,6,2,1,1,6,
		5,4,2,10,1,6,9,2,8,1,24,1,2,13,7,8,8,2,1,4,1,3,1,3,3,5,2,5,10,9,4,9,12,2,1,6,1,10,1,1,7,7,4,10,8,3,1,13,4,3,1,6,1,3,5,2,1,2,17,16,5,2,16,6,
		1,4,2,1,3,3,6,8,5,11,11,1,3,3,2,4,6,10,9,5,7,4,7,4,7,1,1,4,2,1,3,6,8,7,1,6,11,5,5,3,24,9,4,2,7,13,5,1,8,82,16,61,1,1,1,4,2,2,16,10,3,8,1,1,
		6,4,2,1,3,1,1,1,4,3,8,4,2,2,1,1,1,1,1,6,3,5,1,1,4,6,9,2,1,1,1,2,1,7,2,1,6,1,5,4,4,3,1,8,1,3,3,1,3,2,2,2,2,3,1,6,1,2,1,2,1,3,7,1,8,2,1,2,1,5,
		2,5,3,5,10,1,2,1,1,3,2,5,11,3,9,3,5,1,1,5,9,1,2,1,5,7,9,9,8,1,3,3,3,6,8,2,3,2,1,1,32,6,1,2,15,9,3,7,13,1,3,10,13,2,14,1,13,10,2,1,3,10,4,15,
		2,15,15,10,1,3,9,6,9,32,25,26,47,7,3,2,3,1,6,3,4,3,2,8,5,4,1,9,4,2,2,19,10,6,2,3,8,1,2,2,4,2,1,9,4,4,4,6,4,8,9,2,3,1,1,1,1,3,5,5,1,3,8,4,6,
		2,1,4,12,1,5,3,7,13,2,5,8,1,6,1,2,5,14,6,1,5,2,4,8,15,5,1,23,6,62,2,10,1,1,8,1,2,2,10,4,2,2,9,2,1,1,3,2,3,1,5,3,3,2,1,3,8,1,1,1,11,3,1,1,4,
		3,7,1,14,1,2,3,12,5,2,5,1,6,7,5,7,14,11,1,3,1,8,9,12,2,1,11,8,4,4,2,6,10,9,13,1,1,3,1,5,1,3,2,4,4,1,18,2,3,14,11,4,29,4,2,7,1,3,13,9,2,2,5,
		3,5,20,7,16,8,5,72,34,6,4,22,12,12,28,45,36,9,7,39,9,191,1,1,1,4,11,8,4,9,2,3,22,1,1,1,1,4,17,1,7,7,1,11,31,10,2,4,8,2,3,2,1,4,2,16,4,32,2,
		3,19,13,4,9,1,5,2,14,8,1,1,3,6,19,6,5,1,16,6,2,10,8,5,1,2,3,1,5,5,1,11,6,6,1,3,3,2,6,3,8,1,1,4,10,7,5,7,7,5,8,9,2,1,3,4,1,1,3,1,3,3,2,6,16,
		1,4,6,3,1,10,6,1,3,15,2,9,2,10,25,13,9,16,6,2,2,10,11,4,3,9,1,2,6,6,5,4,30,40,1,10,7,12,14,33,6,3,6,7,3,1,3,1,11,14,4,9,5,12,11,49,18,51,31,
		140,31,2,2,1,5,1,8,1,10,1,4,4,3,24,1,10,1,3,6,6,16,3,4,5,2,1,4,2,57,10,6,22,2,22,3,7,22,6,10,11,36,18,16,33,36,2,5,5,1,1,1,4,10,1,4,13,2,7,
		5,2,9,3,4,1,7,43,3,7,3,9,14,7,9,1,11,1,1,3,7,4,18,13,1,14,1,3,6,10,73,2,2,30,6,1,11,18,19,13,22,3,46,42,37,89,7,3,16,34,2,2,3,9,1,7,1,1,1,2,
		2,4,10,7,3,10,3,9,5,28,9,2,6,13,7,3,1,3,10,2,7,2,11,3,6,21,54,85,2,1,4,2,2,1,39,3,21,2,2,5,1,1,1,4,1,1,3,4,15,1,3,2,4,4,2,3,8,2,20,1,8,7,13,
		4,1,26,6,2,9,34,4,21,52,10,4,4,1,5,12,2,11,1,7,2,30,12,44,2,30,1,1,3,6,16,9,17,39,82,2,2,24,7,1,7,3,16,9,14,44,2,1,2,1,2,3,5,2,4,1,6,7,5,3,
		2,6,1,11,5,11,2,1,18,19,8,1,3,24,29,2,1,3,5,2,2,1,13,6,5,1,46,11,3,5,1,1,5,8,2,10,6,12,6,3,7,11,2,4,16,13,2,5,1,1,2,2,5,2,28,5,2,23,10,8,4,
		4,22,39,95,38,8,14,9,5,1,13,5,4,3,13,12,11,1,9,1,27,37,2,5,4,4,63,211,95,2,2,2,1,3,5,2,1,1,2,2,1,1,1,3,2,4,1,2,1,1,5,2,2,1,1,2,3,1,3,1,1,1,
		3,1,4,2,1,3,6,1,1,3,7,15,5,3,2,5,3,9,11,4,2,22,1,6,3,8,7,1,4,28,4,16,3,3,25,4,4,27,27,1,4,1,2,2,7,1,3,5,2,28,8,2,14,1,8,6,16,25,3,3,3,14,3,
		3,1,1,2,1,4,6,3,8,4,1,1,1,2,3,6,10,6,2,3,18,3,2,5,5,4,3,1,5,2,5,4,23,7,6,12,6,4,17,11,9,5,1,1,10,5,12,1,1,11,26,33,7,3,6,1,17,7,1,5,12,1,11,
		2,4,1,8,14,17,23,1,2,1,7,8,16,11,9,6,5,2,6,4,16,2,8,14,1,11,8,9,1,1,1,9,25,4,11,19,7,2,15,2,12,8,52,7,5,19,2,16,4,36,8,1,16,8,24,26,4,6,2,9,
		5,4,36,3,28,12,25,15,37,27,17,12,59,38,5,32,127,1,2,9,17,14,4,1,2,1,1,8,11,50,4,14,2,19,16,4,17,5,4,5,26,12,45,2,23,45,104,30,12,8,3,10,2,2,
		3,3,1,4,20,7,2,9,6,15,2,20,1,3,16,4,11,15,6,134,2,5,59,1,2,2,2,1,9,17,3,26,137,10,211,59,1,2,4,1,4,1,1,1,2,6,2,3,1,1,2,3,2,3,1,3,4,4,2,3,3,
		1,4,3,1,7,2,2,3,1,2,1,3,3,3,2,2,3,2,1,3,14,6,1,3,2,9,6,15,27,9,34,145,1,1,2,1,1,1,1,2,1,1,1,1,2,2,2,3,1,2,1,1,1,2,3,5,8,3,5,2,4,1,3,2,2,2,12,
		4,1,1,1,10,4,5,1,20,4,16,1,15,9,5,12,2,9,2,5,4,2,26,19,7,1,26,4,30,12,15,42,1,6,8,172,1,1,4,2,1,1,11,2,2,4,2,1,2,1,10,8,1,2,1,4,5,1,2,5,1,8,
		4,1,3,4,2,1,6,2,1,3,4,1,2,1,1,1,1,12,5,7,2,4,3,1,1,1,3,3,6,1,2,2,3,3,3,2,1,2,12,14,11,6,6,4,12,2,8,1,7,10,1,35,7,4,13,15,4,3,23,21,28,52,5,
		26,5,6,1,7,10,2,7,53,3,2,1,1,1,2,163,532,1,10,11,1,3,3,4,8,2,8,6,2,2,23,22,4,2,2,4,2,1,3,1,3,3,5,9,8,2,1,2,8,1,10,2,12,21,20,15,105,2,3,1,1,
		3,2,3,1,1,2,5,1,4,15,11,19,1,1,1,1,5,4,5,1,1,2,5,3,5,12,1,2,5,1,11,1,1,15,9,1,4,5,3,26,8,2,1,3,1,1,15,19,2,12,1,2,5,2,7,2,19,2,20,6,26,7,5,
		2,2,7,34,21,13,70,2,128,1,1,2,1,1,2,1,1,3,2,2,2,15,1,4,1,3,4,42,10,6,1,49,85,8,1,2,1,1,4,4,2,3,6,1,5,7,4,3,211,4,1,2,1,2,5,1,2,4,2,2,6,5,6,
		10,3,4,48,100,6,2,16,296,5,27,387,2,2,3,7,16,8,5,38,15,39,21,9,10,3,7,59,13,27,21,47,5,21,6
	};
	static codepoint_range_t base_ranges[] = {
		{ 0x0020, 0x00FF }, // Basic Latin + Latin Supplement
		{ 0x2000, 0x206F }, // General Punctuation
		{ 0x3000, 0x30FF }, // CJK Symbols and Punctuations, Hiragana, Katakana
		{ 0x31F0, 0x31FF }, // Katakana Phonetic Extensions
		{ 0xFF00, 0xFFEF }, // Half-width characters
		{ 0xFFFD, 0xFFFD }  // Replacement character
	};
	static codepoint_range_t ranges[CUTE_ARRAY_SIZE(base_ranges) + CUTE_ARRAY_SIZE(differences_from_0x4E00) * 2] = { 0 };
	static int count = 0;
	if (!count) {
		CUTE_MEMCPY(ranges, base_ranges, sizeof(base_ranges));
		count = s_unpack_ranges(0x4E00, differences_from_0x4E00, CUTE_ARRAY_SIZE(differences_from_0x4E00), ranges + CUTE_ARRAY_SIZE(base_ranges));
	}
	codepoint_set_t set = { count, ranges };
	return set;
}

codepoint_set_t cf_japanese()
{
	// 2999 ideograms code points for Japanese
	// - 2136 Joyo (meaning "for regular use" or "for common use") Kanji code points
	// - 863 Jinmeiyo (meaning "for personal name") Kanji code points
	// - Sourced from the character information database of the Information-technology Promotion Agency, Japan
	//   - https://mojikiban.ipa.go.jp/mji/
	//   - Available under the terms of the Creative Commons Attribution-ShareAlike 2.1 Japan (CC BY-SA 2.1 JP).
	//     - https://creativecommons.org/licenses/by-sa/2.1/jp/deed.en
	//     - https://creativecommons.org/licenses/by-sa/2.1/jp/legalcode
	//   - You can generate this code by the script at:
	//     - https://github.com/vaiorabbit/everyday_use_kanji
	// - References:
	//   - List of Joyo Kanji
	//     - (Official list by the Agency for Cultural Affairs) https://www.bunka.go.jp/kokugo_nihongo/sisaku/joho/joho/kakuki/14/tosin02/index.html
	//     - (Wikipedia) https://en.wikipedia.org/wiki/List_of_j%C5%8Dy%C5%8D_kanji
	//   - List of Jinmeiyo Kanji
	//     - (Official list by the Ministry of Justice) http://www.moj.go.jp/MINJI/minji86.html
	//     - (Wikipedia) https://en.wikipedia.org/wiki/Jinmeiy%C5%8D_kanji
	// - Missing 1 Joyo Kanji: U+20B9F (Kun'yomi: Shikaru, On'yomi: Shitsu,shichi), see https://github.com/ocornut/imgui/pull/3627 for details.
	// Stored as prefix difference array with the initial offset of 0x4E00. This encoding is designed to reduce source code size.
	static const short differences_from_0x4E00[] = {
		0,1,2,4,1,1,1,1,2,1,3,3,2,2,1,5,3,5,7,5,6,1,2,1,7,2,6,3,1,8,1,1,4,1,1,18,2,11,2,6,2,1,2,1,5,1,2,1,3,1,2,1,2,3,3,1,1,2,3,1,1,1,12,7,9,1,4,5,1,
		1,2,1,10,1,1,9,2,2,4,5,6,9,3,1,1,1,1,9,3,18,5,2,2,2,2,1,6,3,7,1,1,1,1,2,2,4,2,1,23,2,10,4,3,5,2,4,10,2,4,13,1,6,1,9,3,1,1,6,6,7,6,3,1,2,11,3,
		2,2,3,2,15,2,2,5,4,3,6,4,1,2,5,2,12,16,6,13,9,13,2,1,1,7,16,4,7,1,19,1,5,1,2,2,7,7,8,2,6,5,4,9,18,7,4,5,9,13,11,8,15,2,1,1,1,2,1,2,2,1,2,2,8,
		2,9,3,3,1,1,4,4,1,1,1,4,9,1,4,3,5,5,2,7,5,3,4,8,2,1,13,2,3,3,1,14,1,1,4,5,1,3,6,1,5,2,1,1,3,3,3,3,1,1,2,7,6,6,7,1,4,7,6,1,1,1,1,1,12,3,3,9,5,
		2,6,1,5,6,1,2,3,18,2,4,14,4,1,3,6,1,1,6,3,5,5,3,2,2,2,2,12,3,1,4,2,3,2,3,11,1,7,4,1,2,1,3,17,1,9,1,24,1,1,4,2,2,4,1,2,7,1,1,1,3,1,2,2,4,15,1,
		1,2,1,1,2,1,5,2,5,20,2,5,9,1,10,8,7,6,1,1,1,1,1,1,6,2,1,2,8,1,1,1,1,5,1,1,3,1,1,1,1,3,1,1,12,4,1,3,1,1,1,1,1,10,3,1,7,5,13,1,2,3,4,6,1,1,30,
		2,9,9,1,15,38,11,3,1,8,24,7,1,9,8,10,2,1,9,31,2,13,6,2,9,4,49,5,2,15,2,1,10,2,1,1,1,2,2,6,15,30,35,3,14,18,8,1,16,10,28,12,19,45,38,1,3,2,3,
		13,2,1,7,3,6,5,3,4,3,1,5,7,8,1,5,3,18,5,3,6,1,21,4,24,9,24,40,3,14,3,21,3,2,1,2,4,2,3,1,15,15,6,5,1,1,3,1,5,6,1,9,7,3,3,2,1,4,3,8,21,5,16,4,
		5,2,10,11,11,3,6,3,2,9,3,6,13,1,2,1,1,1,1,11,12,6,6,1,4,2,6,5,2,1,1,3,3,6,13,3,1,1,5,1,2,3,3,14,2,1,2,2,2,5,1,9,5,1,1,6,12,3,12,3,4,13,2,14,
		2,8,1,17,5,1,16,4,2,2,21,8,9,6,23,20,12,25,19,9,38,8,3,21,40,25,33,13,4,3,1,4,1,2,4,1,2,5,26,2,1,1,2,1,3,6,2,1,1,1,1,1,1,2,3,1,1,1,9,2,3,1,1,
		1,3,6,3,2,1,1,6,6,1,8,2,2,2,1,4,1,2,3,2,7,3,2,4,1,2,1,2,2,1,1,1,1,1,3,1,2,5,4,10,9,4,9,1,1,1,1,1,1,5,3,2,1,6,4,9,6,1,10,2,31,17,8,3,7,5,40,1,
		7,7,1,6,5,2,10,7,8,4,15,39,25,6,28,47,18,10,7,1,3,1,1,2,1,1,1,3,3,3,1,1,1,3,4,2,1,4,1,3,6,10,7,8,6,2,2,1,3,3,2,5,8,7,9,12,2,15,1,1,4,1,2,1,1,
		1,3,2,1,3,3,5,6,2,3,2,10,1,4,2,8,1,1,1,11,6,1,21,4,16,3,1,3,1,4,2,3,6,5,1,3,1,1,3,3,4,6,1,1,10,4,2,7,10,4,7,4,2,9,4,3,1,1,1,4,1,8,3,4,1,3,1,
		6,1,4,2,1,4,7,2,1,8,1,4,5,1,1,2,2,4,6,2,7,1,10,1,1,3,4,11,10,8,21,4,6,1,3,5,2,1,2,28,5,5,2,3,13,1,2,3,1,4,2,1,5,20,3,8,11,1,3,3,3,1,8,10,9,2,
		10,9,2,3,1,1,2,4,1,8,3,6,1,7,8,6,11,1,4,29,8,4,3,1,2,7,13,1,4,1,6,2,6,12,12,2,20,3,2,3,6,4,8,9,2,7,34,5,1,18,6,1,1,4,4,5,7,9,1,2,2,4,3,4,1,7,
		2,2,2,6,2,3,25,5,3,6,1,4,6,7,4,2,1,4,2,13,6,4,4,3,1,5,3,4,4,3,2,1,1,4,1,2,1,1,3,1,11,1,6,3,1,7,3,6,2,8,8,6,9,3,4,11,3,2,10,12,2,5,11,1,6,4,5,
		3,1,8,5,4,6,6,3,5,1,1,3,2,1,2,2,6,17,12,1,10,1,6,12,1,6,6,19,9,6,16,1,13,4,4,15,7,17,6,11,9,15,12,6,7,2,1,2,2,15,9,3,21,4,6,49,18,7,3,2,3,1,
		6,8,2,2,6,2,9,1,3,6,4,4,1,2,16,2,5,2,1,6,2,3,5,3,1,2,5,1,2,1,9,3,1,8,6,4,8,11,3,1,1,1,1,3,1,13,8,4,1,3,2,2,1,4,1,11,1,5,2,1,5,2,5,8,6,1,1,7,
		4,3,8,3,2,7,2,1,5,1,5,2,4,7,6,2,8,5,1,11,4,5,3,6,18,1,2,13,3,3,1,21,1,1,4,1,4,1,1,1,8,1,2,2,7,1,2,4,2,2,9,2,1,1,1,4,3,6,3,12,5,1,1,1,5,6,3,2,
		4,8,2,2,4,2,7,1,8,9,5,2,3,2,1,3,2,13,7,14,6,5,1,1,2,1,4,2,23,2,1,1,6,3,1,4,1,15,3,1,7,3,9,14,1,3,1,4,1,1,5,8,1,3,8,3,8,15,11,4,14,4,4,2,5,5,
		1,7,1,6,14,7,7,8,5,15,4,8,6,5,6,2,1,13,1,20,15,11,9,2,5,6,2,11,2,6,2,5,1,5,8,4,13,19,25,4,1,1,11,1,34,2,5,9,14,6,2,2,6,1,1,14,1,3,14,13,1,6,
		12,21,14,14,6,32,17,8,32,9,28,1,2,4,11,8,3,1,14,2,5,15,1,1,1,1,3,6,4,1,3,4,11,3,1,1,11,30,1,5,1,4,1,5,8,1,1,3,2,4,3,17,35,2,6,12,17,3,1,6,2,
		1,1,12,2,7,3,3,2,1,16,2,8,3,6,5,4,7,3,3,8,1,9,8,5,1,2,1,3,2,8,1,2,9,12,1,1,2,3,8,3,24,12,4,3,7,5,8,3,3,3,3,3,3,1,23,10,3,1,2,2,6,3,1,16,1,16,
		22,3,10,4,11,6,9,7,7,3,6,2,2,2,4,10,2,1,1,2,8,7,1,6,4,1,3,3,3,5,10,12,12,2,3,12,8,15,1,1,16,6,6,1,5,9,11,4,11,4,2,6,12,1,17,5,13,1,4,9,5,1,11,
		2,1,8,1,5,7,28,8,3,5,10,2,17,3,38,22,1,2,18,12,10,4,38,18,1,4,44,19,4,1,8,4,1,12,1,4,31,12,1,14,7,75,7,5,10,6,6,13,3,2,11,11,3,2,5,28,15,6,18,
		18,5,6,4,3,16,1,7,18,7,36,3,5,3,1,7,1,9,1,10,7,2,4,2,6,2,9,7,4,3,32,12,3,7,10,2,23,16,3,1,12,3,31,4,11,1,3,8,9,5,1,30,15,6,12,3,2,2,11,19,9,
		14,2,6,2,3,19,13,17,5,3,3,25,3,14,1,1,1,36,1,3,2,19,3,13,36,9,13,31,6,4,16,34,2,5,4,2,3,3,5,1,1,1,4,3,1,17,3,2,3,5,3,1,3,2,3,5,6,3,12,11,1,3,
		1,2,26,7,12,7,2,14,3,3,7,7,11,25,25,28,16,4,36,1,2,1,6,2,1,9,3,27,17,4,3,4,13,4,1,3,2,2,1,10,4,2,4,6,3,8,2,1,18,1,1,24,2,2,4,33,2,3,63,7,1,6,
		40,7,3,4,4,2,4,15,18,1,16,1,1,11,2,41,14,1,3,18,13,3,2,4,16,2,17,7,15,24,7,18,13,44,2,2,3,6,1,1,7,5,1,7,1,4,3,3,5,10,8,2,3,1,8,1,1,27,4,2,1,
		12,1,2,1,10,6,1,6,7,5,2,3,7,11,5,11,3,6,6,2,3,15,4,9,1,1,2,1,2,11,2,8,12,8,5,4,2,3,1,5,2,2,1,14,1,12,11,4,1,11,17,17,4,3,2,5,5,7,3,1,5,9,9,8,
		2,5,6,6,13,13,2,1,2,6,1,2,2,49,4,9,1,2,10,16,7,8,4,3,2,23,4,58,3,29,1,14,19,19,11,11,2,7,5,1,3,4,6,2,18,5,12,12,17,17,3,3,2,4,1,6,2,3,4,3,1,
		1,1,1,5,1,1,9,1,3,1,3,6,1,8,1,1,2,6,4,14,3,1,4,11,4,1,3,32,1,2,4,13,4,1,2,4,2,1,3,1,11,1,4,2,1,4,4,6,3,5,1,6,5,7,6,3,23,3,5,3,5,3,3,13,3,9,10,
		1,12,10,2,3,18,13,7,160,52,4,2,2,3,2,14,5,4,12,4,6,4,1,20,4,11,6,2,12,27,1,4,1,2,2,7,4,5,2,28,3,7,25,8,3,19,3,6,10,2,2,1,10,2,5,4,1,3,4,1,5,
		3,2,6,9,3,6,2,16,3,3,16,4,5,5,3,2,1,2,16,15,8,2,6,21,2,4,1,22,5,8,1,1,21,11,2,1,11,11,19,13,12,4,2,3,2,3,6,1,8,11,1,4,2,9,5,2,1,11,2,9,1,1,2,
		14,31,9,3,4,21,14,4,8,1,7,2,2,2,5,1,4,20,3,3,4,10,1,11,9,8,2,1,4,5,14,12,14,2,17,9,6,31,4,14,1,20,13,26,5,2,7,3,6,13,2,4,2,19,6,2,2,18,9,3,5,
		12,12,14,4,6,2,3,6,9,5,22,4,5,25,6,4,8,5,2,6,27,2,35,2,16,3,7,8,8,6,6,5,9,17,2,20,6,19,2,13,3,1,1,1,4,17,12,2,14,7,1,4,18,12,38,33,2,10,1,1,
		2,13,14,17,11,50,6,33,20,26,74,16,23,45,50,13,38,33,6,6,7,4,4,2,1,3,2,5,8,7,8,9,3,11,21,9,13,1,3,10,6,7,1,2,2,18,5,5,1,9,9,2,68,9,19,13,2,5,
		1,4,4,7,4,13,3,9,10,21,17,3,26,2,1,5,2,4,5,4,1,7,4,7,3,4,2,1,6,1,1,20,4,1,9,2,2,1,3,3,2,3,2,1,1,1,20,2,3,1,6,2,3,6,2,4,8,1,3,2,10,3,5,3,4,4,
		3,4,16,1,6,1,10,2,4,2,1,1,2,10,11,2,2,3,1,24,31,4,10,10,2,5,12,16,164,15,4,16,7,9,15,19,17,1,2,1,1,5,1,1,1,1,1,3,1,4,3,1,3,1,3,1,2,1,1,3,3,7,
		2,8,1,2,2,2,1,3,4,3,7,8,12,92,2,10,3,1,3,14,5,25,16,42,4,7,7,4,2,21,5,27,26,27,21,25,30,31,2,1,5,13,3,22,5,6,6,11,9,12,1,5,9,7,5,5,22,60,3,5,
		13,1,1,8,1,1,3,3,2,1,9,3,3,18,4,1,2,3,7,6,3,1,2,3,9,1,3,1,3,2,1,3,1,1,1,2,1,11,3,1,6,9,1,3,2,3,1,2,1,5,1,1,4,3,4,1,2,2,4,4,1,7,2,1,2,2,3,5,13,
		18,3,4,14,9,9,4,16,3,7,5,8,2,6,48,28,3,1,1,4,2,14,8,2,9,2,1,15,2,4,3,2,10,16,12,8,7,1,1,3,1,1,1,2,7,4,1,6,4,38,39,16,23,7,15,15,3,2,12,7,21,
		37,27,6,5,4,8,2,10,8,8,6,5,1,2,1,3,24,1,16,17,9,23,10,17,6,1,51,55,44,13,294,9,3,6,2,4,2,2,15,1,1,1,13,21,17,68,14,8,9,4,1,4,9,3,11,7,1,1,1,
		5,6,3,2,1,1,1,2,3,8,1,2,2,4,1,5,5,2,1,4,3,7,13,4,1,4,1,3,1,1,1,5,5,10,1,6,1,5,2,1,5,2,4,1,4,5,7,3,18,2,9,11,32,4,3,3,2,4,7,11,16,9,11,8,13,38,
		32,8,4,2,1,1,2,1,2,4,4,1,1,1,4,1,21,3,11,1,16,1,1,6,1,3,2,4,9,8,57,7,44,1,3,3,13,3,10,1,1,7,5,2,7,21,47,63,3,15,4,7,1,16,1,1,2,8,2,3,42,15,4,
		1,29,7,22,10,3,78,16,12,20,18,4,67,11,5,1,3,15,6,21,31,32,27,18,13,71,35,5,142,4,10,1,2,50,19,33,16,35,37,16,19,27,7,1,133,19,1,4,8,7,20,1,4,
		4,1,10,3,1,6,1,2,51,5,40,15,24,43,22928,11,1,13,154,70,3,1,1,7,4,10,1,2,1,1,2,1,2,1,2,2,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,
		3,2,1,1,1,1,2,1,1,
	};
	static codepoint_range_t base_ranges[] =
	{
		{ 0x0020, 0x00FF }, // Basic Latin + Latin Supplement
		{ 0x3000, 0x30FF }, // CJK Symbols and Punctuations, Hiragana, Katakana
		{ 0x31F0, 0x31FF }, // Katakana Phonetic Extensions
		{ 0xFF00, 0xFFEF }, // Half-width characters
		{ 0xFFFD, 0xFFFD }  // Replacement character
	};
	static codepoint_range_t ranges[CUTE_ARRAY_SIZE(base_ranges) + CUTE_ARRAY_SIZE(differences_from_0x4E00) * 2] = { 0 };
	static int count = 0;
	if (!count) {
		CUTE_MEMCPY(ranges, base_ranges, sizeof(base_ranges));
		count = s_unpack_ranges(0x4E00, differences_from_0x4E00, CUTE_ARRAY_SIZE(differences_from_0x4E00), ranges + CUTE_ARRAY_SIZE(base_ranges));
	}
	codepoint_set_t set = { count, ranges };
	return set;
}

codepoint_set_t cf_thai()
{
	static codepoint_range_t ranges[] = {
		{ 0x0020, 0x00FF }, // Basic Latin
		{ 0x2010, 0x205E }, // Punctuations
		{ 0x0E00, 0x0E7F }, // Thai
		{ 0xFFFD, 0xFFFD }  // Replacement character
	};
	codepoint_set_t set = { CUTE_ARRAY_SIZE(ranges), ranges };
	return set;
}

codepoint_set_t cf_vietnamese()
{
	static codepoint_range_t ranges[] = {
		{ 0x0020, 0x00FF }, // Basic Latin
		{ 0x0102, 0x0103 },
		{ 0x0110, 0x0111 },
		{ 0x0128, 0x0129 },
		{ 0x0168, 0x0169 },
		{ 0x01A0, 0x01A1 },
		{ 0x01AF, 0x01B0 },
		{ 0x1EA0, 0x1EF9 },
		{ 0xFFFD, 0xFFFD }  // Replacement character
	};
	codepoint_set_t set = { CUTE_ARRAY_SIZE(ranges), ranges };
	return set;
}

codepoint_set_t cf_cyrillic()
{
	static codepoint_range_t ranges[] = {
		{ 0x0020, 0x00FF }, // Basic Latin + Latin Supplement
		{ 0x0400, 0x052F }, // Cyrillic + Cyrillic Supplement
		{ 0x2DE0, 0x2DFF }, // Cyrillic Extended-A
		{ 0xA640, 0xA69F }, // Cyrillic Extended-B
		{ 0xFFFD, 0xFFFD }  // Replacement character
	};
	codepoint_set_t set = { CUTE_ARRAY_SIZE(ranges), ranges };
	return set;
}

font_t cf_make_font_mem(void* data, int size, result_t* result_out)
{
	font_internal_t* font = (font_internal_t*)CUTE_NEW(font_internal_t);
	font->file_data = (uint8_t*)data;
	if (!stbtt_InitFont(&font->info, font->file_data, stbtt_GetFontOffsetForIndex(font->file_data, 0))) {
		CUTE_FREE(data);
		CUTE_FREE(font);
		if (result_out) *result_out = result_failure("Failed to parse ttf file with stb_truetype.h.");
		return { ~0ULL };
	}
	font_t result = { cf_app->font_id_gen++ };
	font_internal_t** font_ptr = cf_app->fonts.insert(result.id);
	CUTE_ASSERT(font_ptr);
	*font_ptr = font;
	if (result_out) *result_out = result_success();
	return result;
}

font_t cf_make_font(const char* path, result_t* result_out)
{
	void* data;
	size_t size;
	result_t err = fs_read_entire_file_to_memory(path, &data, &size);
	if (is_error(err)) {
		CUTE_FREE(data);
		if (result_out) *result_out = err;
		return { ~0ULL };
	}
	font_t font = cf_make_font_mem(data, (int)size, result_out);
	return font;
}

void cf_destroy_font(font_t font_handle)
{
	font_internal_t* font = cf_app->fonts.get(font_handle.id);
	// TODO.
}

result_t cf_font_add_codepoints(font_t font_handle, codepoint_set_t set)
{
	font_internal_t* font = cf_app->fonts.get(font_handle.id);
	if (!font) return result_failure("Invalid font, did you forget to call `cf_make_font`?");
	for (int i = 0; i < set.count; ++i) {
		font->ranges.add(set.ranges[i]);
	}
	return result_success();
}

// Useful debugging tool for viewing the atlas.
static void s_save(const char* path, uint8_t* pixels, int w, int h)
{
	cp_image_t img;
	img.w = w;
	img.h = h;
	img.pix = (cp_pixel_t*)CUTE_ALLOC(sizeof(cp_pixel_t) * w * h);
	for (int i = 0; i < w * h; ++i) {
		cp_pixel_t pix;
		pix.r = pix.g = pix.b = pixels[i];
		pix.a = 255;
		img.pix[i] = pix;
	}
	cp_save_png(path, &img);
	CUTE_FREE(img.pix);
}

result_t cf_font_build(font_t font_handle, float size)
{
	font_internal_t* font = cf_app->fonts.get(font_handle.id);
	if (!font) return result_failure("Invalid font, did you forget to call `cf_make_font`?");
	// TODO - Delete old font data.
	// TODO - Oversampling.
	font->size = size;
	font->scale = stbtt_ScaleForPixelHeight(&font->info, size);
	int ascent, descent, line_gap;
	stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);
	font->ascent = ascent * font->scale;
	font->descent = descent * font->scale;
	font->line_gap = line_gap * font->scale;
	font->line_height = font->ascent - font->descent + font->line_gap;
	font->height = font->ascent - font->descent;
	font->size = size;

	// Load each glyph's data from the font.
	array<int> glyph_indices;
	for (int i = 0; i < font->ranges.size(); ++i) {
		int lo = font->ranges[i].lo;
		int hi = font->ranges[i].hi;
		while (lo < hi) {
			int codepoint = ++lo;
			int glyph_index = stbtt_FindGlyphIndex(&font->info, codepoint);
			if (!glyph_index) {
				// This glyph wasn't found in the font at all.
				continue;
			}
			if (font->glyphs.has(codepoint)) {
				// This glyph is already accounted for.
				continue;
			}
			int xadvance, lsb, x0, y0, x1, y1;
			stbtt_GetGlyphHMetrics(&font->info, glyph_index, &xadvance, &lsb);
			stbtt_GetGlyphBitmapBox(&font->info, glyph_index, font->scale, font->scale, &x0, &y0, &x1, &y1);
			int w = x1 - x0;
			int h = y1 - y0;
			glyph_t glyph;
			glyph.u = glyph.v = V2(0,0);
			glyph.w = w;
			glyph.h = h;
			glyph.offset = V2(lsb * font->scale, (float)y0);
			glyph.xadvance = xadvance * font->scale;
			glyph.visible = w > 0 && h > 0 && stbtt_IsGlyphEmpty(&font->info, glyph_index) == 0;
			glyph_indices.add(glyph_index);
			font->glyphs.insert(codepoint, glyph);
		}
	}

	// Gather up all rectangles for each glyph.
	int pad = 1;
	int rect_count = font->glyphs.count();
	array<stbrp_rect> rects;
	rects.ensure_count(rect_count);
	CUTE_MEMSET(rects.data(), 0, sizeof(stbrp_rect) * rect_count);
	glyph_t* glyphs = font->glyphs.items();
	for (int i = 0; i < rect_count; ++i) {
		rects[i].w = glyphs[i].w + pad;
		rects[i].h = glyphs[i].h + pad;
		rects[i].id = i;
	}

	// Pack all rectangles tightly together.
	int atlas_width = 1024;
	int atlas_height_max = 1024 * 32;
	stbrp_context stbrp;
	array<stbrp_node> nodes;
	nodes.ensure_count(atlas_height_max - pad);
	stbrp_init_target(&stbrp, atlas_width - pad, atlas_height_max - pad, nodes.data(), nodes.count());
	stbrp_pack_rects(&stbrp, rects.data(), rect_count);

	// Find the texture atlas's packed height.
	int packed_height = 0;
	for (int i = 0; i < rect_count; ++i) {
		packed_height = max(packed_height, rects[i].y + rects[i].h);
	}
	int atlas_height = fit_power_of_two(packed_height);

	// Calculate glyph UV's.
	// Render all glyphs acoording to their rectangle into a single texture atlas.
	uint8_t* pixels = (uint8_t*)CUTE_CALLOC(atlas_width * atlas_height);
	float iw = 1.0f / (float)atlas_width;
	float ih = 1.0f / (float)atlas_height;
	for (int i = 0; i < rect_count; ++i) {
		int x = rects[i].x + pad;
		int y = rects[i].y + pad;
		int w = rects[i].w - pad;
		int h = rects[i].h - pad;
		glyphs[i].u.x = x * iw;
		glyphs[i].v.x = y * ih;
		glyphs[i].u.y = (x + w) * iw;
		glyphs[i].v.y = (y + h) * ih;
		stbtt_MakeGlyphBitmap(&font->info, pixels + y * atlas_width + x, w, h, atlas_width, font->scale, font->scale, glyph_indices[i]);
	}
	font->pixels = pixels;
	//s_save("out.png", pixels, atlas_width, atlas_height);

	// TODO - Hook up to batch API automagically.

	return result_failure("Not finished implementing this.");
}
