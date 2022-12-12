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
#include "cute_color.h"

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

typedef struct CF_Font { uint64_t id; } CF_Font;

CUTE_API CF_Font CUTE_CALL cf_make_font(const char* path, CF_Result* result_out);
CUTE_API CF_Font CUTE_CALL cf_make_font_mem(void* data, int size, CF_Result* result_out);
CUTE_API void CUTE_CALL cf_destroy_font(CF_Font font);
CUTE_API CF_Result CUTE_CALL cf_font_add_codepoints(CF_Font font, CF_CodepointSet set);
CUTE_API CF_Result CUTE_CALL cf_font_build(CF_Font font, float size);
CUTE_API void CUTE_CALL cf_font_missing_codepoints(CF_Font, int** missing_codepoints, int* count);
CUTE_API int CUTE_CALL cf_font_get_kern(CF_Font, int codepoint0, int codepoint1);

struct CF_Glyph
{
	uint64_t image_id;
	CF_V2 u, v;
	CF_V2 q0, q1;
	int w, h;
	float xadvance;
	bool visible;
};

CUTE_API CF_Glyph CUTE_CALL cf_font_get_glyph(CF_Font font, int codepoint);

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
using Font = CF_Font;
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

CUTE_INLINE Font make_font(const char* path, Result* result_out = NULL) { return cf_make_font(path, result_out); }
CUTE_INLINE Font make_font_mem(void* data, int size, Result* result_out = NULL) { return cf_make_font_mem(data, size, result_out); }
CUTE_INLINE void destroy_font(Font font) { cf_destroy_font(font); }
CUTE_INLINE CF_Result font_add_codepoints(Font font, CodepointSet set) { return cf_font_add_codepoints(font, set); }
CUTE_INLINE Result font_build(Font font, float size) { return cf_font_build(font, size); }
CUTE_INLINE void font_missing_codepoints(Font font, int** missing_codepoints, int* count) { return cf_font_missing_codepoints(font, missing_codepoints, count); }

}

#endif // CUTE_CPP

#endif // CUTE_FONT_H
