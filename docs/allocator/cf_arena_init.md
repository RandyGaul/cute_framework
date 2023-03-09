# cf_arena_init

Category: [allocator](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

Initializes an arena for later allocations.

```cpp
void cf_arena_init(CF_Arena* arena, int alignment, int block_size);
```

Parameters | Description
--- | ---
arena | The arena to initialize.
alignment | An alignment boundary, must be a power of two.
block_size | The default size of each internal call to `malloc` to form pages to further allocate from.

## Related Pages

[cf_arena_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_arena_alloc.md)  
[cf_arena_reset](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_arena_reset.md)  
