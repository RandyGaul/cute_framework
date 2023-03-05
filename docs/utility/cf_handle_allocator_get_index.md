# cf_handle_allocator_get_index | [utility](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility_readme.md) | [cute_handle_table.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_handle_table.h)

Returns the 32-bit index value a [CF_Handle](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle.md) maps to.

```cpp
uint32_t cf_handle_allocator_get_index(CF_HandleTable* table, CF_Handle handle);
```

Parameters | Description
--- | ---
table | The table.
handle | A handle created by [cf_handle_allocator_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_alloc.md).

## Remarks

This function will return a valid value if the handle is valid. Check for validity with [cf_handle_allocator_handle_valid](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_handle_valid.md). The
function [cf_handle_allocator_active](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_active.md) does not affect handle validity.

## Related Pages

[CF_Handle](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle.md)  
[CF_HandleTable](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handletable.md)  
[cf_handle_allocator_handle_valid](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_handle_valid.md)  
[cf_handle_allocator_free](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_free.md)  
[cf_handle_allocator_get_type](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_get_type.md)  
[cf_handle_allocator_active](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_active.md)  
[cf_handle_allocator_activate](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_activate.md)  
[cf_handle_allocator_deactivate](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_deactivate.md)  
[cf_handle_allocator_update_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_update_index.md)  
