[](../header.md ':include')

# cf_aabb_tree_move

Category: [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=collision)  
GitHub: [cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)  
---

An optimized version of [cf_aabb_tree_update_leaf](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_update_leaf.md). Update's a [CF_Leaf](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_leaf.md)'s AABB (axis-aligned bounding box) based on `offset` movement.

```cpp
bool cf_aabb_tree_move(CF_AabbTree tree, CF_Leaf leaf, CF_Aabb aabb, CF_V2 offset);
```

Parameters | Description
--- | ---
tree | The tree.
leaf | The [CF_Leaf](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_leaf.md) representing your object, returned from [cf_aabb_tree_insert](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_insert.md).
aabb | The new wrapping AABB around your moved object (you must re-calculate this AABB before calling).
offset | How far the new AABB will be moving, such as the object's velocity.

## Return Value

Returns true if the leaf was updated internally, false otherwise.

## Remarks

Internally there are some optimizations so that the tree is only adjusted if the aabb is moved enough. See the return value for more details.
The `offset` param is used to try and predict where your object will be moving next. The internal AABB is expanded in this direction to
try and reduce how often the tree needs to be internally rebalanced. For a simpler to use function, you may use [cf_aabb_tree_update_leaf](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_update_leaf.md), which
is likely good enough most of the time.

## Related Pages

[cf_aabb_tree_insert](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_insert.md)  
[cf_aabb_tree_update_leaf](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_update_leaf.md)  
[cf_aabb_tree_get_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_get_aabb.md)  
[cf_aabb_tree_query_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_aabb.md)  
[cf_aabb_tree_query_ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_ray.md)  
