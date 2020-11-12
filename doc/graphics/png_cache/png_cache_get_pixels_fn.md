
# png_cache_get_pixels_fn

This function is needed to hook up to `batch_t` in order to draw sprites.

## Syntax

```cpp
get_pixels_fn* png_cache_get_pixels_fn(png_cache_t* cache);
```

## Function Parameters

Parameter Name | Description
--- | ---
cache | The png cache to hookup to `batch_t`.

## Return Value

The return value gets passed to [batch_make](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/batch/batch_make.md). 
