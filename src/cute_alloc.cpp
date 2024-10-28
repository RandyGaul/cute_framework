/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifdef _MSC_VER // Leak checking for debug Windows builds.
#	define _CRTDBG_MAPALLOC
#	define _CRTDBG_MAP_ALLOC
#	include <crtdbg.h>
#endif
#include <stdlib.h>

#include <cute_alloc.h>
#include <cute_c_runtime.h>
#include <cute_array.h>

#include <internal/cute_alloc_internal.h>

void* s_default_alloc(size_t size, void* udata)
{
	CF_UNUSED(udata);
	return malloc(size);
}

void s_default_free(void* ptr, void* udata)
{
	CF_UNUSED(udata);
	free(ptr);
}

void* s_default_calloc(size_t size, size_t count, void* udata)
{
	CF_UNUSED(udata);
	return calloc(size, count);
}

void* s_default_realloc(void* ptr, size_t size, void* udata)
{
	CF_UNUSED(udata);
	return realloc(ptr, size);
}

CF_Allocator s_default_allocator = {
	NULL,
	s_default_alloc,
	s_default_free,
	s_default_calloc,
	s_default_realloc
};

CF_GLOBAL CF_Allocator s_allocator = s_default_allocator;

void cf_allocator_override(CF_Allocator allocator)
{
	s_allocator = allocator;
}

void cf_allocator_restore_default()
{
	s_allocator = s_default_allocator;
}

void* cf_alloc(size_t size)
{
	return s_allocator.alloc_fn ? s_allocator.alloc_fn(size, NULL) : s_default_alloc(size, NULL);
}

void cf_free(void* ptr)
{
	s_allocator.free_fn ? s_allocator.free_fn(ptr, NULL) : s_default_free(ptr, NULL);
}

void* cf_calloc(size_t size, size_t count)
{
	return s_allocator.calloc_fn ? s_allocator.calloc_fn(size, count, NULL) : s_default_calloc(size, count, NULL);
}

void* cf_realloc(void* ptr, size_t size)
{
	return s_allocator.realloc_fn ? s_allocator.realloc_fn(ptr, size, NULL) : s_default_realloc(ptr, size, NULL);
}

//--------------------------------------------------------------------------------------------------

void* cf_aligned_alloc(size_t size, int alignment)
{
	CF_ASSERT(alignment <= 256);
	void* p = CF_ALLOC(size + alignment);
	if (!p) return NULL;
	size_t offset = (size_t)p & (alignment - 1);
	p = CF_ALIGN_FORWARD_PTR((char*)p + 1, alignment);
	CF_ASSERT(!(((size_t)p) & (alignment - 1)));
	*((char*)p - 1) = (char)(alignment - offset);
	return p;
}

void cf_aligned_free(void* p)
{
	if (!p) return;
	size_t offset = (size_t)*((uint8_t*)p - 1);
	CF_FREE((char*)p - (offset & 0xFF));
}

//--------------------------------------------------------------------------------------------------

CF_Arena cf_make_arena(int alignment, int block_size)
{
	CF_Arena arena;
	CF_MEMSET(&arena, 0, sizeof(arena));
	arena.alignment = alignment;
	arena.block_size = block_size;
	return arena;
}

void* cf_arena_alloc(CF_Arena* arena, int size)
{
	CF_ASSERT((int)size < arena->block_size);
	if (size > (int)(arena->end - arena->ptr)) {
		if (arena->block_index < asize(arena->blocks)) {
			arena->ptr = arena->blocks[arena->block_index];
		} else {
			arena->ptr = (char*)cf_aligned_alloc(arena->block_size, arena->alignment);
			apush(arena->blocks, arena->ptr);
		}
		arena->end = arena->ptr + arena->block_size;
		arena->block_index++;
	}
	void* result = arena->ptr;
	arena->ptr = (char*)CF_ALIGN_FORWARD_PTR(arena->ptr + size, arena->alignment);
	CF_ASSERT(!(((int)(uintptr_t)(arena->ptr)) & (arena->alignment - 1)));
	CF_ASSERT(arena->ptr <= arena->end);
	return result;
}

void cf_arena_free(CF_Arena* arena, int size)
{
	CF_ASSERT(arena->block_index > 0);
	char* aligned_ptr = (char*)CF_ALIGN_BACKWARD_PTR(arena->ptr - size, arena->alignment);
	if (aligned_ptr >= arena->blocks[arena->block_index - 1]) {
		arena->ptr = aligned_ptr;
	} else {
		int remaining_size = (int)(arena->ptr - arena->blocks[arena->block_index - 1]);
		size -= remaining_size;
		while (size > 0 && arena->block_index > 0) {
			arena->block_index--;
			CF_ASSERT(arena->block_index >= 0);
			arena->ptr = arena->blocks[arena->block_index];
			arena->end = arena->ptr + arena->block_size;
			if (size < arena->block_size) {
				arena->ptr = (char*)CF_ALIGN_BACKWARD_PTR(arena->end - size, arena->alignment);
				size = 0;
			} else {
				size -= arena->block_size;
			}
		}
		CF_ASSERT(size == 0);
	}
}

void cf_arena_reset(CF_Arena* arena)
{
	if (arena->blocks && asize(arena->blocks) > 0) {
		arena->ptr = arena->blocks[0];
		arena->end = arena->ptr + arena->block_size;
		arena->block_index = 1;
	} else {
		arena->ptr = NULL;
		arena->end = NULL;
		arena->block_index = 0;
	}
}

void cf_destroy_arena(CF_Arena* arena)
{
	if (arena->blocks) {
		for (int i = 0; i < asize(arena->blocks); ++i) {
			cf_aligned_free(arena->blocks[i]);
		}
		afree(arena->blocks);
	}
	arena->ptr = NULL;
	arena->end = NULL;
	arena->blocks = NULL;
	arena->block_index = 0;
}

//--------------------------------------------------------------------------------------------------

struct CF_MemoryBlock
{
	uint8_t* memory;
	CF_MemoryBlock* next;
};

struct CF_MemoryPool
{
	int element_size;
	size_t block_size;
	int alignment;
	void* free_list;
	CF_MemoryBlock* blocks;
	int element_count_per_block;
};

CF_MemoryPool* cf_make_memory_pool(int element_size, int element_count, int alignment)
{
	element_size = element_size > sizeof(void*) ? element_size : sizeof(void*);
	element_size = CF_ALIGN_FORWARD(element_size, alignment);
	size_t block_size = element_size * element_count;

	CF_MemoryPool* pool = (CF_MemoryPool*)cf_aligned_alloc(sizeof(CF_MemoryPool), alignment);
	pool->element_size = element_size;
	pool->block_size = block_size;
	pool->alignment = alignment;
	pool->free_list = NULL;
	pool->element_count_per_block = element_count;

	// Allocate the first block and initialize the free list.
	CF_MemoryBlock* block = (CF_MemoryBlock*)cf_aligned_alloc(sizeof(CF_MemoryBlock), alignment);
	block->memory = (uint8_t*)cf_aligned_alloc(block_size, alignment);
	block->next = NULL;
	pool->blocks = block;
	pool->free_list = block->memory;
	for (int i = 0; i < element_count - 1; ++i) {
		void** element = (void**)(block->memory + element_size * i);
		void* next = (void*)(block->memory + element_size * (i + 1));
		*element = next;
	}
	void** last_element = (void**)(block->memory + element_size * (element_count - 1));
	*last_element = NULL;

	return pool;
}

void cf_destroy_memory_pool(CF_MemoryPool* pool)
{
	CF_MemoryBlock* block = pool->blocks;
	while (block) {
		CF_MemoryBlock* next = block->next;
		cf_aligned_free(block->memory);
		cf_aligned_free(block);
		block = next;
	}
	cf_aligned_free(pool);
}

void* cf_memory_pool_alloc(CF_MemoryPool* pool)
{
	// Try to allocate from the free list first.
	if (pool->free_list) {
		void* mem = pool->free_list;
		pool->free_list = *(void**)pool->free_list;
		return mem;
	}

	// If no free elements, allocate a new block and initialize its free list.
	CF_MemoryBlock* new_block = (CF_MemoryBlock*)cf_aligned_alloc(sizeof(CF_MemoryBlock), pool->alignment);
	new_block->memory = (uint8_t*)cf_aligned_alloc(pool->block_size, pool->alignment);
	new_block->next = pool->blocks;
	pool->blocks = new_block;
	pool->free_list = new_block->memory;
	for (int i = 0; i < pool->element_count_per_block - 1; ++i) {
		void** element = (void**)(new_block->memory + pool->element_size * i);
		void* next = (void*)(new_block->memory + pool->element_size * (i + 1));
		*element = next;
	}
	void** last_element = (void**)(new_block->memory + pool->element_size * (pool->element_count_per_block - 1));
	*last_element = NULL;

	return cf_memory_pool_alloc(pool);
}

void cf_memory_pool_free(CF_MemoryPool* pool, void* element)
{
	*(void**)element = pool->free_list;
	pool->free_list = element;
}