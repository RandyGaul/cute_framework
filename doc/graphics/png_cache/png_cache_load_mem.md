
# png_cache_load_mem

Returns an image from the cache. If it does not exist in the cache, it is loaded from memory and placed into the cache.

## Syntax

```cpp
error_t png_cache_load_mem(png_cache_t* cache, const char* png_path, const void* memory, size_t size, png_t* png = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
cache | The png cache to load from.
png_path | Path to the png file, in this case is treated as the "name" of the PNG.
memory | Pointer to the raw PNG bytes in memory.
size | Size of the buffer at `memory`.
png | The loaded PNG. This parameter can be `NULL`, in which `png_cache_load` will be like a RAM prefetching function.

## Return Value

Returns any error details on failure.

## Related Functions
  
[png_cache_load](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_load.md)  
[png_cache_unload](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_unload.md)  
