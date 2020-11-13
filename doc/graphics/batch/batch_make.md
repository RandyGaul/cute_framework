# batch_make

Creates a new batch.

## Syntax

```cpp
batch_t* batch_make(get_pixels_fn* get_pixels, void* get_pixels_udata, void* mem_ctx = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
get_pixels | Callback to periodically fetching pixels by id.
get_pixels_udata | The userdata pointer passed to `get_pixels` whenever it is called.
mem_ctx | Used for custom allocators, this can be set to `NULL`. See (TODO) for more details.

## Return Value

Returns a new batch.

## Related Functions

[batch_destroy](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_destroy)  
