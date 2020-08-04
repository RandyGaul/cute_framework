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

#include <cute_defines.h>

/*
	Implements a *single-threaded* string-interning system where each string on the stack
	is represented by a `uint64_t`, and internally ref-counts inside of a global string-
	interning system stored statically.

	There is no special support for string operations in a multi-threaded scenario.
*/

namespace cute
{

struct string_t
{
	explicit string_t();
	explicit string_t(char* str);
	explicit string_t(const char* str);
	explicit string_t(const char* begin, const char* end);
	explicit string_t(void* null_pointer);
	string_t(const string_t& other);
	~string_t();

	int len() const;
	const char* c_str() const;

	string_t& operator=(const string_t& rhs);
	int operator==(const string_t& rhs) const;
	int operator!=(const string_t& rhs) const;

	uint64_t id;
};

void string_set_allocator_context(void* user_allocator_context);
void* string_get_allocator_context();
void string_defrag();
void string_nuke();

}

#endif // STRING_H
