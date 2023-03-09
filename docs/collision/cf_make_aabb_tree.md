[](../header.md ':include')

# cf_make_aabb_tree

Category: [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=collision)  
GitHub: [cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)  
---

Creates a [CF_AabbTree](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabbtree.md). This kind of tree is used for optimizing collision detection using AABBs (axis-aligned bounding boxes).

```cpp
CF_AabbTree cf_make_aabb_tree(int initial_capacity);
```

Parameters | Description
--- | ---
initial_capacity | Sizes the internal arrays for a number of elements to insert. This may be zero, but a decent starting number is 256 if you're unsure.

## Return Value

Returns a [CF_AabbTree](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabbtree.md) for optimizing collision queries.

## Remarks

Usually an AABB tree is used as an acceleration structure to speed up collision detection. Some useful search terms to learn more are:
Bounding Volume Hierarchy (BHV), Dynamic AABB Tree, Broadphase vs. Narrowphase. For many games it's a good idea to wrap your shapes up
in an AABB and place them into the tree. The tree can then be queried for potential hits, where a fast test is used on the wrapping AABB
first. You can then follow up potential hit-pairs by performing more expensive tests on the underlying shapes.

## Related Pages

[cf_make_aabb_tree_from_memory](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_make_aabb_tree_from_memory.md)  
[cf_destroy_aabb_tree](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_destroy_aabb_tree.md)  
[cf_aabb_tree_insert](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_insert.md)  
[cf_aabb_tree_query_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_aabb.md)  
[cf_aabb_tree_query_ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_ray.md)  
