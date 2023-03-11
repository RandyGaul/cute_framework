[](../header.md ':include')

# cf_aligned_free

Category: [allocator](/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

Frees a block of memory previously allocated by [cf_aligned_alloc](/allocator/cf_aligned_alloc.md).

```cpp
CF_API void CF_CALL cf_aligned_free(void* ptr);
```

Parameters | Description
--- | ---
ptr | The memory to deallocate.

## Remarks

Aligned allocation is mostly useful as a performance optimization, or for SIMD operations that require byte alignments.

## Related Pages

[cf_aligned_alloc](/allocator/cf_aligned_alloc.md)  
