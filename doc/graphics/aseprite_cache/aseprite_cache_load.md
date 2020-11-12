
# png_cache_load

Returns an sprite from the cache. If it does not exist in the cache, it is loaded from disk and placed into the cache.

## Syntax

```cpp
error_t aseprite_cache_load(aseprite_cache_t* cache, const char* aseprite_path, sprite_t* sprite);
```

## Function Parameters

Parameter Name | Description
--- | ---
cache | The cache to load from.
aseprite_path | Path to the aseprite file.
sprite | The loaded sprite.

## Return Value

Returns any error details on failure.

## Related Functions
  
[aseprite_cache_load](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_load.md)  
