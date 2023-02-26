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

#include <cute_alloc.h>
#include <cute_c_runtime.h>
#include <cute_array.h>

void* cf_aligned_alloc(size_t size, int alignment)
{
	CUTE_ASSERT(alignment <= 256);
	void* p = CUTE_ALLOC(size + alignment);
	if (!p) return NULL;
	size_t offset = (size_t)p & (alignment - 1);
	p = CUTE_ALIGN_FORWARD_PTR((char*)p + 1, alignment);
	CUTE_ASSERT(!(((size_t)p) & (alignment - 1)));
	*((char*)p - 1) = (char)(alignment - offset);
	return p;
}

void cf_aligned_free(void* p)
{
	if (!p) return;
	size_t offset = (size_t)*((uint8_t*)p - 1);
	CUTE_FREE((char*)p - (offset & 0xFF));
}

//--------------------------------------------------------------------------------------------------

void cf_arena_init(CF_Arena* arena, int alignment, int block_size)
{
	CUTE_MEMSET(arena, 0, sizeof(*arena));
	arena->alignment = alignment;
	arena->block_size = block_size;
}

void* cf_arena_alloc(CF_Arena* arena, size_t size)
{
	CUTE_ASSERT((int)size < arena->block_size);
	if (size > (size_t)(arena->end - arena->ptr)) {
		arena->ptr = (char*)cf_aligned_alloc(arena->block_size, arena->alignment);
		arena->end = arena->ptr + arena->block_size;
		apush(arena->blocks, arena->ptr);
	}
	void* result = arena->ptr;
	arena->ptr = (char*)CUTE_ALIGN_FORWARD_PTR(arena->ptr + size, arena->alignment);
	CUTE_ASSERT(!(((size_t)(arena->ptr)) & (arena->alignment - 1)));
	CUTE_ASSERT(arena->ptr <= arena->end);
	return result;
}

void cf_arena_reset(CF_Arena* arena)
{
	if (arena->blocks) {
		for (int i = 0; i < alen(arena->blocks); ++i) {
			cf_aligned_free(arena->blocks[i]);
		}
		afree(arena->blocks);
	}
	arena->ptr = NULL;
	arena->end = NULL;
	arena->blocks = NULL;
}

//--------------------------------------------------------------------------------------------------

struct CF_MemoryPool
{
	int unaligned_element_size;
	int element_size;
	size_t arena_size;
	int alignment;
	uint8_t* arena;
	void* free_list;
	int overflow_count;
};

CF_MemoryPool* cf_make_memory_pool(int element_size, int element_count, int alignment)
{
	element_size = element_size > sizeof(void*) ? element_size : sizeof(void*);
	int unaligned_element_size = element_size;
	element_size = CUTE_ALIGN_FORWARD(element_size, alignment);
	size_t header_size = CUTE_ALIGN_FORWARD(sizeof(CF_MemoryPool), alignment);
	CF_MemoryPool* pool = (CF_MemoryPool*)cf_aligned_alloc(header_size + element_size * element_count, alignment);

	pool->unaligned_element_size = unaligned_element_size;
	pool->element_size = element_size;
	pool->arena_size = (size_t)element_size * element_count;
	pool->arena = (uint8_t*)((uintptr_t)pool + header_size);
	pool->free_list = pool->arena;
	pool->overflow_count = 0;

	for (int i = 0; i < element_count - 1; ++i) {
		void** element = (void**)(pool->arena + element_size * i);
		void* next = (void*)(pool->arena + element_size * (i + 1));
		*element = next;
	};

	void** last_element = (void**)(pool->arena + element_size * (element_count - 1));
	*last_element = NULL;

	return pool;
}

void cf_destroy_memory_pool(CF_MemoryPool* pool)
{
	if (pool->overflow_count) {
		// Attempted to destroy pool without freeing all overflow allocations.
		CUTE_ASSERT(pool->overflow_count == 0);
	}
	CUTE_FREE(pool);
}

void* cf_memory_pool_alloc(CF_MemoryPool* pool)
{
	void *mem = cf_memory_pool_try_alloc(pool);
	if (!mem) {
		mem = cf_aligned_alloc(pool->unaligned_element_size, pool->alignment);
		if (mem) {
			pool->overflow_count++;
		}
	}
	return mem;
}

void* cf_memory_pool_try_alloc(CF_MemoryPool* pool)
{
	if (pool->free_list) {
		void *mem = pool->free_list;
		pool->free_list = *((void**)pool->free_list);
		return mem;
	} else {
		return NULL;
	}
}

void cf_memory_pool_free(CF_MemoryPool* pool, void* element)
{
	int difference = (int)((uint8_t*)element - pool->arena);
	bool in_bounds = (void*)element >= pool->arena && difference <= (pool->arena_size - pool->element_size);
	if (pool->overflow_count && !in_bounds) {
		cf_aligned_free(element);
		pool->overflow_count--;
	} else if (in_bounds) {
		*(void**)element = pool->free_list;
		pool->free_list = element;
	} else {
		// Tried to free something that definitely didn't come from this pool.
		CUTE_ASSERT(false);
	}
}
