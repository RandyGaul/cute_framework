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

struct cf_strpool_t
{
	strpool_t inst;
	void* mem_ctx;
};

cf_strpool_t* cf_make_strpool(void* user_allocator_context)
{
	cf_strpool_t* pool = (cf_strpool_t*)CUTE_ALLOC(sizeof(cf_strpool_t), user_allocator_context);
	strpool_config_t strpool_config = strpool_default_config;
	strpool_config.memctx = user_allocator_context;
	strpool_init(&pool->inst, &strpool_config);
	pool->mem_ctx = user_allocator_context;
	return pool;
}

void cf_destroy_strpool(cf_strpool_t* pool)
{
	strpool_term(&pool->inst);
	CUTE_FREE(pool, pool->mem_ctx);
}

cf_strpool_id cf_strpool_inject(cf_strpool_t* pool, const char* string, int length)
{
	return { strpool_inject(&pool->inst, string, length) };
}

cf_strpool_id cf_strpool_inject(cf_strpool_t* pool, const char* string)
{
	return { strpool_inject(&pool->inst, string, (int)CUTE_STRLEN(string)) };
}

void cf_strpool_discard(cf_strpool_t* pool, cf_strpool_id id)
{
	strpool_discard(&pool->inst, id.val);
}

void cf_strpool_defrag(cf_strpool_t* pool)
{
	strpool_defrag(&pool->inst);
}

int cf_strpool_incref(cf_strpool_t* pool, cf_strpool_id id)
{
	return strpool_incref(&pool->inst, id.val);
}

int cf_strpool_decref(cf_strpool_t* pool, cf_strpool_id id)
{
	return strpool_decref(&pool->inst, id.val);
}

int cf_strpool_getref(cf_strpool_t* pool, cf_strpool_id id)
{
	return strpool_getref(&pool->inst, id.val);
}

bool cf_strpool_isvalid(const cf_strpool_t* pool, cf_strpool_id id)
{
	return strpool_isvalid(&pool->inst, id.val);
}

const char* cf_strpool_cstr(const cf_strpool_t* pool, cf_strpool_id id)
{
	return strpool_cstr(&pool->inst, id.val);
}

size_t cf_strpool_length(const cf_strpool_t* pool, cf_strpool_id id)
{
	return (size_t)strpool_length(&pool->inst, id.val);
}

}