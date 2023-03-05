# cf_memory_pool_free | [allocator](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/README.md) | [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)

Frees an allocation made by [cf_memory_pool_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_memory_pool_alloc.md) or [cf_memory_pool_try_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_memory_pool_try_alloc.md).

```cpp
void cf_memory_pool_free(CF_MemoryPool* pool, void* element);
```

Parameters | Description
--- | ---
pool | The pool.
element | The pointer to deallocate.

## Related Pages

[cf_make_memory_pool](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_make_memory_pool.md)  
[cf_destroy_memory_pool](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_destroy_memory_pool.md)  
[cf_memory_pool_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_memory_pool_alloc.md)  
[cf_memory_pool_try_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_memory_pool_try_alloc.md)  
