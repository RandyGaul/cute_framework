# cf_aabb_tree_get_aabb

Category: [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=collision)  
GitHub: [cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)  
---

Fetches the internally expanded AABB for a given leaf.

```cpp
CF_Aabb cf_aabb_tree_get_aabb(CF_AabbTree tree, CF_Leaf leaf);
```

Parameters | Description
--- | ---
tree | The tree.
leaf | The leaf.

## Return Value

The expanded AABB for `leaf`.

## Remarks

This is useful for when you want to generate all pairs of potential overlaps for a specific leaf. Just simply use [cf_aabb_tree_query_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_aabb.md)
  on the the return value of this function.

## Related Pages

[cf_aabb_tree_insert](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_insert.md)  
[cf_aabb_tree_remove](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_remove.md)  
[cf_aabb_tree_update_leaf](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_update_leaf.md)  
[cf_aabb_tree_query_ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_ray.md)  
[cf_aabb_tree_query_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_aabb.md)  
