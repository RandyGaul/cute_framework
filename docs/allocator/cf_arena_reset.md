[](../header.md ':include')

# cf_arena_reset

Category: [allocator](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

Free's up all resources used by the allocator and places it back into an initialized state.

```cpp
void cf_arena_reset(CF_Arena* arena);
```

Parameters | Description
--- | ---
arena | The arena to reset.

## Related Pages

[cf_arena_init](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_arena_init.md)  
[cf_arena_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_arena_alloc.md)  
