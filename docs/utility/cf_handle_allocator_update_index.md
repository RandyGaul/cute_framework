[](../header.md ':include')

# cf_handle_allocator_update_index

Category: [utility](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=utility)  
GitHub: [cute_handle_table.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_handle_table.h)  
---

Sets 32-bit index value a handle maps to.

```cpp
void cf_handle_allocator_update_index(CF_HandleTable* table, CF_Handle handle, uint32_t index);
```

Parameters | Description
--- | ---
table | The table.
handle | A handle created by [cf_handle_allocator_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_alloc.md).
index | A new 32-bit index value to store.

## Related Pages

[CF_Handle](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle.md)  
[CF_HandleTable](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handletable.md)  
[cf_handle_allocator_get_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_get_index.md)  
[cf_handle_allocator_get_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_get_index.md)  
[cf_handle_allocator_get_type](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_get_type.md)  
[cf_handle_allocator_active](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_active.md)  
[cf_handle_allocator_activate](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_activate.md)  
[cf_handle_allocator_deactivate](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_deactivate.md)  
[cf_handle_allocator_handle_valid](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_handle_valid.md)  
[cf_handle_allocator_free](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_free.md)  
