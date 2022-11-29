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

#include "cute_defines.h"
#include "cute_math.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct CF_Leaf
{
	#ifdef CUTE_CPP
	int id = -1;
	#else
	int id; /*= -1;*/
	#endif
} CF_Leaf;

typedef struct CF_AabbTree CF_AabbTree;
typedef bool (CF_AabbTreeQueryFn)(CF_Leaf leaf, CF_Aabb aabb, void* leaf_udata, void* fn_udata);

CUTE_INLINE CF_Leaf cf_leaf_defaults()
{
	CF_Leaf result;
	result.id = -1;
	return result;
}
CUTE_API CF_AabbTree* CUTE_CALL cf_make_aabb_tree(int initial_capacity /*= 0*/ /*= NULL*/);
CUTE_API CF_AabbTree* CUTE_CALL cf_make_aabb_tree_from_memory(const void* buffer, size_t size /*= NULL*/);
CUTE_API void CUTE_CALL cf_destroy_aabb_tree(CF_AabbTree* tree);

/**
 * Adds a new leaf to the tree, and rebalances as necessary.
 */
CUTE_API CF_Leaf CUTE_CALL CF_Aabbree_insert(CF_AabbTree* tree, CF_Aabb aabb, void* udata /*= NULL*/);

/**
 * Removes a leaf from the tree, and rebalances as necessary.
 */
CUTE_API void CUTE_CALL CF_Aabbree_remove(CF_AabbTree* tree, CF_Leaf leaf);

/**
 * Use this function when an aabb needs to be updated. Leafs need to be updated whenever the shape
 * inside the leaf's aabb moves. Internally there are some optimizations so that the tree is only
 * adjusted if the aabb is moved enough.
 * Returns true if the leaf was updated, false otherwise.
 */
CUTE_API bool CUTE_CALL CF_Aabbree_update_leaf(CF_AabbTree* tree, CF_Leaf leaf, CF_Aabb aabb);

/**
 * Updates a leaf with a new aabb (if needed) with the new `aabb` and an `offset` for how far the new
 * aabb will be moving.
 * This function does more optimizations than `aabb_tree_update_leaf` by attempting to use the `offset`
 * to predict motion and avoid restructering of the tree.
 * Returns true if the leaf was updated, false otherwise.
 */
CUTE_API bool CUTE_CALL CF_Aabbree_move(CF_AabbTree* tree, CF_Leaf leaf, CF_Aabb aabb, CF_V2 offset);

/**
 * Returns the internal "expanded" aabb. This is useful for when you want to generate all pairs of
 * potential overlaps for a specific leaf. Just simply use `CF_Aabbree_query_aabb` on the the return value
 * of this function.
 */
CUTE_API CF_Aabb CUTE_CALL CF_Aabbree_get_aabb(CF_AabbTree* tree, CF_Leaf leaf);

/**
 * Returns the `udata` pointer from `aabb_tree_insert`.
 */
CUTE_API void* CUTE_CALL CF_Aabbree_get_udata(CF_AabbTree* tree, CF_Leaf leaf);

/**
 * Finds all leafs overlapping `aabb`. The `fn` callback is called once per overlap. If you want to stop
 * searching, return false from `fn`, otherwise keep returning true.
 */
CUTE_API void CUTE_CALL CF_Aabbree_query_aabb(const CF_AabbTree* tree, CF_AabbTreeQueryFn* fn, CF_Aabb aabb, void* fn_udata /*= NULL*/);

/**
 * Finds all leafs hit by `ray`. The `fn` callback is called once per overlap. If you want to stop
 * searching, return false from `fn`, otherwise keep returning true.
 */
CUTE_API void CUTE_CALL CF_Aabbree_query_ray(const CF_AabbTree* tree, CF_AabbTreeQueryFn* fn, CF_Ray ray, void* fn_udata /*= NULL*/);

/**
 * Returns a cost heuristic value to quantify the quality of the tree.
 */
CUTE_API float CUTE_CALL CF_Aabbree_cost(const CF_AabbTree* tree);

/**
 * Asserts the internal structure is correct. Just for debugging.
 */
CUTE_API void CUTE_CALL CF_Aabbree_validate(const CF_AabbTree* tree);

/**
 * Returns the size of the tree if it were serialized into a buffer.
 */
CUTE_API size_t CUTE_CALL CF_Aabbree_serialized_size(const CF_AabbTree* tree);

/**
 * Writes the tree to `buffer`. The buffer should be at least `CF_Aabbree_serialized_size` bytes large.
 * Returns true on success, false otherwise.
 */
CUTE_API bool CUTE_CALL CF_Aabbree_serialize(const CF_AabbTree* tree, void* buffer, size_t size);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using AabbTree = CF_AabbTree;

using Leaf = CF_Leaf;
using AabbTreeQueryFn = CF_AabbTreeQueryFn;
using Aabb = CF_Aabb;
using Ray = CF_Ray;

CUTE_INLINE AabbTree* make_aabb_tree(int initial_capacity = 0) { return cf_make_aabb_tree(initial_capacity); }
CUTE_INLINE AabbTree* make_aabb_tree_from_memory(const void* buffer, size_t size = NULL) { return cf_make_aabb_tree_from_memory(buffer, size); }
CUTE_INLINE void destroy_aabb_tree(AabbTree* tree) { cf_destroy_aabb_tree(tree); }
CUTE_INLINE Leaf aabb_tree_insert(AabbTree* tree, Aabb aabb, void* udata = NULL) { return CF_Aabbree_insert(tree, aabb, udata); }
CUTE_INLINE void aabb_tree_remove(AabbTree* tree, Leaf leaf) { CF_Aabbree_remove(tree, leaf); }
CUTE_INLINE bool aabb_tree_update_leaf(AabbTree* tree, Leaf leaf, Aabb aabb) { return CF_Aabbree_update_leaf(tree, leaf, aabb); }
CUTE_INLINE bool aabb_tree_move(AabbTree* tree, Leaf leaf, Aabb aabb, v2 offset) { return CF_Aabbree_move(tree, leaf, aabb, offset); }
CUTE_INLINE Aabb aabb_tree_get_aabb(AabbTree* tree, Leaf leaf) { return CF_Aabbree_get_aabb(tree, leaf); }
CUTE_INLINE void* aabb_tree_get_udata(AabbTree* tree, Leaf leaf) { return CF_Aabbree_get_udata(tree, leaf); }
CUTE_INLINE void aabb_tree_query(const AabbTree* tree, AabbTreeQueryFn* fn, Aabb aabb, void* fn_udata = NULL) { CF_Aabbree_query_aabb(tree, fn, aabb, fn_udata); }
CUTE_INLINE void aabb_tree_query(const AabbTree* tree, AabbTreeQueryFn* fn, Ray ray, void* fn_udata = NULL) { CF_Aabbree_query_ray(tree, fn, ray, fn_udata); }
CUTE_INLINE float aabb_tree_cost(const AabbTree* tree) { return CF_Aabbree_cost(tree); }
CUTE_INLINE void aabb_tree_validate(const AabbTree* tree) { CF_Aabbree_validate(tree); }
CUTE_INLINE size_t aabb_tree_serialized_size(const AabbTree* tree) { return CF_Aabbree_serialized_size(tree); }
CUTE_INLINE bool aabb_tree_serialize(const AabbTree* tree, void* buffer, size_t size) { return CF_Aabbree_serialize(tree, buffer, size); }

}

#endif // CUTE_CPP

#endif // CUTE_AABB_TREE_H
