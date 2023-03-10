[](../header.md ':include')

# cf_handle_allocator_get_index

Category: [utility](/api_reference?id=utility)  
GitHub: [cute_handle_table.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_handle_table.h)  
---

Returns the 32-bit index value a [CF_Handle](/utility/cf_handle.md) maps to.

```cpp
CF_API uint32_t CF_CALL cf_handle_allocator_get_index(CF_HandleTable* table, CF_Handle handle);
```

Parameters | Description
--- | ---
table | The table.
handle | A handle created by [cf_handle_allocator_alloc](/utility/cf_handle_allocator_alloc.md).

## Remarks

This function will return a valid value if the handle is valid. Check for validity with [cf_handle_allocator_handle_valid](/utility/cf_handle_allocator_handle_valid.md). The
function [cf_handle_allocator_active](/utility/cf_handle_allocator_active.md) does not affect handle validity.

## Related Pages

[CF_Handle](/utility/cf_handle.md)  
[CF_HandleTable](/utility/cf_handletable.md)  
[cf_handle_allocator_handle_valid](/utility/cf_handle_allocator_handle_valid.md)  
[cf_handle_allocator_free](/utility/cf_handle_allocator_free.md)  
[cf_handle_allocator_get_type](/utility/cf_handle_allocator_get_type.md)  
[cf_handle_allocator_active](/utility/cf_handle_allocator_active.md)  
[cf_handle_allocator_activate](/utility/cf_handle_allocator_activate.md)  
[cf_handle_allocator_deactivate](/utility/cf_handle_allocator_deactivate.md)  
[cf_handle_allocator_update_index](/utility/cf_handle_allocator_update_index.md)  
