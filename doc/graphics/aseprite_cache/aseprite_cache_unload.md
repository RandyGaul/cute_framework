
# aseprite_cache_unload

Unloads an aseprite from the cache.

## Syntax

```cpp
void aseprite_cache_unload(aseprite_cache_t* cache, const char* aseprite_path);
```

## Function Parameters

Parameter Name | Description
--- | ---
cache | The cache to unload from.
aseprite_path | The loaded aseprite to be removed from the cache.

## Remarks

This function can be used to control your RAM usage, for example when switching from one level/area to another can be a good time to unload images that will no longer be used.

## Related Functions
  
[aseprite_cache_load](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_load.md)  
