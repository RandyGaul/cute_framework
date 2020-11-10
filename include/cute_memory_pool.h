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

#ifndef CUTE_MEMORY_POOL_H
#define CUTE_MEMORY_POOL_H

#include <cute_defines.h>

namespace cute
{

/**
 * Memory pool is useful mainly as an optimization for one of two purposes.
 * 
 *     1. Avoid memory fragmentation over time.
 *     2. Avoid synchronization (mutex) within `malloc`.
 * 
 * The idea is to allocate a block of memory once, and then manually allocate from that block
 * different chunks of a fixed size.
 */
struct memory_pool_t;

/**
 * Constructs a new memory pool.
 * `element_size` is the fixed size each internal allocation will be.
 * `element_count` determins how big the internal pool will be.
 */
CUTE_API memory_pool_t* CUTE_CALL memory_pool_make(int element_size, int element_count, void* user_allocator_context = NULL);

/**
 * Destroys a memory pool previously created with `memory_pool_make`. Does not clean up any leftover
 * allocations from `memory_pool_alloc` that overflowed to the `malloc` backup. See `memory_pool_alloc`
 * for more details.
 */
CUTE_API void CUTE_CALL memory_pool_destroy(memory_pool_t* pool);

/**
 * Returns a block of memory of `element_size` bytes. If the number of allocations in the pool exceeds
 * `element_count` then `malloc` is used as a fallback.
 */
CUTE_API void* CUTE_CALL memory_pool_alloc(memory_pool_t* pool);

/**
 * The same as `memory_pool_alloc` without the `malloc` fallback -- returns `NULL` if the memory pool
 * is all used up.
 */
CUTE_API void* CUTE_CALL memory_pool_try_alloc(memory_pool_t* pool);

/**
 * Frees an allocation previously acquired by `memory_pool_alloc` or `memory_pool_try_alloc`.
 */
CUTE_API void CUTE_CALL memory_pool_free(memory_pool_t* pool, void* element);

}

#endif // CUTE_MEMORY_POOL_H
