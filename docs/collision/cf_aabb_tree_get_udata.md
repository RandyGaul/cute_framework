[](../header.md ':include')

# cf_aabb_tree_get_udata

Category: [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=collision)  
GitHub: [cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)  
---

Returns the `udata` pointer from [cf_aabb_tree_insert](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_insert.md).

```cpp
void* cf_aabb_tree_get_udata(CF_AabbTree tree, CF_Leaf leaf);
```

Parameters | Description
--- | ---
tree | The tree.
leaf | The leaf.

## Return Value

The `udata` pointer from [cf_aabb_tree_insert](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_insert.md).

## Related Pages

[cf_aabb_tree_insert](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_insert.md)  
