/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/


#ifndef CF_AABB_TREE_H
#define CF_AABB_TREE_H

#include "cute_defines.h"
#include "cute_math.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_AabbTree
 * @category collision
 * @brief    An opaque handle representing an AABB (axis-aligned bounding box) tree. See `cf_make_aabb_tree` for more details.
 * @related  cf_make_aabb_tree cf_aabb_tree_insert
 */
typedef struct CF_AabbTree { uint64_t id; } CF_AabbTree;
// @end

/**
 * @struct   CF_Leaf
 * @category collision
 * @brief    An opaque handle to an entry in a `CF_AabbTree`. See `cf_aabb_tree_insert` for more details.
 * @related  cf_aabb_tree_insert cf_aabb_tree_remove
 */
typedef struct CF_Leaf { int id; } CF_Leaf;
// @end

/**
 * @function CF_AabbTreeQueryFn
 * @category collision
 * @brief    A function pointer (callback) that reports hits during a tree collision query.
 * @param    leaf        The leaf node currently hit by the query.
 * @param    aabb        The AABB of the leaf currently hit by the query.
 * @param    leaf_udata  The user data pointer provided when you called `cf_aabb_tree_insert` as the `udata` param.
 * @param    fn_udata    The user data pointer provided when you called a query function, such as `cf_aabb_tree_query_aabb` or `cf_aabb_tree_query_ray`.
 * @return   You should return true if you want to continue the query, or false if you want to early-exit for any reason.
 * @example > Calculating a path along a small grid, and printing the result.
 *     TODO
 * @remarks  This function pointer is a function you must implement yourself. It will be called by `cf_aabb_tree_query_aabb` or `cf_aabb_tree_query_ray`
 *           during a query, and reports hits to you. Usually you will return true, but if you want to early out for any reason you can return false. You may
 *           return false as an optimization, for example, if you want to know if you hit _anything_, since processing later hits won't matter after the
             first hit is found.
 * @related  CF_AabbTree cf_make_aabb_tree cf_aabb_tree_query_aabb cf_aabb_tree_query_ray
 */
typedef bool (CF_AabbTreeQueryFn)(CF_Leaf leaf, CF_Aabb aabb, void* leaf_udata, void* fn_udata);

/**
 * @function cf_make_aabb_tree
 * @category collision
 * @brief    Creates a `CF_AabbTree`. This kind of tree is used for optimizing collision detection using AABBs (axis-aligned bounding boxes).
 * @param    initial_capacity  Sizes the internal arrays for a number of elements to insert. This may be zero, but a decent starting number is 256 if you're unsure.
 * @return   Returns a `CF_AabbTree` for optimizing collision queries.
 * @remarks  Usually an AABB tree is used as an acceleration structure to speed up collision detection. Some useful search terms to learn more are:
 *           Bounding Volume Hierarchy (BHV), Dynamic AABB Tree, Broadphase vs. Narrowphase. For many games it's a good idea to wrap your shapes up
 *           in an AABB and place them into the tree. The tree can then be queried for potential hits, where a fast test is used on the wrapping AABB
 *           first. You can then follow up potential hit-pairs by performing more expensive tests on the underlying shapes.
 * @related  cf_make_aabb_tree_from_memory cf_destroy_aabb_tree cf_aabb_tree_insert cf_aabb_tree_query_aabb cf_aabb_tree_query_ray
 */
CF_API CF_AabbTree CF_CALL cf_make_aabb_tree(int initial_capacity);

/**
 * @function cf_make_aabb_tree_from_memory
 * @category collision
 * @brief    Creates a `CF_AabbTree` from a buffer of memory. This is an advanced function, you're probably look for `cf_make_aabb_tree` instead.
 * @param    initial_capacity  Sizes the internal arrays for a number of elements to insert. This may be zero, but a decent starting number is 256 if you're unsure.
 * @return   Returns a `CF_AabbTree` for optimizing collision queries.
 * @remarks  If you have a serialized tree stored in a buffer of memory by `cf_aabb_tree_serialize`, this function can be used to load up the serialized tree. This is a fairly advanced
 *           function, if you just want to make an AABB tree you may be looking for `cf_make_aabb_tree` instead. Destroy the tree with `cf_destroy_aabb_tree` when you're done using it.
 * @related  cf_make_aabb_tree cf_aabb_tree_serialize cf_destroy_aabb_tree cf_aabb_tree_serialized_size
 */
CF_API CF_AabbTree CF_CALL cf_make_aabb_tree_from_memory(const void* buffer, size_t size /*= NULL*/);

/**
 * @function cf_destroy_aabb_tree
 * @category collision
 * @brief    Destroys an AABB tree previously created by `cf_make_aabb_tree` or `cf_make_aabb_tree_from_memory`.
 * @param    tree       The tree to destroy.
 * @remarks  If you have a serialized tree stored in a buffer of memory by `cf_aabb_tree_serialize`, this function can be used to load up the serialized tree. This is a fairly advanced
 *           function, if you just want to make an AABB tree you may be looking for `cf_make_aabb_tree` instead. Destroy the tree with `cf_destroy_aabb_tree` when you're done using it.
 * @related  cf_make_aabb_tree cf_make_aabb_tree_from_memory
 */
CF_API void CF_CALL cf_destroy_aabb_tree(CF_AabbTree tree);

/**
 * @function cf_aabb_tree_insert
 * @category collision
 * @brief    Adds a new `CF_Leaf` to the tree.
 * @param    tree       The tree.
 * @param    aabb       The AABB (axis-aligned bounding box) representing your object in the tree.
 * @param    udata      Can be `NULL`. An optional user data pointer. This gets returned to you in the callback `CF_AabbTreeQueryFn`.
 * @return   Returns a `CF_Leaf` representing the inserted AABB.
 * @related  cf_aabb_tree_remove cf_aabb_tree_update_leaf cf_aabb_tree_move cf_aabb_tree_get_aabb cf_aabb_tree_query_aabb cf_aabb_tree_query_ray
 */
CF_API CF_Leaf CF_CALL cf_aabb_tree_insert(CF_AabbTree tree, CF_Aabb aabb, void* udata);

/**
 * @function cf_aabb_tree_remove
 * @category collision
 * @brief    Removes a `CF_Leaf` from the tree.
 * @related  cf_aabb_tree_insert
 */
CF_API void CF_CALL cf_aabb_tree_remove(CF_AabbTree tree, CF_Leaf leaf);

/**
 * @function cf_aabb_tree_update_leaf
 * @category collision
 * @brief    Update's a `CF_Leaf`'s AABB (axis-aligned bounding box). Call this if your object moves to let the tree know about it.
 * @param    tree       The tree.
 * @param    leaf       The `CF_Leaf` representing your object, returned from `cf_aabb_tree_insert`.
 * @param    aabb       The new wrapping AABB around your moved object (you must re-calculate this AABB before calling).
 * @return   Returns true if the leaf was updated internally, false otherwise.
 * @remarks  Internally there are some optimizations so that the tree is only adjusted if the aabb is moved enough. See the return value for more details.
 * @related  cf_aabb_tree_insert cf_aabb_tree_move cf_aabb_tree_get_aabb cf_aabb_tree_query_aabb cf_aabb_tree_query_ray
 */
CF_API bool CF_CALL cf_aabb_tree_update_leaf(CF_AabbTree tree, CF_Leaf leaf, CF_Aabb aabb);

/**
 * @function cf_aabb_tree_move
 * @category collision
 * @brief    An optimized version of `cf_aabb_tree_update_leaf`. Update's a `CF_Leaf`'s AABB (axis-aligned bounding box) based on `offset` movement.
 * @param    tree       The tree.
 * @param    leaf       The `CF_Leaf` representing your object, returned from `cf_aabb_tree_insert`.
 * @param    aabb       The new wrapping AABB around your moved object (you must re-calculate this AABB before calling).
 * @param    offset     How far the new AABB will be moving, such as the object's velocity.
 * @return   Returns true if the leaf was updated internally, false otherwise.
 * @remarks  Internally there are some optimizations so that the tree is only adjusted if the aabb is moved enough. See the return value for more details.
 *           The `offset` param is used to try and predict where your object will be moving next. The internal AABB is expanded in this direction to
 *           try and reduce how often the tree needs to be internally rebalanced. For a simpler to use function, you may use `cf_aabb_tree_update_leaf`, which
 *           is likely good enough most of the time.
 * @related  cf_aabb_tree_insert cf_aabb_tree_update_leaf cf_aabb_tree_get_aabb cf_aabb_tree_query_aabb cf_aabb_tree_query_ray
 */
CF_API bool CF_CALL cf_aabb_tree_move(CF_AabbTree tree, CF_Leaf leaf, CF_Aabb aabb, CF_V2 offset);

/**
 * @function cf_aabb_tree_get_aabb
 * @category collision
 * @brief    Fetches the internally expanded AABB for a given leaf.
 * @param    tree       The tree.
 * @param    leaf       The leaf.
 * @return   The expanded AABB for `leaf`.
 * @remarks  This is useful for when you want to generate all pairs of potential overlaps for a specific leaf. Just simply use `cf_aabb_tree_query_aabb`
             on the the return value of this function.
 * @related  cf_aabb_tree_insert cf_aabb_tree_remove cf_aabb_tree_update_leaf cf_aabb_tree_get_aabb cf_aabb_tree_query_aabb cf_aabb_tree_query_ray
 */
CF_API CF_Aabb CF_CALL cf_aabb_tree_get_aabb(CF_AabbTree tree, CF_Leaf leaf);

/**
 * @function cf_aabb_tree_get_udata
 * @category collision
 * @brief    Returns the `udata` pointer from `cf_aabb_tree_insert`.
 * @param    tree       The tree.
 * @param    leaf       The leaf.
 * @return   The `udata` pointer from `cf_aabb_tree_insert`.
 * @related  cf_aabb_tree_insert
 */
CF_API void* CF_CALL cf_aabb_tree_get_udata(CF_AabbTree tree, CF_Leaf leaf);

/**
 * @function cf_aabb_tree_query_aabb
 * @category collision
 * @brief    Finds all `CF_Leaf`'s who's AABB overlaps the param `aabb`.
 * @param    tree       The tree to query.
 * @param    fn         A callback `CF_AabbTreeQueryFn` you have implemented. This is called once per hit. See `CF_AabbTreeQueryFn` for more details.
 * @param    aabb       The `CF_Aabb` to query the tree against. Internally tracked AABBs hit will be reported via `fn`, a callback `CF_AabbTreeQueryFn` you must implement.
 * @param    fn_udata   Can be `NULL`. An optional user data pointer, handed back to you when `fn` is called to report hits.
 * @related  cf_aabb_tree_insert cf_aabb_tree_query_ray
 */
CF_API void CF_CALL cf_aabb_tree_query_aabb(const CF_AabbTree tree, CF_AabbTreeQueryFn* fn, CF_Aabb aabb, void* fn_udata);

/**
 * @function cf_aabb_tree_query_ray
 * @category collision
 * @brief    Finds all `CF_Leaf`'s who's AABB overlaps the param `ray`.
 * @param    tree       The tree to query.
 * @param    fn         A callback `CF_AabbTreeQueryFn` you have implemented. This is called once per hit. See `CF_AabbTreeQueryFn` for more details.
 * @param    ray        The `CF_Ray` to query the tree against. Internally tracked AABBs hit will be reported via `fn`, a callback `CF_AabbTreeQueryFn` you must implement.
 * @param    fn_udata   Can be `NULL`. An optional user data pointer, handed back to you when `fn` is called to report hits.
 * @related  cf_aabb_tree_insert cf_aabb_tree_query_aabb
 */
CF_API void CF_CALL cf_aabb_tree_query_ray(const CF_AabbTree tree, CF_AabbTreeQueryFn* fn, CF_Ray ray, void* fn_udata);

/**
 * @function cf_aabb_tree_cost
 * @category collision
 * @brief    Returns a cost heuristic value to quantify the quality of the tree.
 * @param    tree       The tree.
 * @return   Returns a cost heuristic value to quantify the quality of the tree.
 */
CF_API float CF_CALL cf_aabb_tree_cost(const CF_AabbTree tree);

/**
 * @function cf_aabb_tree_validate
 * @category collision
 * @brief    Asserts the internal structure is correct. Just for debugging.
 */
CF_API void CF_CALL cf_aabb_tree_validate(const CF_AabbTree tree);

/**
 * @function cf_aabb_tree_serialized_size
 * @category collision
 * @brief    Returns the size of the tree if it were serialized into a buffer.
 * @param    tree       The tree.
 * @return   Returns the size of the tree if it were serialized into a buffer.
 * @remarks  A tree may be serialized with `cf_aabb_tree_serialize`. The `size` parameter should be the value returned by this function.
 * @related  cf_aabb_tree_serialize
 */
CF_API size_t CF_CALL cf_aabb_tree_serialized_size(const CF_AabbTree tree);

/**
 * @function cf_aabb_tree_serialize
 * @category collision
 * @brief    Serializes a tree to a buffer.
 * @param    tree       The tree.
 * @param    buffer     A buffer of at least `cf_aabb_tree_serialized_size` size to save the tree into as a byte-array.
 * @return   Returns true upon success, false otherwise.
 * @remarks  Call `cf_aabb_tree_serialized_size` to get the `size` parameter. This is useful to save a tree to disk or send a tree over
 *           the network. The main purpose is an optimization to avoid building the tree from scratch.
 * @related  cf_make_aabb_tree cf_aabb_tree_serialized_size
 */
CF_API bool CF_CALL cf_aabb_tree_serialize(const CF_AabbTree tree, void* buffer, size_t size);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using AabbTree = CF_AabbTree;
using Leaf = CF_Leaf;
using AabbTreeQueryFn = CF_AabbTreeQueryFn;
using Aabb = CF_Aabb;
using Ray = CF_Ray;

CF_INLINE AabbTree make_aabb_tree(int initial_capacity = 0) { return cf_make_aabb_tree(initial_capacity); }
CF_INLINE AabbTree make_aabb_tree_from_memory(const void* buffer, size_t size) { return cf_make_aabb_tree_from_memory(buffer, size); }
CF_INLINE void destroy_aabb_tree(AabbTree tree) { cf_destroy_aabb_tree(tree); }
CF_INLINE Leaf aabb_tree_insert(AabbTree tree, Aabb aabb, void* udata = NULL) { return cf_aabb_tree_insert(tree, aabb, udata); }
CF_INLINE void aabb_tree_remove(AabbTree tree, Leaf leaf) { cf_aabb_tree_remove(tree, leaf); }
CF_INLINE bool aabb_tree_update_leaf(AabbTree tree, Leaf leaf, Aabb aabb) { return cf_aabb_tree_update_leaf(tree, leaf, aabb); }
CF_INLINE bool aabb_tree_move(AabbTree tree, Leaf leaf, Aabb aabb, v2 offset) { return cf_aabb_tree_move(tree, leaf, aabb, offset); }
CF_INLINE Aabb aabb_tree_get_aabb(AabbTree tree, Leaf leaf) { return cf_aabb_tree_get_aabb(tree, leaf); }
CF_INLINE void* aabb_tree_get_udata(AabbTree tree, Leaf leaf) { return cf_aabb_tree_get_udata(tree, leaf); }
CF_INLINE void aabb_tree_query(const AabbTree tree, AabbTreeQueryFn* fn, Aabb aabb, void* fn_udata = NULL) { cf_aabb_tree_query_aabb(tree, fn, aabb, fn_udata); }
CF_INLINE void aabb_tree_query(const AabbTree tree, AabbTreeQueryFn* fn, Ray ray, void* fn_udata = NULL) { cf_aabb_tree_query_ray(tree, fn, ray, fn_udata); }
CF_INLINE float aabb_tree_cost(const AabbTree tree) { return cf_aabb_tree_cost(tree); }
CF_INLINE void aabb_tree_validate(const AabbTree tree) { cf_aabb_tree_validate(tree); }
CF_INLINE size_t aabb_tree_serialized_size(const AabbTree tree) { return cf_aabb_tree_serialized_size(tree); }
CF_INLINE bool aabb_tree_serialize(const AabbTree tree, void* buffer, size_t size) { return cf_aabb_tree_serialize(tree, buffer, size); }

}

#endif // CF_CPP

#endif // CF_AABB_TREE_H
