# get_pixels_fn

A function pointer used by the batching system to periodically fetch pixels for a corresponding image id.

## Syntax

```cpp
typedef void (get_pixels_fn)(uint64_t image_id, void* buffer, int bytes_to_fill, void* udata);
```

## Function Parameters

Parameter Name | Description
--- | ---
image_id | Uniquely maps to a single image, as determined by you.
buffer | Pointer to the memory where you need to fill in pixel data.
bytes_to_fill | Number of bytes to write to `buffer`.
udata | The `udata` pointer that was originally passed to [batch_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/batch/batch_make.md) as `get_pixels_udata`.

## Remarks

`get_pixels_fn` will be called periodically from within [batch_flush](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_flush.md) whenever access to pixels in RAM are needed to construct internal texture atlases to be sent to the GPU.

Both [aseprite cache](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_get_pixels_fn.md) and [png cache](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_get_pixels_fn.md) provide a way to supply this function pointer. The batch will periodically fetch pixels from the cache on an as-needed basis, depending on what sprites you're currently drawing.
 
 ## Related Functions
 
 [batch_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/batch/batch_make.md)  
 [aseprite_cache_get_pixels_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_get_pixels_fn.md)  
 [png_cache_get_pixels_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_get_pixels_fn.md)  
 
