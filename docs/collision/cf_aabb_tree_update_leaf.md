[](../header.md ':include')

# cf_aabb_tree_update_leaf

Category: [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=collision)  
GitHub: [cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)  
---

Update's a [CF_Leaf](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_leaf.md)'s AABB (axis-aligned bounding box). Call this if your object moves to let the tree know about it.

```cpp
bool cf_aabb_tree_update_leaf(CF_AabbTree tree, CF_Leaf leaf, CF_Aabb aabb);
```

Parameters | Description
--- | ---
tree | The tree.
leaf | The [CF_Leaf](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_leaf.md) representing your object, returned from [cf_aabb_tree_insert](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_insert.md).
aabb | The new wrapping AABB around your moved object (you must re-calculate this AABB before calling).

## Return Value

Returns true if the leaf was updated internally, false otherwise.

## Remarks

Internally there are some optimizations so that the tree is only adjusted if the aabb is moved enough. See the return value for more details.

## Related Pages

[cf_aabb_tree_insert](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_insert.md)  
[cf_aabb_tree_move](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_move.md)  
[cf_aabb_tree_get_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_get_aabb.md)  
[cf_aabb_tree_query_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_aabb.md)  
[cf_aabb_tree_query_ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_ray.md)  
