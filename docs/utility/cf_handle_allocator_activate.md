# cf_handle_allocator_activate

Category: [utility](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=utility)  
GitHub: [cute_handle_table.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_handle_table.h)  
---

Sets the active state of a handle to true.

```cpp
void cf_handle_allocator_activate(CF_HandleTable* table, CF_Handle handle);
```

Parameters | Description
--- | ---
table | The table.
handle | A handle created by [cf_handle_allocator_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_alloc.md).

## Remarks

Handles are initially created in an active state. You can toggle the active state with [cf_handle_allocator_activate](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_activate.md) or [cf_handle_allocator_deactivate](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_deactivate.md).
The active state _does not affect_ [cf_handle_allocator_handle_valid](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_handle_valid.md).

## Related Pages

[CF_Handle](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle.md)  
[CF_HandleTable](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handletable.md)  
[cf_handle_allocator_get_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_get_index.md)  
[cf_handle_allocator_get_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_get_index.md)  
[cf_handle_allocator_get_type](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_get_type.md)  
[cf_handle_allocator_active](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_active.md)  
[cf_handle_allocator_handle_valid](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_handle_valid.md)  
[cf_handle_allocator_deactivate](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_deactivate.md)  
[cf_handle_allocator_update_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_update_index.md)  
[cf_handle_allocator_free](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_free.md)  
