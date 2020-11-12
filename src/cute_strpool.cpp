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

#include <cute_strpool.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>

#ifndef STRPOOL_IMPLEMENTATION
#	define STRPOOL_IMPLEMENTATION
#	define STRPOOL_MALLOC(ctx, size) CUTE_ALLOC(size, ctx)
#	define STRPOOL_FREE(ctx, ptr) CUTE_FREE(ptr, ctx)
#endif
#include <mattiasgustavsson/strpool.h>

namespace cute
{

struct strpool_t
{
	::strpool_t inst;
	void* mem_ctx;
};

strpool_t* make_strpool(void* user_allocator_context)
{
	strpool_t* pool = (strpool_t*)CUTE_ALLOC(sizeof(strpool_t), user_allocator_context);
	strpool_config_t strpool_config = strpool_default_config;
	strpool_config.memctx = user_allocator_context;
	strpool_init(&pool->inst, &strpool_config);
	pool->mem_ctx = user_allocator_context;
	return pool;
}

void destroy_strpool(strpool_t* pool)
{
	strpool_term(&pool->inst);
	CUTE_FREE(pool, pool->mem_ctx);
}

strpool_id strpool_inject(strpool_t* pool, const char* string, int length)
{
	return ::strpool_inject(&pool->inst, string, length);
}

strpool_id strpool_inject(strpool_t* pool, const char* string)
{
	return ::strpool_inject(&pool->inst, string, CUTE_STRLEN(string));
}

void strpool_discard(strpool_t* pool, strpool_id id)
{
	::strpool_discard(&pool->inst, id);
}

void strpool_defrag(strpool_t* pool)
{
	::strpool_defrag(&pool->inst);
}

int strpool_incref(strpool_t* pool, strpool_id id)
{
	return ::strpool_incref(&pool->inst, id);
}

int strpool_decref(strpool_t* pool, strpool_id id)
{
	return ::strpool_decref(&pool->inst, id);
}

int strpool_getref(strpool_t* pool, strpool_id id)
{
	return ::strpool_getref(&pool->inst, id);
}

bool strpool_isvalid(const strpool_t* pool, strpool_id id)
{
	return ::strpool_isvalid(&pool->inst, id);
}

const char* strpool_cstr(const strpool_t* pool, strpool_id id)
{
	return ::strpool_cstr(&pool->inst, id);
}

size_t strpool_length(const strpool_t* pool, strpool_id id)
{
	return (size_t)::strpool_length(&pool->inst, id);
}

}