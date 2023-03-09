[](../header.md ':include')

# CF_Arena

Category: [allocator](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

A simple way to allocate memory without calling `malloc` too often.

## Remarks

Individual allocations cannot be free'd, instead the entire allocator can reset.

## Related Pages

[cf_arena_init](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_arena_init.md)  
[cf_arena_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_arena_alloc.md)  
[cf_arena_reset](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_arena_reset.md)  
