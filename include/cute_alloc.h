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

#ifndef CUTE_ALLOC_H
#define CUTE_ALLOC_H

#include "cute_defines.h" // for #define CUTE_CPP

#if !defined(CUTE_ALLOC) && !defined(CUTE_FREE)
#	ifdef _MSC_VER // Leak checking for debug Windows builds.
#		define _CRTDBG_MAPALLOC
#		define _CRTDBG_MAP_ALLOC
#		include <crtdbg.h>
#	endif
#	include <stdlib.h>
#	define CUTE_CALLOC(size) calloc(size, 1)
#	define CUTE_ALLOC(size) malloc(size)
#	define CUTE_FREE(ptr) free(ptr)
#	define CUTE_REALLOC(ptr, size) realloc(ptr, size)
#endif

//--------------------------------------------------------------------------------------------------
// Overload operator new ourselves.
// This avoids including thousands of lines of code in <new>, and also lets us hook up our own
// custom allocators without too much hassle.

#ifdef CUTE_CPP
	#ifdef _MSC_VER
	#	pragma warning(disable:4291)
	#endif

	enum CF_DummyEnum { CF_DUMMY_ENUM };
	inline void* operator new(size_t, CF_DummyEnum, void* ptr) { return ptr; }
	#define CUTE_PLACEMENT_NEW(ptr) new(CF_DUMMY_ENUM, ptr)
	#define CUTE_NEW(T) new(CF_DUMMY_ENUM, CUTE_ALLOC(sizeof(T))) T
#endif // CUTE_CPP

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// Aligned allocation.
// Mainly useful for optimization purposes, such as SIMD alignment.

CUTE_API void* CUTE_CALL cf_aligned_alloc(size_t size, int alignment);
CUTE_API void CUTE_CALL cf_aligned_free(void* p);

//--------------------------------------------------------------------------------------------------
// Arena allocator.
// Fetches memory with reduced calls to `malloc`.
// Free all memory allocated with `cf_arena_reset`. You can not free inidividual allocations.

typedef struct CF_Arena
{
	int alignment;
	int block_size;
	char* ptr;
	char* end;
	char** blocks;
} CF_Arena;

CUTE_API void CUTE_CALL cf_arena_init(CF_Arena* arena, int alignment, int block_size);
CUTE_API void* CUTE_CALL cf_arena_alloc(CF_Arena* arena, size_t size);
CUTE_API void CUTE_CALL cf_arena_reset(CF_Arena* arena);

//--------------------------------------------------------------------------------------------------
// Memory pool allocator.

typedef struct CF_MemoryPool CF_MemoryPool;

/**
 * Constructs a new memory pool.
 * `element_size` is the fixed size each internal allocation will be.
 * `element_count` determins how big the internal pool will be.
 */
CUTE_API CF_MemoryPool* CUTE_CALL cf_make_memory_pool(int element_size, int element_count, int alignment);

/**
 * Destroys a memory pool previously created with `make_memory_pool`. Does not clean up any leftover
 * allocations from `cf_memory_pool_alloc` that overflowed to the `malloc` backup. See `cf_memory_pool_alloc`
 * for more details.
 */
CUTE_API void CUTE_CALL cf_destroy_memory_pool(CF_MemoryPool* pool);

/**
 * Returns a block of memory of `element_size` bytes. If the number of allocations in the pool exceeds
 * `element_count` then `malloc` is used as a fallback.
 */
CUTE_API void* CUTE_CALL cf_memory_pool_alloc(CF_MemoryPool* pool);

/**
 * The same as `cf_memory_pool_alloc` without the `malloc` fallback -- returns `NULL` if the memory pool
 * is all used up.
 */
CUTE_API void* CUTE_CALL CF_MemoryPoolry_alloc(CF_MemoryPool* pool);

/**
 * Frees an allocation previously acquired by `cf_memory_pool_alloc` or `CF_MemoryPoolry_alloc`.
 */
CUTE_API void CUTE_CALL cf_memory_pool_free(CF_MemoryPool* pool, void* element);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

CUTE_INLINE void* aligned_alloc(size_t size, int alignment) { return cf_aligned_alloc(size, alignment); }
CUTE_INLINE void aligned_free(void* ptr) { return cf_aligned_free(ptr); }

using Arena = CF_Arena;

CUTE_INLINE void arena_init(CF_Arena* arena, int alignment, int block_size) { cf_arena_init(arena, alignment, block_size); }
CUTE_INLINE void* arena_alloc(CF_Arena* arena, size_t size) { return cf_arena_alloc(arena, size); }
CUTE_INLINE void arena_reset(CF_Arena* arena) { return cf_arena_reset(arena); }

using MemoryPool = CF_MemoryPool;

CUTE_INLINE MemoryPool* make_memory_pool(int element_size, int element_count, int alignment) { return cf_make_memory_pool(element_size, element_count, alignment); }
CUTE_INLINE void destroy_memory_pool(MemoryPool* pool) { cf_destroy_memory_pool(pool); }
CUTE_INLINE void* memory_pool_alloc(MemoryPool* pool) { return cf_memory_pool_alloc(pool); }
CUTE_INLINE void* MemoryPoolry_alloc(MemoryPool* pool) { return CF_MemoryPoolry_alloc(pool); }
CUTE_INLINE void memory_pool_free(MemoryPool* pool, void* element) { return cf_memory_pool_free(pool, element); }

}

#endif // CUTE_CPP

#endif // CUTE_ALLOC_H
