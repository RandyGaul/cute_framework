# cf_memory_pool_alloc | [allocator](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/README.md) | [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)

Allocates a chunk of memory from the pool. The allocation size was determined by `element_size` in [cf_make_memory_pool](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_make_memory_pool.md).

```cpp
void* cf_memory_pool_alloc(CF_MemoryPool* pool);
```

Parameters | Description
--- | ---
pool | The pool.

## Return Value

Returns an aligned pointer of `size` bytes.

## Remarks

Attempts to allocate from the internal pool. If the pool is empty a call to `malloc` is made as a backup. All
backup allocations are not tracked anywhere, so you must call [cf_memory_pool_free](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_memory_pool_free.md) on each allocation to be
sure they all properly cleaned up.

## Related Pages

[cf_make_memory_pool](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_make_memory_pool.md)  
[cf_destroy_memory_pool](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_destroy_memory_pool.md)  
[cf_memory_pool_try_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_memory_pool_try_alloc.md)  
[cf_memory_pool_free](https://github.com/RandyGaul/cute_framework/blob/master/docs/allocator/cf_memory_pool_free.md)  
