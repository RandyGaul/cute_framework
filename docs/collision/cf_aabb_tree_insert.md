[](../header.md ':include')

# cf_aabb_tree_insert

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)  
---

Adds a new [CF_Leaf](/collision/cf_leaf.md) to the tree.

```cpp
CF_API CF_Leaf CF_CALL cf_aabb_tree_insert(CF_AabbTree tree, CF_Aabb aabb, void* udata);
```

Parameters | Description
--- | ---
tree | The tree.
aabb | The AABB (axis-aligned bounding box) representing your object in the tree.
udata | Can be `NULL`. An optional user data pointer. This gets returned to you in the callback [CF_AabbTreeQueryFn](/collision/cf_aabbtreequeryfn.md).

## Return Value

Returns a [CF_Leaf](/collision/cf_leaf.md) representing the inserted AABB.

## Related Pages

[cf_aabb_tree_remove](/collision/cf_aabb_tree_remove.md)  
[cf_aabb_tree_update_leaf](/collision/cf_aabb_tree_update_leaf.md)  
[cf_aabb_tree_move](/collision/cf_aabb_tree_move.md)  
[cf_aabb_tree_get_aabb](/collision/cf_aabb_tree_get_aabb.md)  
[cf_aabb_tree_query_aabb](/collision/cf_aabb_tree_query_aabb.md)  
[cf_aabb_tree_query_ray](/collision/cf_aabb_tree_query_ray.md)  
