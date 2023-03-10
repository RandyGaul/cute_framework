[](../header.md ':include')

# cf_aabb_tree_get_aabb

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)  
---

Fetches the internally expanded AABB for a given leaf.

```cpp
CF_API CF_Aabb CF_CALL cf_aabb_tree_get_aabb(CF_AabbTree tree, CF_Leaf leaf);
```

Parameters | Description
--- | ---
tree | The tree.
leaf | The leaf.

## Return Value

The expanded AABB for `leaf`.

## Remarks

This is useful for when you want to generate all pairs of potential overlaps for a specific leaf. Just simply use [cf_aabb_tree_query_aabb](/collision/cf_aabb_tree_query_aabb.md)
  on the the return value of this function.

## Related Pages

[cf_aabb_tree_insert](/collision/cf_aabb_tree_insert.md)  
[cf_aabb_tree_remove](/collision/cf_aabb_tree_remove.md)  
[cf_aabb_tree_update_leaf](/collision/cf_aabb_tree_update_leaf.md)  
[cf_aabb_tree_query_ray](/collision/cf_aabb_tree_query_ray.md)  
[cf_aabb_tree_query_aabb](/collision/cf_aabb_tree_query_aabb.md)  
