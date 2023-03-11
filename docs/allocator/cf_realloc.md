[](../header.md ':include')

# cf_realloc

Category: [allocator](/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

Reallocates a block of memory to a new size.

```cpp
CF_API void* CF_CALL cf_realloc(void* ptr, size_t size);
```

## Remarks

You must reassign your old pointer! Generally this is more efficient than calling `cf_malloc`, [cf_free](/allocator/cf_free.md), and
`CF_MEMCPY` yourself. Though, this is not a concern for most games.

## Related Pages

[cf_allocator_override](/allocator/cf_allocator_override.md)  
[cf_allocator_restore_default](/allocator/cf_allocator_restore_default.md)  
[cf_alloc](/allocator/cf_alloc.md)  
[cf_free](/allocator/cf_free.md)  
[cf_calloc](/allocator/cf_calloc.md)  
