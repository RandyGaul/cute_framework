
# png_cache_make

Constructs a new png cache. Destroy it with [png_cache_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_destroy.md) when done with it.

## Syntax

```cpp
png_cache_t* png_cache_make(void* mem_ctx = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
mem_ctx | Used for custom allocators, this can be set to `NULL`. See (TODO) for more details.

## Return Value

Returns the new png cache.

## Related Functions
  
[png_cache_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_destroy.md)  
