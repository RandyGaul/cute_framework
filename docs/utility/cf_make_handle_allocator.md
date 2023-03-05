# cf_make_handle_allocator | [utility](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility_readme.md) | [cute_handle_table.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_handle_table.h)

Returns a new [CF_HandleTable](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handletable.md).

```cpp
CF_HandleTable* cf_make_handle_allocator(int initial_capacity);
```

Parameters | Description
--- | ---
initial_capacity | The initial number of handles to store within. Grows internally as necessary.

## Remarks

The handle table is used to generate [CF_Handle](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle.md)s with [cf_handle_allocator_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_alloc.md). Each handle is unique and maps
to a 32-bit index along with a 16-bit type value. This is useful for adding a layer of indirection to systems that
want to track object lifetimes, grant access to objects, while also decoupling object memory storage from their
lifetime cycles. Free it up with [cf_destroy_handle_allocator](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_destroy_handle_allocator.md) when done.

## Related Pages

[CF_Handle](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle.md)  
[CF_HandleTable](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handletable.md)  
[cf_destroy_handle_allocator](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_destroy_handle_allocator.md)  
[cf_handle_allocator_alloc](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_handle_allocator_alloc.md)  
