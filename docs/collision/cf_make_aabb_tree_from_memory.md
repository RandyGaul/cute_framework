[](../header.md ':include')

# cf_make_aabb_tree_from_memory

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)  
---

Creates a [CF_AabbTree](/collision/cf_aabbtree.md) from a buffer of memory. This is an advanced function, you're probably look for [cf_make_aabb_tree](/collision/cf_make_aabb_tree.md) instead.

```cpp
CF_AabbTree cf_make_aabb_tree_from_memory(const void* buffer, size_t size /*= NULL*/);
```

Parameters | Description
--- | ---
initial_capacity | Sizes the internal arrays for a number of elements to insert. This may be zero, but a decent starting number is 256 if you're unsure.

## Return Value

Returns a [CF_AabbTree](/collision/cf_aabbtree.md) for optimizing collision queries.

## Remarks

If you have a serialized tree stored in a buffer of memory by [cf_aabb_tree_serialize](/collision/cf_aabb_tree_serialize.md), this function can be used to load up the serialized tree. This is a fairly advanced
function, if you just want to make an AABB tree you may be looking for [cf_make_aabb_tree](/collision/cf_make_aabb_tree.md) instead. Destroy the tree with [cf_destroy_aabb_tree](/collision/cf_destroy_aabb_tree.md) when you're done using it.

## Related Pages

[cf_make_aabb_tree](/collision/cf_make_aabb_tree.md)  
[cf_aabb_tree_serialize](/collision/cf_aabb_tree_serialize.md)  
[cf_destroy_aabb_tree](/collision/cf_destroy_aabb_tree.md)  
[cf_aabb_tree_serialized_size](/collision/cf_aabb_tree_serialized_size.md)  
