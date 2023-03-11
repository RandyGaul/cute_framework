[](../header.md ':include')

# CF_Allocator

Category: [allocator](/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

A simple way to allocate memory without calling `malloc` too often.

Struct Members | Description
--- | ---
`void* udata` | Can be `NULL`. An optional parameter handed back to you in the other function pointers below.
`void* (*alloc_fn)(size_t size, void* udata)` | Your custom allocation function.
`void (*free_fn)(void* ptr, void* udata)` | Your custom free function.
`void* (*calloc_fn)(size_t size, size_t count, void* udata)` | Your custom calloc function. Allocates memory that is cleared to zero.
`void* (*realloc_fn)(void* ptr, size_t size, void* udata)` | Your custom realloc function. Reallocates a pointer to a new size.

## Remarks

Individual allocations cannot be free'd, instead the entire allocator can reset.

## Related Pages

[cf_arena_init](/allocator/cf_arena_init.md)  
[cf_arena_alloc](/allocator/cf_arena_alloc.md)  
[cf_arena_reset](/allocator/cf_arena_reset.md)  
