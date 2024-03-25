[](../header.md ':include')

# CF_HandleTable

Category: [utility](/api_reference?id=utility)  
GitHub: [cute_handle_table.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_handle_table.h)  
---

An opaque pointer representing a handle table.

## Remarks

The handle table stores a lookup table of unique [CF_Handle](/utility/cf_handle.md)s. Handles use a lookup table mechanism
to map one integer to another. This is useful to implement object pools or other similar kinds of
data structures. This is a rather low-level utility, mainly used to implement CF's ECS. If you want
to see it in action you may peruse the CF source code. Otherwise, it's assumed you know what you're
doing if you're reading this.

## Related Pages

[cf_handle_allocator_get_index](/utility/cf_handle_allocator_get_index.md)  
[CF_Handle](/utility/cf_handle.md)  
[cf_make_handle_allocator](/utility/cf_make_handle_allocator.md)  
[cf_handle_allocator_alloc](/utility/cf_handle_allocator_alloc.md)  
