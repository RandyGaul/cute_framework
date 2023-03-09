[](../header.md ':include')

# cf_handle_allocator_handle_valid

Category: [utility](/api_reference?id=utility)  
GitHub: [cute_handle_table.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_handle_table.h)  
---

Returns true if a [CF_Handle](/utility/cf_handle.md) is valid.

```cpp
int cf_handle_allocator_handle_valid(CF_HandleTable* table, CF_Handle handle);
```

Parameters | Description
--- | ---
table | The table.
handle | A handle created by [cf_handle_allocator_alloc](/utility/cf_handle_allocator_alloc.md).

## Remarks

Handles are created in a valid state. They only become invalid when [cf_handle_allocator_free](/utility/cf_handle_allocator_free.md) is called.

## Related Pages

[CF_Handle](/utility/cf_handle.md)  
[CF_HandleTable](/utility/cf_handletable.md)  
[cf_handle_allocator_get_index](/utility/cf_handle_allocator_get_index.md)  
[cf_handle_allocator_get_index](/utility/cf_handle_allocator_get_index.md)  
[cf_handle_allocator_get_type](/utility/cf_handle_allocator_get_type.md)  
[cf_handle_allocator_active](/utility/cf_handle_allocator_active.md)  
[cf_handle_allocator_activate](/utility/cf_handle_allocator_activate.md)  
[cf_handle_allocator_deactivate](/utility/cf_handle_allocator_deactivate.md)  
[cf_handle_allocator_update_index](/utility/cf_handle_allocator_update_index.md)  
[cf_handle_allocator_free](/utility/cf_handle_allocator_free.md)  
