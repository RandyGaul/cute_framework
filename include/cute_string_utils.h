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

#ifndef CUTE_STRING_UTIL_H
#define CUTE_STRING_UTIL_H

#include "cute_defines.h"

#ifdef  CUTE_CPP

#include "cute_string.h"
#include "cute_array.h"

#include <stdarg.h>

CUTE_API cf_string_t operator+(const cf_string_t& a, const cf_string_t& b);

CUTE_API int cf_to_int(const cf_string_t& x);
CUTE_API float cf_to_float(const cf_string_t& x);

CUTE_API cf_string_t cf_format(cf_string_t fmt, int n, ...);

CUTE_API cf_string_t cf_to_string(int x);
CUTE_API cf_string_t cf_to_string(uint64_t x);
CUTE_API cf_string_t cf_to_string(float x);
CUTE_API cf_string_t cf_to_string(bool x);
CUTE_API cf_string_t cf_to_string(const cf_array<char>& x);
CUTE_API cf_string_t cf_to_string(char x);
CUTE_API cf_array<char> cf_to_array(cf_string_t s);
CUTE_API cf_array<char> cf_to_array(const char* s);
CUTE_API cf_array<char> cf_to_array(const char* s, size_t sz);

CUTE_API void cf_string_utils_cleanup_static_memory();


namespace cute
{

CUTE_INLINE int to_int(const string_t& x) { return cf_to_int(x); }
CUTE_INLINE float to_float(const string_t& x) { return cf_to_float(x); }

CUTE_INLINE string_t cf_format(string_t fmt, int n, ...);

CUTE_INLINE string_t to_string(int x) { return cf_to_string(x); }
CUTE_INLINE string_t to_string(uint64_t x) { return cf_to_string(x); }
CUTE_INLINE string_t to_string(float x) { return cf_to_string(x); }
CUTE_INLINE string_t to_string(bool x) { return cf_to_string(x); }
CUTE_INLINE string_t to_string(const array<char>& x) { return cf_to_string(x); }
CUTE_INLINE string_t to_string(char x) { return cf_to_string(x); }
CUTE_INLINE array<char> to_array(string_t s) { return cf_to_array(s); }
CUTE_INLINE array<char> to_array(const char* s) { return cf_to_array(s); }
CUTE_INLINE array<char> to_array(const char* s, size_t sz) { return cf_to_array(s,sz); }

CUTE_INLINE void string_utils_cleanup_static_memory() { return cf_string_utils_cleanup_static_memory(); }

}

#endif //  CUTE_CPP

#endif // CUTE_STRING_UTIL_H