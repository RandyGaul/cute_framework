[](../header.md ':include')

# cf_arena_init

Category: [allocator](/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

Initializes an arena for later allocations.

```cpp
CF_API void CF_CALL cf_arena_init(CF_Arena* arena, int alignment, int block_size);
```

Parameters | Description
--- | ---
arena | The arena to initialize.
alignment | An alignment boundary, must be a power of two.
block_size | The default size of each internal call to `malloc` to form pages to further allocate from.

## Related Pages

[cf_arena_alloc](/allocator/cf_arena_alloc.md)  
[cf_arena_reset](/allocator/cf_arena_reset.md)  
