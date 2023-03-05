# cf_handle_allocator_handle_valid | [utility](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/README.md) | [cute_handle_table.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_handle_table.h)

Returns true if a [CF_Handle](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle.md) is valid.

```cpp
int cf_handle_allocator_handle_valid(CF_HandleTable* table, CF_Handle handle);
```

Parameters | Description
--- | ---
table | The table.
handle | A handle created by [cf_handle_allocator_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_alloc.md).

## Remarks

Handles are created in a valid state. They only become invalid when [cf_handle_allocator_free](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_free.md) is called.

## Related Pages

[CF_Handle](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle.md)  
[CF_HandleTable](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handletable.md)  
[cf_handle_allocator_get_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_get_index.md)  
[cf_handle_allocator_get_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_get_index.md)  
[cf_handle_allocator_get_type](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_get_type.md)  
[cf_handle_allocator_active](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_active.md)  
[cf_handle_allocator_activate](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_activate.md)  
[cf_handle_allocator_deactivate](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_deactivate.md)  
[cf_handle_allocator_update_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_update_index.md)  
[cf_handle_allocator_free](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_free.md)  
