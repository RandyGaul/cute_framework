[](../header.md ':include')

# cf_destroy_aabb_tree

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)  
---

Destroys an AABB tree previously created by [cf_make_aabb_tree](/collision/cf_make_aabb_tree.md) or [cf_make_aabb_tree_from_memory](/collision/cf_make_aabb_tree_from_memory.md).

```cpp
void cf_destroy_aabb_tree(CF_AabbTree tree);
```

Parameters | Description
--- | ---
tree | The tree to destroy.

## Remarks

If you have a serialized tree stored in a buffer of memory by [cf_aabb_tree_serialize](/collision/cf_aabb_tree_serialize.md), this function can be used to load up the serialized tree. This is a fairly advanced
function, if you just want to make an AABB tree you may be looking for [cf_make_aabb_tree](/collision/cf_make_aabb_tree.md) instead. Destroy the tree with [cf_destroy_aabb_tree](/collision/cf_destroy_aabb_tree.md) when you're done using it.

## Related Pages

[cf_make_aabb_tree](/collision/cf_make_aabb_tree.md)  
[cf_make_aabb_tree_from_memory](/collision/cf_make_aabb_tree_from_memory.md)  
