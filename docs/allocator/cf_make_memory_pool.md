# cf_make_memory_pool | [allocator](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/README.md) | [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)

Creates a memory pool.

```cpp
CF_MemoryPool* cf_make_memory_pool(int element_size, int element_count, int alignment);
```

Parameters | Description
--- | ---
element_size | The size of each allocation.
element_count | The number of elements in the internal pool.
alignment | An alignment boundary, must be a power of two.

## Return Value

Returns a memory pool pointer.

## Related Pages

[cf_destroy_memory_pool](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_destroy_memory_pool.md)  
[cf_memory_pool_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_memory_pool_alloc.md)  
[cf_memory_pool_try_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_memory_pool_try_alloc.md)  
[cf_memory_pool_free](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_memory_pool_free.md)  
