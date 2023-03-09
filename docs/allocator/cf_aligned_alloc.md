[](../header.md ':include')

# cf_aligned_alloc

Category: [allocator](/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

Allocates a block of memory aligned along a byte boundary.

```cpp
void* cf_aligned_alloc(size_t size, int alignment);
```

Parameters | Description
--- | ---
size | The size of the allocation.
alignment | An alignment boundary, must be a power of two.

## Return Value

Returns an aligned pointer of `size` bytes.

## Remarks

Aligned allocation is mostly useful as a performance optimization, or for SIMD operations that require byte alignments.

## Related Pages

cf_aligned_free  
