[](../header.md ':include')

# cf_destroy_memory_pool

Category: [allocator](/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

Destroys a memory pool.

```cpp
void cf_destroy_memory_pool(CF_MemoryPool* pool);
```

Parameters | Description
--- | ---
pool | The pool to destroy.

## Remarks

Does not clean up any allocations that overflowed to `malloc` backup. See [cf_memory_pool_alloc](/allocator/cf_memory_pool_alloc.md) for more details.

## Related Pages

[cf_make_memory_pool](/allocator/cf_make_memory_pool.md)  
[cf_memory_pool_alloc](/allocator/cf_memory_pool_alloc.md)  
[cf_memory_pool_try_alloc](/allocator/cf_memory_pool_try_alloc.md)  
[cf_memory_pool_free](/allocator/cf_memory_pool_free.md)  
