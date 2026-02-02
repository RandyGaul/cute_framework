/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_ALLOC_H
#define CF_ALLOC_H

#include "cute_defines.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Allocator
 * @category allocator
 * @brief    A simple way to allocate memory without calling `malloc` too often.
 * @remarks  Individual allocations cannot be free'd, instead the entire allocator can reset.
 * @related  CF_Allocator cf_allocator_override cf_allocator_restore_default cf_alloc cf_free cf_calloc cf_realloc
 */
typedef struct CF_Allocator
{
	/* @member Can be `NULL`. An optional parameter handed back to you in the other function pointers below. */
	void* udata;

	/* @member Your custom allocation function. */
	void* (*alloc_fn)(size_t size, void* udata);

	/* @member Your custom free function. */
	void (*free_fn)(void* ptr, void* udata);

	/* @member Your custom calloc function. Allocates memory that is cleared to zero. */
	void* (*calloc_fn)(size_t size, size_t count, void* udata);

	/* @member Your custom realloc function. Reallocates a pointer to a new size. */
	void* (*realloc_fn)(void* ptr, size_t size, void* udata);
} CF_Allocator;
// @end

/**
 * @function cf_allocator_override
 * @category allocator
 * @brief    Overrides the default allocator with a custom one.
 * @remarks  The default allocator simply calls malloc/free and friends. You may override this behavior by passing
 *           a `CF_Allocator` to this function. This lets you hook up your own custom allocator. Usually you only want
 *           to do this on certain platforms for performance optimizations, but is not a necessary thing to do for many games.
 * @related  CF_Allocator cf_allocator_override cf_allocator_restore_default cf_alloc cf_free cf_calloc cf_realloc
 */
CF_API void CF_CALL cf_allocator_override(CF_Allocator allocator);

/**
 * @function cf_allocator_restore_default
 * @category allocator
 * @brief    Restores the default allocator.
 * @remarks  The default allocator simply calls malloc/free and friends. You may override this behavior by passing
 *           a `CF_Allocator` to this function. This lets you hook up your own custom allocator. Usually you only want
 *           to do this on certain platforms for performance optimizations, but is not a necessary thing to do for many games.
 * @related  CF_Allocator cf_allocator_override cf_allocator_restore_default cf_alloc cf_free cf_calloc cf_realloc
 */
CF_API void CF_CALL cf_allocator_restore_default(void);

/**
 * @function cf_alloc
 * @category allocator
 * @brief    Allocates a block of memory of `size` bytes and returns it.
 * @related  CF_Allocator cf_allocator_override cf_allocator_restore_default cf_alloc cf_free cf_calloc cf_realloc
 */
CF_API void* CF_CALL cf_alloc(size_t size);

/**
 * @function cf_free
 * @category allocator
 * @brief    Frees a block of memory previously allocated by `cf_alloc`.
 * @related  CF_Allocator cf_allocator_override cf_allocator_restore_default cf_alloc cf_free cf_calloc cf_realloc
 */
CF_API void CF_CALL cf_free(void* ptr);

/**
 * @function cf_calloc
 * @category allocator
 * @brief    Allocates a block of memory `size * count` bytes in size.
 * @remarks  The memory returned is completely zero'd out. Generally this is more efficient than calling `cf_malloc` and
 *           then clearing the memory to zero yourself. Though, it's not a concern for most games.
 * @related  CF_Allocator cf_allocator_override cf_allocator_restore_default cf_alloc cf_free cf_calloc cf_realloc
 */
CF_API void* CF_CALL cf_calloc(size_t size, size_t count);

/**
 * @function cf_realloc
 * @category allocator
 * @brief    Reallocates a block of memory to a new size.
 * @remarks  You must reassign your old pointer! Generally this is more efficient than calling `cf_malloc`, `cf_free`, and
 *           `CF_MEMCPY` yourself. Though, this is not a concern for most games.
 * @related  CF_Allocator cf_allocator_override cf_allocator_restore_default cf_alloc cf_free cf_calloc cf_realloc
 */
CF_API void* CF_CALL cf_realloc(void* ptr, size_t size);

//--------------------------------------------------------------------------------------------------
// Overload operator new ourselves.
// This avoids including thousands of lines of code in <new>, and also lets us hook up our own
// custom allocators without too much hassle.

#ifdef CF_CPP
	#ifdef _MSC_VER
	#	pragma warning(disable:4291)
	#endif

	enum CF_DummyEnum { CF_DUMMY_ENUM };
	inline void* operator new(size_t, CF_DummyEnum, void* ptr) { return ptr; }
	#define CF_PLACEMENT_NEW(ptr) new(CF_DUMMY_ENUM, ptr)
	#define CF_NEW(...) new(CF_DUMMY_ENUM, cf_alloc(sizeof(__VA_ARGS__))) (__VA_ARGS__)
#endif // CF_CPP

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
CF_API void* CF_CALL cf_aligned_alloc(size_t size, int alignment);

/**
 * @function cf_aligned_free
 * @category allocator
 * @brief    Frees a block of memory previously allocated by `cf_aligned_alloc`.
 * @param    ptr           The memory to deallocate.
 * @remarks  Aligned allocation is mostly useful as a performance optimization, or for SIMD operations that require byte alignments.
 * @related  cf_aligned_alloc
 */
CF_API void CF_CALL cf_aligned_free(void* ptr);

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
	int block_index;
	/* dyna */ char** blocks;
} CF_Arena;
// @end

/**
 * @function cf_arena_init
 * @category allocator
 * @brief    Initializes an arena for later allocations.
 * @param    arena         The arena to initialize.
 * @param    alignment     An alignment boundary, must be a power of two.
 * @param    block_size    The default size of each internal call to `malloc` to form pages to further allocate from.
 * @related  cf_arena_init cf_arena_alloc cf_arena_reset cf_arena_free
 */
CF_API CF_Arena CF_CALL cf_make_arena(int alignment, int block_size);

/**
 * @function cf_arena_alloc
 * @category allocator
 * @brief    Allocates a block of memory aligned along a byte boundary.
 * @param    arena         The arena to allocate from.
 * @param    size          The size of the allocation, it cannot be larger than `block_size` from `cf_arena_init`.
 * @return   Returns an aligned pointer of `size` bytes.
 * @related  cf_arena_init cf_arena_alloc cf_arena_reset cf_arena_free
 */
CF_API void* CF_CALL cf_arena_alloc(CF_Arena* arena, int size);

/**
 * @function cf_arena_free
 * @category allocator
 * @brief    Frees the most recent allocation(s) from the arena.
 * @param    arena         The arena from which to free memory.
 * @param    size          The size of the most recent allocation to free.
 * @remarks  This supports freeing memory in a Last-In-First-Out (LIFO) order, meaning 
 *           only the most recent allocation(s) can be freed. It does not support freeing allocations in 
 *           arbitrary order. Minimal error checking is performed, so only call this function if you
 *           know what you're doing, otherwise you'll get memory corruption issues.
 * @related  cf_arena_init cf_arena_alloc cf_arena_reset cf_arena_free
 */
CF_API void CF_CALL cf_arena_free(CF_Arena* arena, int ptr);

/**
 * @function cf_arena_reset
 * @category allocator
 * @brief    Resets the allocator.
 * @param    arena         The arena to reset.
 * @remarks  This does not free up internal resources, and will reuse all previously allocated
 *           resources to fulfill subsequent `cf_arena_alloc` calls.
 * @related  cf_arena_init cf_arena_alloc cf_arena_reset cf_arena_free
 */
CF_API void CF_CALL cf_arena_reset(CF_Arena* arena);

/**
 * @function cf_destroy_arena
 * @category allocator
 * @brief    Frees up all resources used by the allocator.
 * @param    arena         The arena to free.
 * @related  cf_arena_init cf_arena_alloc cf_arena_reset cf_arena_free
 */
CF_API void CF_CALL cf_destroy_arena(CF_Arena* arena);

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
 * @related  cf_destroy_memory_pool cf_memory_pool_alloc cf_memory_pool_free
 */
CF_API CF_MemoryPool* CF_CALL cf_make_memory_pool(int element_size, int element_count, int alignment);

/**
 * @function cf_destroy_memory_pool
 * @category allocator
 * @brief    Destroys a memory pool.
 * @param    pool           The pool to destroy.
 * @related  cf_make_memory_pool cf_memory_pool_alloc cf_memory_pool_free
 */
CF_API void CF_CALL cf_destroy_memory_pool(CF_MemoryPool* pool);

/**
 * @function cf_memory_pool_alloc
 * @category allocator
 * @brief    Allocates a chunk of memory from the pool. The allocation size was determined by `element_size` in `cf_make_memory_pool`.
 * @param    pool           The pool.
 * @return   Returns an aligned pointer of `size` bytes.
 * @related  cf_make_memory_pool cf_destroy_memory_pool cf_memory_pool_free
 */
CF_API void* CF_CALL cf_memory_pool_alloc(CF_MemoryPool* pool);

/**
 * @function cf_memory_pool_free
 * @category allocator
 * @brief    Frees an allocation made by `cf_memory_pool_alloc`.
 * @param    pool           The pool.
 * @param    element        The pointer to deallocate.
 * @related  cf_make_memory_pool cf_destroy_memory_pool cf_memory_pool_alloc
 */
CF_API void CF_CALL cf_memory_pool_free(CF_MemoryPool* pool, void* element);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

CF_INLINE void* aligned_alloc(size_t size, int alignment) { return cf_aligned_alloc(size, alignment); }
CF_INLINE void aligned_free(void* ptr) { return cf_aligned_free(ptr); }

CF_INLINE CF_Arena make_arena(int alignment, int block_size) { return cf_make_arena(alignment, block_size); }
CF_INLINE void* arena_alloc(CF_Arena* arena, int size) { return cf_arena_alloc(arena, size); }
CF_INLINE void arena_free(CF_Arena* arena, int size) { cf_arena_free(arena, size); }
CF_INLINE void arena_reset(CF_Arena* arena) { cf_arena_reset(arena); }
CF_INLINE void destroy_arena(CF_Arena* arena) { cf_destroy_arena(arena); }

CF_INLINE CF_MemoryPool* make_memory_pool(int element_size, int element_count, int alignment) { return cf_make_memory_pool(element_size, element_count, alignment); }
CF_INLINE void destroy_memory_pool(CF_MemoryPool* pool) { cf_destroy_memory_pool(pool); }
CF_INLINE void* memory_pool_alloc(CF_MemoryPool* pool) { return cf_memory_pool_alloc(pool); }
CF_INLINE void memory_pool_free(CF_MemoryPool* pool, void* element) { return cf_memory_pool_free(pool, element); }

}

#endif // CF_CPP

#endif // CF_ALLOC_H
