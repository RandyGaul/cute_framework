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

#ifndef CUTE_AABB_TREE_H
#define CUTE_AABB_TREE_H

#include <cute_defines.h>
#include <cute_math.h>

namespace cute
{

struct aabb_tree_t;
struct leaf_t { int id; };
typedef bool (aabb_tree_query_fn)(leaf_t leaf, aabb_t aabb, void* leaf_udata, void* fn_udata);

CUTE_API aabb_tree_t* CUTE_CALL create_aabb_tree(int initial_capacity = 0, void* user_allocator_context = NULL);
CUTE_API aabb_tree_t* CUTE_CALL create_aabb_tree_from_memory(const void* buffer, size_t size, void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL destroy_aabb_tree(aabb_tree_t* tree);

/**
 * Adds a new leaf to the tree, and rebalances as necessary.
 */
CUTE_API leaf_t CUTE_CALL aabb_tree_insert(aabb_tree_t* tree, aabb_t aabb, void* udata = NULL);

/**
 * Removes a leaf from the tree, and rebalances as necessary.
 */
CUTE_API void CUTE_CALL aabb_tree_remove(aabb_tree_t* tree, leaf_t leaf);

/**
 * Use this function when an aabb needs to be updated. Leafs need to be updated whenever the shape
 * inside the leaf's aabb moves. Internally there are some optimizations so that the tree is only
 * adjusted if the aabb is moved enough.
 * Returns true if the leaf was updated, false otherwise.
 */
CUTE_API bool CUTE_CALL aabb_tree_update_leaf(aabb_tree_t* tree, leaf_t leaf, aabb_t aabb);

/**
 * Updates a leaf with a new aabb (if needed) with the new `aabb` and an `offset` for how far the new
 * aabb will be moving.
 * This function does more optimizations than `aabb_tree_update_leaf` by attempting to use the `offset`
 * to predict motion and avoid restructering of the tree.
 * Returns true if the leaf was updated, false otherwise.
 */
CUTE_API bool CUTE_CALL aabb_tree_move(aabb_tree_t* tree, leaf_t leaf, aabb_t aabb, v2 offset);

/**
 * Returns the internal "expanded" aabb. This is useful for when you want to generate all pairs of
 * potential overlaps for a specific leaf. Just simply use `aabb_tree_query` on the the return value
 * of this function.
 */
CUTE_API aabb_t CUTE_CALL aabb_tree_get_aabb(aabb_tree_t* tree, leaf_t leaf);

/**
 * Returns the `udata` pointer from `aabb_tree_insert`.
 */
CUTE_API void* CUTE_CALL aabb_tree_get_udata(aabb_tree_t* tree, leaf_t leaf);

/**
 * Finds all leafs overlapping `aabb`. The `fn` callback is called once per overlap. If you want to stop
 * searching, return false from `fn`, otherwise keep returning true.
 */
CUTE_API void CUTE_CALL aabb_tree_query(const aabb_tree_t* tree, aabb_tree_query_fn* fn, aabb_t aabb, void* fn_udata = NULL);

/**
 * Finds all leafs hit by `ray`. The `fn` callback is called once per overlap. If you want to stop
 * searching, return false from `fn`, otherwise keep returning true.
 */
CUTE_API void CUTE_CALL aabb_tree_query(const aabb_tree_t* tree, aabb_tree_query_fn* fn, ray_t ray, void* fn_udata = NULL);

/**
 * Returns a cost heuristic value to quantify the quality of the tree.
 */
CUTE_API float CUTE_CALL aabb_tree_cost(const aabb_tree_t* tree);

/**
 * Asserts the internal structure is correct. Just for debugging.
 */
CUTE_API void CUTE_CALL aabb_tree_validate(const aabb_tree_t* tree);

/**
 * Returns the size of the tree if it were serialized into a buffer.
 */
CUTE_API size_t CUTE_CALL aabb_tree_serialized_size(const aabb_tree_t* tree);

/**
 * Writes the tree to `buffer`. The buffer should be at least `aabb_tree_serialized_size` bytes large.
 * Returns true on success, false otherwise.
 */
CUTE_API bool CUTE_CALL aabb_tree_serialize(const aabb_tree_t* tree, void* buffer, size_t size);

}

#endif // CUTE_AABB_TREE_H
