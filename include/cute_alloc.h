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
CF_API void CF_CALL cf_allocator_restore_default();

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
CF_API void CF_CALL cf_arena_init(CF_Arena* arena, int alignment, int block_size);

/**
 * @function cf_arena_alloc
 * @category allocator
 * @brief    Allocates a block of memory aligned along a byte boundary.
 * @param    arena         The arena to allocate from.
 * @param    size          The size of the allocation, it cannot be larger than `block_size` from `cf_arena_init`.
 * @return   Returns an aligned pointer of `size` bytes.
 * @related  cf_arena_init cf_arena_reset
 */
CF_API void* CF_CALL cf_arena_alloc(CF_Arena* arena, size_t size);

/**
 * @function cf_arena_reset
 * @category allocator
 * @brief    Free's up all resources used by the allocator and places it back into an initialized state.
 * @param    arena         The arena to reset.
 * @related  cf_arena_init cf_arena_alloc
 */
CF_API void CF_CALL cf_arena_reset(CF_Arena* arena);

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
CF_API CF_MemoryPool* CF_CALL cf_make_memory_pool(int element_size, int element_count, int alignment);

/**
 * @function cf_destroy_memory_pool
 * @category allocator
 * @brief    Destroys a memory pool.
 * @param    pool           The pool to destroy.
 * @remarks  Does not clean up any allocations that overflowed to `malloc` backup. See `cf_memory_pool_alloc` for more details.
 * @related  cf_make_memory_pool cf_memory_pool_alloc cf_memory_pool_try_alloc cf_memory_pool_free
 */
CF_API void CF_CALL cf_destroy_memory_pool(CF_MemoryPool* pool);

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
CF_API void* CF_CALL cf_memory_pool_alloc(CF_MemoryPool* pool);

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
CF_API void* CF_CALL cf_memory_pool_try_alloc(CF_MemoryPool* pool);

/**
 * @function cf_memory_pool_free
 * @category allocator
 * @brief    Frees an allocation made by `cf_memory_pool_alloc` or `cf_memory_pool_try_alloc`.
 * @param    pool           The pool.
 * @param    element        The pointer to deallocate.
 * @related  cf_make_memory_pool cf_destroy_memory_pool cf_memory_pool_alloc cf_memory_pool_try_alloc
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

using Arena = CF_Arena;

CF_INLINE void arena_init(CF_Arena* arena, int alignment, int block_size) { cf_arena_init(arena, alignment, block_size); }
CF_INLINE void* arena_alloc(CF_Arena* arena, size_t size) { return cf_arena_alloc(arena, size); }
CF_INLINE void arena_reset(CF_Arena* arena) { return cf_arena_reset(arena); }

using MemoryPool = CF_MemoryPool;

CF_INLINE MemoryPool* make_memory_pool(int element_size, int element_count, int alignment) { return cf_make_memory_pool(element_size, element_count, alignment); }
CF_INLINE void destroy_memory_pool(MemoryPool* pool) { cf_destroy_memory_pool(pool); }
CF_INLINE void* memory_pool_alloc(MemoryPool* pool) { return cf_memory_pool_alloc(pool); }
CF_INLINE void* memory_pool_try_alloc(MemoryPool* pool) { return cf_memory_pool_try_alloc(pool); }
CF_INLINE void memory_pool_free(MemoryPool* pool, void* element) { return cf_memory_pool_free(pool, element); }

}

#endif // CF_CPP

#endif // CF_ALLOC_H
