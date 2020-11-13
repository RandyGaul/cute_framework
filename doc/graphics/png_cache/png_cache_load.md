
# png_cache_load

Returns an image from the cache. If it does not exist in the cache, it is loaded from disk and placed into the cache.

## Syntax

```cpp
error_t png_cache_load(png_cache_t* cache, const char* png_path, png_t* png = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
cache | The png cache to load from.
png_path | Path to the png file.
png | The loaded PNG. This parameter can be `NULL`, in which `png_cache_load` will be like a RAM prefetching function.

## Return Value

Returns any error details on failure.

## Related Functions
  
[png_cache_load_mem](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_load_mem.md)  
[png_cache_unload](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_unload.md)  
