
# image_load_png_mem_indexed

Loads an indexed (paletted) png from memory.

## Syntax

```cpp
error_t image_load_png_mem_indexed(const void* data, int size, image_t* img, void* user_allocator_context = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
data | Pointer to the buffer to load from.
size | Size of the `data` buffer in bytes.
img | The output image once loaded.
user_allocator_context | Used for custom allocators, this can be set to `NULL`. See (TODO) for more details.

## Return Value

Returns any error details on failure.

## Related Functions
  
[image_load_png_indexed](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/image_load_png_indexed.md)  
[image_load_png_wh](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/image_load_png_wh.md)  
[image_free](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/image_free.md)  
