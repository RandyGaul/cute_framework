[](../header.md ':include')

# cf_handle_allocator_deactivate

Category: [utility](/api_reference?id=utility)  
GitHub: [cute_handle_table.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_handle_table.h)  
---

Sets the active state of a handle to false.

```cpp
CF_API void CF_CALL cf_handle_allocator_deactivate(CF_HandleTable* table, CF_Handle handle);
```

Parameters | Description
--- | ---
table | The table.
handle | A handle created by [cf_handle_allocator_alloc](/utility/cf_handle_allocator_alloc.md).

## Remarks

Handles are initially created in an active state. You can toggle the active state with [cf_handle_allocator_activate](/utility/cf_handle_allocator_activate.md) or [cf_handle_allocator_deactivate](/utility/cf_handle_allocator_deactivate.md).
The active state _does not affect_ [cf_handle_allocator_handle_valid](/utility/cf_handle_allocator_handle_valid.md).

## Related Pages

[CF_Handle](/utility/cf_handle.md)  
[CF_HandleTable](/utility/cf_handletable.md)  
[cf_handle_allocator_get_index](/utility/cf_handle_allocator_get_index.md)  
[cf_handle_allocator_get_index](/utility/cf_handle_allocator_get_index.md)  
[cf_handle_allocator_get_type](/utility/cf_handle_allocator_get_type.md)  
[cf_handle_allocator_active](/utility/cf_handle_allocator_active.md)  
[cf_handle_allocator_activate](/utility/cf_handle_allocator_activate.md)  
[cf_handle_allocator_handle_valid](/utility/cf_handle_allocator_handle_valid.md)  
[cf_handle_allocator_update_index](/utility/cf_handle_allocator_update_index.md)  
[cf_handle_allocator_free](/utility/cf_handle_allocator_free.md)  
