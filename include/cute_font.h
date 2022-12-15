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

CUTE_API void CUTE_CALL cf_make_font(const char* path, const char* font_name, CF_Result* result_out);
CUTE_API void CUTE_CALL cf_make_font_mem(void* data, int size, const char* font_name, CF_Result* result_out);
CUTE_API void CUTE_CALL cf_destroy_font(const char* font_name);
CUTE_API void CUTE_CALL cf_font_add_backup_codepoints(const char* font_name, int* codepoints, int count);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

CUTE_INLINE void make_font(const char* path, const char* font_name, Result* result_out = NULL) { return cf_make_font(path, font_name, result_out); }
CUTE_INLINE void make_font_mem(void* data, int size, const char* font_name, Result* result_out = NULL) { return cf_make_font_mem(data, size, font_name, result_out); }
CUTE_INLINE void destroy_font(const char* font_name) { cf_destroy_font(font_name); }
CUTE_INLINE void font_add_backup_codepoints(const char* font_name, int* codepoints, int count) { cf_font_add_backup_codepoints(font_name, codepoints, count); }

}

#endif // CUTE_CPP

#endif // CUTE_FONT_H
