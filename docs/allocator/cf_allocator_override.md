[](../header.md ':include')

# cf_allocator_override

Category: [allocator](/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

Overrides the default allocator with a custom one.

```cpp
CF_API void CF_CALL cf_allocator_override(CF_Allocator allocator);
```

## Remarks

The default allocator simply calls malloc/free and friends. You may override this behavior by passing
a [CF_Allocator](/allocator/cf_allocator.md) to this function. This lets you hook up your own custom allocator. Usually you only want
to do this on certain platforms for performance optimizations, but is not a necessary thing to do for many games.

## Related Pages

[cf_realloc](/allocator/cf_realloc.md)  
[cf_allocator_restore_default](/allocator/cf_allocator_restore_default.md)  
[cf_alloc](/allocator/cf_alloc.md)  
[cf_free](/allocator/cf_free.md)  
[cf_calloc](/allocator/cf_calloc.md)  
