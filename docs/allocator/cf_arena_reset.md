[](../header.md ':include')

# cf_arena_reset

Category: [allocator](/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

Free's up all resources used by the allocator and places it back into an initialized state.

```cpp
CF_API void CF_CALL cf_arena_reset(CF_Arena* arena);
```

Parameters | Description
--- | ---
arena | The arena to reset.

## Related Pages

[cf_arena_init](/allocator/cf_arena_init.md)  
[cf_arena_alloc](/allocator/cf_arena_alloc.md)  
