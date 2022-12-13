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

#ifndef CUTE_FONT_H
#define CUTE_FONT_H

#include "cute_defines.h"
#include "cute_result.h"
#include "cute_math.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct CF_CodepointRange
{
	int lo;
	int hi;
} CF_CodepointRange;

typedef struct CF_CodepointSet
{
	int count;
	const CF_CodepointRange* ranges;
} CF_CodepointSet;

CUTE_API CF_CodepointSet CUTE_CALL cf_ascii_latin();
CUTE_API CF_CodepointSet CUTE_CALL cf_greek();
CUTE_API CF_CodepointSet CUTE_CALL cf_korean();
CUTE_API CF_CodepointSet CUTE_CALL cf_chinese_full();
CUTE_API CF_CodepointSet CUTE_CALL cf_chinese_simplified_common();
CUTE_API CF_CodepointSet CUTE_CALL cf_japanese();
CUTE_API CF_CodepointSet CUTE_CALL cf_thai();
CUTE_API CF_CodepointSet CUTE_CALL cf_vietnamese();
CUTE_API CF_CodepointSet CUTE_CALL cf_cyrillic();

CUTE_API void CUTE_CALL cf_make_font(const char* path, const char* font_name, CF_Result* result_out);
CUTE_API void CUTE_CALL cf_make_font_mem(void* data, int size, const char* font_name, CF_Result* result_out);
CUTE_API void CUTE_CALL cf_destroy_font(const char* font_name);
CUTE_API CF_Result CUTE_CALL cf_font_add_codepoints(const char* font_name, CF_CodepointSet set);
CUTE_API CF_Result CUTE_CALL cf_font_build(const char* font_name, float size);
CUTE_API void CUTE_CALL cf_font_missing_codepoints(const char* font_name, int** missing_codepoints, int* count);
CUTE_API float CUTE_CALL cf_font_get_kern(const char* font_name, float font_size, int codepoint0, int codepoint1);

struct CF_Glyph
{
	uint64_t image_id;
	CF_V2 u, v;
	CF_V2 q0, q1;
	int w, h;
	float xadvance;
	bool visible;
};

CUTE_API CF_Glyph CUTE_CALL cf_font_get_glyph(const char* font_name, float font_size, int codepoint);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using CodepointRange = CF_CodepointRange;
using CodepointSet = CF_CodepointSet;
using Glyph = CF_Glyph;

CUTE_INLINE CodepointSet ascii_latin() { return cf_ascii_latin(); }
CUTE_INLINE CodepointSet greek() { return cf_greek(); }
CUTE_INLINE CodepointSet korean() { return cf_korean(); }
CUTE_INLINE CodepointSet chinese_full() { return cf_chinese_full(); }
CUTE_INLINE CodepointSet chinese_simplified_common() { return cf_chinese_simplified_common(); }
CUTE_INLINE CodepointSet japanese() { return cf_japanese(); }
CUTE_INLINE CodepointSet thai() { return cf_thai(); }
CUTE_INLINE CodepointSet vietnamese() { return cf_vietnamese(); }
CUTE_INLINE CodepointSet cyrillic() { return cf_cyrillic(); }

CUTE_INLINE void make_font(const char* path, const char* font_name, Result* result_out = NULL) { return cf_make_font(path, font_name, result_out); }
CUTE_INLINE void make_font_mem(void* data, int size, const char* font_name, Result* result_out = NULL) { return cf_make_font_mem(data, size, font_name, result_out); }
CUTE_INLINE void destroy_font(const char* font_name) { cf_destroy_font(font_name); }
CUTE_INLINE Result font_add_codepoints(const char* font_name, CodepointSet set) { return cf_font_add_codepoints(font_name, set); }
CUTE_INLINE Result font_build(const char* font_name, float size) { return cf_font_build(font_name, size); }
CUTE_INLINE void font_missing_codepoints(const char* font_name, int** missing_codepoints, int* count) { return cf_font_missing_codepoints(font_name, missing_codepoints, count); }
CUTE_INLINE float font_get_kern(const char* font_name, float font_size, int codepoint0, int codepoint1) { return cf_font_get_kern(font_name, font_size, codepoint0, codepoint1); }
CUTE_INLINE CF_Glyph font_get_glyph(const char* font_name, float font_size, int codepoint) { return cf_font_get_glyph(font_name, font_size, codepoint); }

}

#endif // CUTE_CPP

#endif // CUTE_FONT_H
