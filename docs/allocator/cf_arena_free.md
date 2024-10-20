[//]: # (This file is automatically generated by Cute Framework's docs parser.)
[//]: # (Do not edit this file by hand!)
[//]: # (See: https://github.com/RandyGaul/cute_framework/blob/master/samples/docs_parser.cpp)
[](../header.md ':include')

# cf_arena_free

Category: [allocator](/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

Frees the most recent allocation(s) from the arena.

```cpp
void cf_arena_free(CF_Arena* arena, int ptr);
```

Parameters | Description
--- | ---
arena | The arena from which to free memory.
size | The size of the most recent allocation to free.

## Remarks

This supports freeing memory in a Last-In-First-Out (LIFO) order, meaning 
only the most recent allocation(s) can be freed. It does not support freeing allocations in 
arbitrary order. Minimal error checking is performed, so only call this function if you
know what you're doing, otherwise you'll get memory corruption issues.

## Related Pages

[cf_arena_init](/allocator/cf_arena_init.md)  
[cf_arena_alloc](/allocator/cf_arena_alloc.md)  
[cf_arena_reset](/allocator/cf_arena_reset.md)  
