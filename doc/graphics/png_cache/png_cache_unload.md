
# png_cache_unload

Unloads an image from the cache.

## Syntax

```cpp
void png_cache_unload(png_cache_t* cache, png_t* png);
```

## Function Parameters

Parameter Name | Description
--- | ---
cache | The png cache to unload from.
png | The loaded PNG to be removed from the cache.

## Remarks

This function can be used to control your RAM usage, for example when switching from one level/area to another can be a good time to unload images that will no longer be used.

## Related Functions
  
[png_cache_load](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_load.md)  
[png_cache_load_mem](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_load_mem.md)  
