# cf_aabb_tree_serialize | [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/README.md) | [cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)

Serializes a tree to a buffer.

```cpp
bool cf_aabb_tree_serialize(const CF_AabbTree tree, void* buffer, size_t size);
```

Parameters | Description
--- | ---
tree | The tree.
buffer | A buffer of at least [cf_aabb_tree_serialized_size](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_serialized_size.md) size to save the tree into as a byte-array.

## Return Value

Returns true upon success, false otherwise.

## Remarks

Call [cf_aabb_tree_serialized_size](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_serialized_size.md) to get the `size` parameter. This is useful to save a tree to disk or send a tree over
the network. The main purpose is an optimization to avoid building the tree from scratch.

## Related Pages

[cf_make_aabb_tree](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_make_aabb_tree.md)  
[cf_aabb_tree_serialized_size](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_serialized_size.md)  
