
# aseprite_cache_make

Constructs a new aseprite cache. Destroy it with [aseprite_cache_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_destroy.md) when done with it.

## Syntax

```cpp
aseprite_cache_t* aseprite_cache_make(void* mem_ctx = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
mem_ctx | Used for custom allocators, this can be set to `NULL`. See (TODO) for more details.

## Return Value

Returns the new aseprite cache.

## Related Functions
  
[aseprite_cache_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_destroy.md)  
