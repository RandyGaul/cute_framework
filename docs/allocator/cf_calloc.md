[](../header.md ':include')

# cf_calloc

Category: [allocator](/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

Allocates a block of memory `size  count` bytes in size.

```cpp
CF_API void* CF_CALL cf_calloc(size_t size, size_t count);
```

## Remarks

The memory returned is completely zero'd out. Generally this is more efficient than calling `cf_malloc` and
then clearing the memory to zero yourself. Though, it's not a concern for most games.

## Related Pages

[cf_allocator_override](/allocator/cf_allocator_override.md)  
[cf_allocator_restore_default](/allocator/cf_allocator_restore_default.md)  
[cf_alloc](/allocator/cf_alloc.md)  
[cf_free](/allocator/cf_free.md)  
[cf_realloc](/allocator/cf_realloc.md)  
