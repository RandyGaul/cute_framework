# cf_arena_alloc | [allocator](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/README.md) | [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)

Allocates a block of memory aligned along a byte boundary.

```cpp
void* cf_arena_alloc(CF_Arena* arena, size_t size);
```

Parameters | Description
--- | ---
arena | The arena to allocate from.
size | The size of the allocation, it can be larger than `block_size` from [cf_arena_init](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_arena_init.md).

## Return Value

Returns an aligned pointer of `size` bytes.

## Related Pages

[cf_arena_init](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_arena_init.md)  
[cf_arena_reset](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_arena_reset.md)  
