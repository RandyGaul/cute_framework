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

#ifndef CUTE_RESULT_H
#define CUTE_RESULT_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define CF_RESULT_DEFS \
	CF_ENUM(RESULT_SUCCESS, 0) \
	CF_ENUM(RESULT_ERROR, -1) \

enum
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_RESULT_DEFS
	#undef CF_ENUM
};

typedef struct cf_result_t
{
	int code;
	const char* details;
} cf_result_t;

CUTE_INLINE bool cf_is_error(cf_result_t result) { return result.code == CF_RESULT_ERROR; }

CUTE_INLINE cf_result_t cf_result_make(int code, const char* details) { cf_result_t result; result.code = code; result.details = details; return result; }
CUTE_INLINE cf_result_t cf_result_error(const char* details) { cf_result_t result; result.code = CF_RESULT_ERROR; result.details = details; return result; }
CUTE_INLINE cf_result_t cf_result_success() { cf_result_t result; result.code = CF_RESULT_SUCCESS; result.details = NULL; return result; }

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using result_t = cf_result_t;

enum : int
{
	#define CF_ENUM(K, V) K = V,
	CF_RESULT_DEFS
	#undef CF_ENUM
};

CUTE_INLINE bool is_error(result_t error) { return cf_is_error(error); }

CUTE_INLINE result_t result_make(int code, const char* details) { return cf_result_make(code, details); }
CUTE_INLINE result_t result_failure(const char* details) { return cf_result_error(details); }
CUTE_INLINE result_t result_success() { return cf_result_success(); }

}

#endif // CUTE_CPP

#endif // CUTE_RESULT_H
