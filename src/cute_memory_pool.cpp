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

#include <cute_memory_pool.h>
#include <cute_alloc.h>
#include <cute_error.h>
#include <cute_c_runtime.h>

namespace cute
{

struct cf_memory_pool_t
{
	int element_size;
	int arena_size;
	uint8_t* arena;
	void* free_list;
	int overflow_count;
	void* mem_ctx;
};

cf_memory_pool_t* cf_memory_pool_make(int element_size, int element_count, void* user_allocator_context)
{
	size_t stride = element_size > sizeof(void*) ? element_size : sizeof(void*);
	size_t arena_size = sizeof(cf_memory_pool_t) + stride * element_count;
	cf_memory_pool_t* pool = (cf_memory_pool_t*)CUTE_ALLOC(arena_size, user_allocator_context);

	pool->element_size = element_size;
	pool->arena_size = (int)(arena_size - sizeof(cf_memory_pool_t));
	pool->arena = (uint8_t*)(pool + 1);
	pool->free_list = pool->arena;
	pool->overflow_count = 0;

	for (int i = 0; i < element_count - 1; ++i)
	{
		void** element = (void**)(pool->arena + stride * i);
		void* next = (void*)(pool->arena + stride * (i + 1));
		*element = next;
	};

	void** last_element = (void**)(pool->arena + stride * (element_count - 1));
	*last_element = NULL;

	return pool;
}

void cf_memory_pool_destroy(cf_memory_pool_t* pool)
{
	if (pool->overflow_count) {
		//error_set("Attempted to destroy pool without freeing all overflow allocations.");
		CUTE_ASSERT(pool->overflow_count == 0);
	}
	CUTE_FREE(pool, pool->mem_ctx);
}

void* cf_memory_pool_alloc(cf_memory_pool_t* pool)
{
	void *mem = cf_memory_pool_try_alloc(pool);
	if (!mem) {
		mem = CUTE_ALLOC(pool->element_size, pool->mem_ctx);
		if (mem) {
			pool->overflow_count++;
		}
	}
	return mem;
}

void* cf_memory_pool_try_alloc(cf_memory_pool_t* pool)
{
	if (pool->free_list) {
		void *mem = pool->free_list;
		pool->free_list = *((void**)pool->free_list);
		return mem;
	} else {
		return NULL;
	}
}

void cf_memory_pool_free(cf_memory_pool_t* pool, void* element)
{
	int difference = (int)((uint8_t*)element - pool->arena);
	int in_bounds = difference < pool->arena_size;
	if (pool->overflow_count && !in_bounds) {
		CUTE_FREE(element, pool->mem_ctx);
		pool->overflow_count--;
	} else if (in_bounds) {
		*(void**)element = pool->free_list;
		pool->free_list = element;
	} else {
		//error_set("Pointer was outside of arena bounds, or a double free was detected, in `memory_pool_t`.");
		assert(0);
	}
}

}

