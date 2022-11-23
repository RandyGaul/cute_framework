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

#include "cute_defines.h"
#include "cute_result.h"
#include "cute_math.h"
#include "cute_color.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct cf_codepoint_range_t
{
	int lo;
	int hi;
} cf_codepoint_range_t;

typedef struct cf_codepoint_set_t
{
	int count;
	const cf_codepoint_range_t* ranges;
} cf_codepoint_set_t;

CUTE_API cf_codepoint_set_t CUTE_CALL cf_ascii_latin();
CUTE_API cf_codepoint_set_t CUTE_CALL cf_greek();
CUTE_API cf_codepoint_set_t CUTE_CALL cf_korean();
CUTE_API cf_codepoint_set_t CUTE_CALL cf_chinese_full();
CUTE_API cf_codepoint_set_t CUTE_CALL cf_chinese_simplified_common();
CUTE_API cf_codepoint_set_t CUTE_CALL cf_japanese();
CUTE_API cf_codepoint_set_t CUTE_CALL cf_thai();
CUTE_API cf_codepoint_set_t CUTE_CALL cf_vietnamese();
CUTE_API cf_codepoint_set_t CUTE_CALL cf_cyrillic();

typedef struct cf_font_t { uint64_t id; } cf_font_t;

CUTE_API cf_font_t CUTE_CALL cf_make_font(const char* path, cf_result_t* result_out);
CUTE_API cf_font_t CUTE_CALL cf_make_font_mem(void* data, int size, cf_result_t* result_out);
CUTE_API void CUTE_CALL cf_destroy_font(cf_font_t font);
CUTE_API cf_result_t CUTE_CALL cf_font_add_codepoints(cf_font_t font, cf_codepoint_set_t set);
CUTE_API cf_result_t CUTE_CALL cf_font_build(cf_font_t font, float size);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using codepoint_range_t = cf_codepoint_range_t;
using codepoint_set_t = cf_codepoint_set_t;
using font_t = cf_font_t;

CUTE_INLINE codepoint_set_t ascii_latin() { return cf_ascii_latin(); }
CUTE_INLINE codepoint_set_t greek() { return cf_greek(); }
CUTE_INLINE codepoint_set_t korean() { return cf_korean(); }
CUTE_INLINE codepoint_set_t chinese_full() { return cf_chinese_full(); }
CUTE_INLINE codepoint_set_t chinese_simplified_common() { return cf_chinese_simplified_common(); }
CUTE_INLINE codepoint_set_t japanese() { return cf_japanese(); }
CUTE_INLINE codepoint_set_t thai() { return cf_thai(); }
CUTE_INLINE codepoint_set_t vietnamese() { return cf_vietnamese(); }
CUTE_INLINE codepoint_set_t cyrillic() { return cf_cyrillic(); }

CUTE_INLINE font_t CUTE_CALL make_font(const char* path, result_t* result_out = NULL) { return cf_make_font(path, result_out); }
CUTE_INLINE font_t CUTE_CALL make_font_mem(void* data, int size, result_t* result_out = NULL) { return cf_make_font_mem(data, size, result_out); }
CUTE_INLINE void CUTE_CALL destroy_font(font_t font) { cf_destroy_font(font); }
CUTE_INLINE result_t CUTE_CALL font_add_codepoints(font_t font, codepoint_set_t set) { return cf_font_add_codepoints(font, set); }
CUTE_INLINE result_t CUTE_CALL font_build(font_t font, float size) { return cf_font_build(font, size); }

}

#endif // CUTE_CPP
