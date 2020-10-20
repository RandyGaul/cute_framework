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

#ifndef CUTE_AABB_TREE_H
#define CUTE_AABB_TREE_H

#include <cute_defines.h>
#include <cute_math.h>

namespace cute
{

struct aabb_tree_t;
struct leaf_t { int id; };
typedef bool (aabb_tree_query_fn)(leaf_t leaf, aabb_t aabb, void* leaf_udata, void* fn_udata);

aabb_tree_t* create_aabb_tree(int initial_capacity = 0, void* user_allocator_context = NULL);
aabb_tree_t* create_aabb_tree_from_memory(const void* buffer, size_t size, void* user_allocator_context = NULL);
void destroy_aabb_tree(aabb_tree_t* tree);

/**
 * Adds a new leaf to the tree, and rebalances as necessary.
 */
leaf_t aabb_tree_insert(aabb_tree_t* tree, aabb_t aabb, void* udata = NULL);

/**
 * Removes a leaf from the tree, and rebalances as necessary.
 */
void aabb_tree_remove(aabb_tree_t* tree, leaf_t leaf);

/**
 * Use this function when an aabb needs to be updated. Leafs need to be updated whenever the shape
 * inside the leaf's aabb moves. Internally there are some optimizations so that the tree is only
 * adjusted if the aabb is moved enough.
 */
void aabb_tree_update_leaf(aabb_tree_t* tree, leaf_t leaf, aabb_t aabb);

/**
 * Returns the `udata` pointer from `aabb_tree_insert`.
 */
void* aabb_tree_get_udata(aabb_tree_t* tree, leaf_t leaf);

/**
 * Finds all leafs overlapping `aabb`. The `fn` callback is called once per overlap. If you want to stop
 * searching, return false from `fn`, otherwise keep returning true.
 */
void aabb_tree_query(const aabb_tree_t* tree, aabb_tree_query_fn* fn, aabb_t aabb, void* fn_udata = NULL);

/**
 * Finds all leafs hit by `ray`. The `fn` callback is called once per overlap. If you want to stop
 * searching, return false from `fn`, otherwise keep returning true.
 */
void aabb_tree_query(const aabb_tree_t* tree, aabb_tree_query_fn* fn, ray_t ray, void* fn_udata = NULL);

/**
 * Returns a cost heuristic value to quantify the quality of the tree.
 */
float aabb_tree_cost(const aabb_tree_t* tree);

/**
 * Asserts the internal structure is correct. Just for debugging.
 */
void aabb_tree_validate(const aabb_tree_t* tree);

/**
 * Returns the size of the tree if it were serialized into a buffer.
 */
size_t aabb_tree_serialized_size(const aabb_tree_t* tree);

/**
 * Writes the tree to `buffer`. The buffer should be at least `aabb_tree_serialized_size` bytes large.
 * Returns true on success, false otherwise.
 */
bool aabb_tree_serialize(const aabb_tree_t* tree, void* buffer, size_t size);

}

#endif // CUTE_AABB_TREE_H
