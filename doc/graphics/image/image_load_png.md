
# image_load_png

Loads a png from disk.

## Syntax

```cpp
error_t image_load_png(const char* path, image_t* img, void* user_allocator_context = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
path | Path to the file to load.
img | The output image once loaded.
user_allocator_context | Used for custom allocators, this can be set to `NULL`. See (TODO) for more details.

## Return Value

Returns any error details on failure.

## Related Functions
  
[image_load_png_mem](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/image_load_png_mem.md)  
[image_load_png_wh](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/image_load_png_wh.md)  
[image_free](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/image_free.md)  
