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

#ifndef CUTE_STRING_H
#define CUTE_STRING_H

#include "cute_defines.h"

#ifdef  CUTE_CPP

#include "cute_strpool.h"

/*
	Implements a *single-threaded* string-interning system where each string on the stack
	is represented by a `uint64_t`, and internally ref-counts inside of a global string-
	interning system stored statically.

	There is no special support for string operations in a multi-threaded scenario. Simply
	make sure there is only one pool in a specific thread, if you really want to use cf_string_t
	between threads.
*/

typedef struct cf_string_t
{
	CUTE_API cf_string_t();
	CUTE_API cf_string_t(char* str);
	CUTE_API cf_string_t(const char* str);
	CUTE_API cf_string_t(const char* begin, const char* end);
	CUTE_API cf_string_t(const cf_string_t& other);
	CUTE_API cf_string_t(cf_strpool_id id);
	CUTE_API ~cf_string_t();

	CUTE_API size_t len() const;
	CUTE_API const char* c_str() const;

	CUTE_API cf_string_t& operator=(const cf_string_t& rhs);
	CUTE_API bool operator==(const cf_string_t& rhs) const;
	CUTE_API bool operator!=(const cf_string_t& rhs) const;
	CUTE_API char operator[](const int i) const;

	CUTE_API void incref();
	CUTE_API void decref();

	CUTE_API bool is_valid() const;

	cf_strpool_id id;
} cf_string_t;

CUTE_API void cf_string_defrag_static_pool();
CUTE_API void cf_string_nuke_static_pool();


namespace cute
{
using string_t = cf_string_t;

CUTE_INLINE void string_defrag_static_pool() { cf_string_defrag_static_pool(); }
CUTE_INLINE void string_nuke_static_pool() { cf_string_nuke_static_pool(); }

}

#endif //  CUTE_CPP

#endif // CUTE_STRING_H
