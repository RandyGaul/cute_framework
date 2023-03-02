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

typedef struct CF_Result
{
	int code;
	const char* details;
} CF_Result;

CUTE_INLINE bool cf_is_error(CF_Result result) { return result.code == CF_RESULT_ERROR; }

CUTE_INLINE CF_Result cf_result_make(int code, const char* details) { CF_Result result; result.code = code; result.details = details; return result; }
CUTE_INLINE CF_Result cf_result_error(const char* details) { CF_Result result; result.code = CF_RESULT_ERROR; result.details = details; return result; }
CUTE_INLINE CF_Result cf_result_success() { CF_Result result; result.code = CF_RESULT_SUCCESS; result.details = NULL; return result; }

#define CF_MESSAGE_BOX_TYPE_DEFS \
	CF_ENUM(MESSAGE_BOX_TYPE_ERROR, 0) \
	CF_ENUM(MESSAGE_BOX_TYPE_WARNING, 1) \
	CF_ENUM(MESSAGE_BOX_TYPE_INFORMATION, 2) \

typedef enum CF_MessageBoxType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_MESSAGE_BOX_TYPE_DEFS
	#undef CF_ENUM
} CF_MessageBoxType;

CUTE_INLINE const char* cf_message_box_type_to_string(CF_MessageBoxType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_MESSAGE_BOX_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CUTE_API void CUTE_CALL cf_message_box(CF_MessageBoxType type, const char* title, const char* text);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using Result = CF_Result;

enum : int
{
	#define CF_ENUM(K, V) K = V,
	CF_RESULT_DEFS
	#undef CF_ENUM
};

using MessageBoxType = CF_MessageBoxType;
#define CF_ENUM(K, V) CUTE_INLINE constexpr MessageBoxType K = CF_##K;
CF_MESSAGE_BOX_TYPE_DEFS
#undef CF_ENUM

CUTE_INLINE const char* to_string(MessageBoxType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_MESSAGE_BOX_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CUTE_INLINE bool is_error(Result error) { return cf_is_error(error); }

CUTE_INLINE Result result_make(int code, const char* details) { return cf_result_make(code, details); }
CUTE_INLINE Result result_failure(const char* details) { return cf_result_error(details); }
CUTE_INLINE Result result_success() { return cf_result_success(); }
CUTE_INLINE void message_box(MessageBoxType type, const char* title, const char* text) { return cf_message_box(type, title, text); }

}

#endif // CUTE_CPP

#endif // CUTE_RESULT_H
