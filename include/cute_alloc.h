/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#include "cute_defines.h"

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

/**
 * @function cf_aligned_alloc
 * @category allocator
 * @brief    Allocates a block of memory aligned along a byte boundary.
 * @param    size          The size of the allocation.
 * @param    alignment     An alignment boundary, must be a power of two.
 * @return   Returns an aligned pointer of `size` bytes.
 * @remarks  Aligned allocation is mostly useful as a performance optimization, or for SIMD operations that require byte alignments.
 * @related  cf_aligned_free
 */
CUTE_API void* CUTE_CALL cf_aligned_alloc(size_t size, int alignment);
CUTE_API void CUTE_CALL cf_aligned_free(void* p);

//--------------------------------------------------------------------------------------------------
// Arena allocator.

/**
 * @struct   CF_Arena
 * @category allocator
 * @brief    A simple way to allocate memory without calling `malloc` too often.
 * @remarks  Individual allocations cannot be free'd, instead the entire allocator can reset.
 * @related  cf_arena_init cf_arena_alloc cf_arena_reset
 */
typedef struct CF_Arena
{
	int alignment;
	int block_size;
	char* ptr;
	char* end;
	char** blocks;
} CF_Arena;
// @end

/**
 * @function cf_arena_init
 * @category allocator
 * @brief    Initializes an arena for later allocations.
 * @param    arena         The arena to initialize.
 * @param    alignment     An alignment boundary, must be a power of two.
 * @param    block_size    The default size of each internal call to `malloc` to form pages to further allocate from.
 * @related  cf_arena_alloc cf_arena_reset
 */
CUTE_API void CUTE_CALL cf_arena_init(CF_Arena* arena, int alignment, int block_size);

/**
 * @function cf_arena_alloc
 * @category allocator
 * @brief    Allocates a block of memory aligned along a byte boundary.
 * @param    arena         The arena to allocate from.
 * @param    size          The size of the allocation, it can be larger than `block_size` from `cf_arena_init`.
 * @return   Returns an aligned pointer of `size` bytes.
 * @related  cf_arena_init cf_arena_reset
 */
CUTE_API void* CUTE_CALL cf_arena_alloc(CF_Arena* arena, size_t size);

/**
 * @function cf_arena_reset
 * @category allocator
 * @brief    Free's up all resources used by the allocator and places it back into an initialized state.
 * @param    arena         The arena to reset.
 * @related  cf_arena_init cf_arena_alloc
 */
CUTE_API void CUTE_CALL cf_arena_reset(CF_Arena* arena);

//--------------------------------------------------------------------------------------------------
// Memory pool allocator.

typedef struct CF_MemoryPool CF_MemoryPool;

/**
 * @function cf_make_memory_pool
 * @category allocator
 * @brief    Creates a memory pool.
 * @param    element_size   The size of each allocation.
 * @param    element_count  The number of elements in the internal pool.
 * @param    alignment      An alignment boundary, must be a power of two.
 * @return   Returns a memory pool pointer.
 * @related  cf_destroy_memory_pool cf_memory_pool_alloc cf_memory_pool_try_alloc cf_memory_pool_free
 */
CUTE_API CF_MemoryPool* CUTE_CALL cf_make_memory_pool(int element_size, int element_count, int alignment);

/**
 * @function cf_destroy_memory_pool
 * @category allocator
 * @brief    Destroys a memory pool.
 * @param    pool           The pool to destroy.
 * @remarks  Does not clean up any allocations that overflowed to `malloc` backup. See `cf_memory_pool_alloc` for more details.
 * @related  cf_make_memory_pool cf_memory_pool_alloc cf_memory_pool_try_alloc cf_memory_pool_free
 */
CUTE_API void CUTE_CALL cf_destroy_memory_pool(CF_MemoryPool* pool);

/**
 * @function cf_memory_pool_alloc
 * @category allocator
 * @brief    Allocates a chunk of memory from the pool. The allocation size was determined by `element_size` in `cf_make_memory_pool`.
 * @param    pool           The pool.
 * @return   Returns an aligned pointer of `size` bytes.
 * @remarks  Attempts to allocate from the internal pool. If the pool is empty a call to `malloc` is made as a backup. All
 *           backup allocations are not tracked anywhere, so you must call `cf_memory_pool_free` on each allocation to be
 *           sure they all properly cleaned up.
 * @related  cf_make_memory_pool cf_destroy_memory_pool cf_memory_pool_try_alloc cf_memory_pool_free
 */
CUTE_API void* CUTE_CALL cf_memory_pool_alloc(CF_MemoryPool* pool);

/**
 * @function cf_memory_pool_try_alloc
 * @category allocator
 * @brief    Allocates a chunk of memory from the pool. The allocation size was determined by `element_size` in `cf_make_memory_pool`.
 * @param    pool           The pool.
 * @return   Returns an aligned pointer of `size` bytes.
 * @remarks  Does not return an allocation if the internal pool is full, and will instead return `NULL` in this case. See
 *           `cf_memory_pool_alloc` for more details about overflowing the pool to use `malloc` as a backup.
 * @related  cf_make_memory_pool cf_destroy_memory_pool cf_memory_pool_alloc cf_memory_pool_free
 */
CUTE_API void* CUTE_CALL cf_memory_pool_try_alloc(CF_MemoryPool* pool);

/**
 * @function cf_memory_pool_free
 * @category allocator
 * @brief    Frees an allocation made by `cf_memory_pool_alloc` or `cf_memory_pool_try_alloc`.
 * @param    pool           The pool.
 * @param    element        The pointer to deallocate.
 * @related  cf_make_memory_pool cf_destroy_memory_pool cf_memory_pool_alloc cf_memory_pool_try_alloc
 */
CUTE_API void CUTE_CALL cf_memory_pool_free(CF_MemoryPool* pool, void* element);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
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
CUTE_INLINE void* memory_pool_try_alloc(MemoryPool* pool) { return cf_memory_pool_try_alloc(pool); }
CUTE_INLINE void memory_pool_free(MemoryPool* pool, void* element) { return cf_memory_pool_free(pool, element); }

}

#endif // CUTE_CPP

#endif // CUTE_ALLOC_H
