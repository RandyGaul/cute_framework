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

#include "cute_defines.h"

namespace cute
{

struct cf_strpool_t;
struct cf_strpool_id { uint64_t val; };

CUTE_API cf_strpool_t* CUTE_CALL cf_make_strpool(void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL cf_destroy_strpool(cf_strpool_t* pool);

CUTE_API cf_strpool_id CUTE_CALL cf_strpool_inject(cf_strpool_t* pool, const char* string, int length);
CUTE_API cf_strpool_id CUTE_CALL cf_strpool_inject(cf_strpool_t* pool, const char* string);
CUTE_API void CUTE_CALL cf_strpool_discard(cf_strpool_t* pool, cf_strpool_id id);

CUTE_API void CUTE_CALL cf_strpool_defrag(cf_strpool_t* pool);

CUTE_API int CUTE_CALL cf_strpool_incref(cf_strpool_t* pool, cf_strpool_id id);
CUTE_API int CUTE_CALL cf_strpool_decref(cf_strpool_t* pool, cf_strpool_id id);
CUTE_API int CUTE_CALL cf_strpool_getref(cf_strpool_t* pool, cf_strpool_id id);

CUTE_API bool CUTE_CALL cf_strpool_isvalid(const cf_strpool_t* pool, cf_strpool_id id);

CUTE_API const char* CUTE_CALL cf_strpool_cstr(const cf_strpool_t* pool, cf_strpool_id id);
CUTE_API size_t CUTE_CALL cf_strpool_length(const cf_strpool_t* pool, cf_strpool_id id);

}

#endif // CUTE_STRPOOL_H
