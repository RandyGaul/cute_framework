/*
	Cute Framework
	Copyright (C) 2021 Randy Gaul https://randygaul.net

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

#ifndef CUTE_STRPOOL_H
#define CUTE_STRPOOL_H

#include <cute_defines.h>

namespace cute
{

struct strpool_t;
using strpool_id = uint64_t;

strpool_t* create_strpool(void* user_allocator_context = NULL);
void destroy_strpool(strpool_t* pool);

strpool_id strpool_inject(strpool_t* pool, const char* string, int length);
void strpool_discard(strpool_t* pool, strpool_id id);

void strpool_defrag(strpool_t* pool);

int strpool_incref(strpool_t* pool, strpool_id id);
int strpool_decref(strpool_t* pool, strpool_id id);
int strpool_getref(strpool_t* pool, strpool_id id);

bool strpool_isvalid(const strpool_t* pool, strpool_id id);

const char* strpool_cstr(const strpool_t* pool, strpool_id id);
size_t strpool_length(const strpool_t* pool, strpool_id id);

}

#endif // CUTE_STRPOOL_H
