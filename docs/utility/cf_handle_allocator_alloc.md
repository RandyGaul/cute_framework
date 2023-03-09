[](../header.md ':include')

# cf_handle_allocator_alloc

Category: [utility](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=utility)  
GitHub: [cute_handle_table.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_handle_table.h)  
---

Allocates and returns a unique [CF_Handle](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle.md) that maps to `index` and `type`.

```cpp
CF_Handle cf_handle_allocator_alloc(CF_HandleTable* table, uint32_t index, uint16_t type);
```

Parameters | Description
--- | ---
table | The table.
index | A 32-bit value the handle maps to. Can be fetched with [cf_handle_allocator_get_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_get_index.md).
type | A 16-bit value the handle maps to. Can be fetched with [cf_handle_allocator_get_type](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_get_type.md).

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
[cf_handle_allocator_handle_valid](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_handle_valid.md)  
