[](../header.md ':include')

# cf_make_memory_pool

Category: [allocator](/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

Creates a memory pool.

```cpp
CF_API CF_MemoryPool* CF_CALL cf_make_memory_pool(int element_size, int element_count, int alignment);
```

Parameters | Description
--- | ---
element_size | The size of each allocation.
element_count | The number of elements in the internal pool.
alignment | An alignment boundary, must be a power of two.

## Return Value

Returns a memory pool pointer.

## Related Pages

[cf_destroy_memory_pool](/allocator/cf_destroy_memory_pool.md)  
[cf_memory_pool_alloc](/allocator/cf_memory_pool_alloc.md)  
[cf_memory_pool_try_alloc](/allocator/cf_memory_pool_try_alloc.md)  
[cf_memory_pool_free](/allocator/cf_memory_pool_free.md)  
