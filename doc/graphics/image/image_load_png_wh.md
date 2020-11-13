
# image_load_png_wh

An optimized function to only load the w/h out of a png file in-memory.

## Syntax

```cpp
error_t image_load_png_wh(const void* data, int size, int* w, int* h);
```

## Function Parameters

Parameter Name | Description
--- | ---
data | Pointer to the buffer to load from.
size | Size of the `data` buffer in bytes.
w | Width of the png.
h | Height of the png.

## Return Value

Returns any error details on failure.

## Related Functions
  
[image_load_png](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/image_load_png.md)  
[image_load_png_mem](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/image_load_png_mem.md)  
[image_free](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/image_free.md)  
