[](../header.md ':include')

# cf_aabb_tree_serialized_size

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)  
---

Returns the size of the tree if it were serialized into a buffer.

```cpp
CF_API size_t CF_CALL cf_aabb_tree_serialized_size(const CF_AabbTree tree);
```

Parameters | Description
--- | ---
tree | The tree.

## Return Value

Returns the size of the tree if it were serialized into a buffer.

## Remarks

A tree may be serialized with [cf_aabb_tree_serialize](/collision/cf_aabb_tree_serialize.md). The `size` parameter should be the value returned by this function.

## Related Pages

[cf_aabb_tree_serialize](/collision/cf_aabb_tree_serialize.md)  
