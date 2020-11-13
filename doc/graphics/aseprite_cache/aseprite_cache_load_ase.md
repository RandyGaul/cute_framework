
# aseprite_cache_load_ase

A low-level function used to return an `ase_t` from the cache. If it does not exist within the cache it is loaded from disk.

## Syntax

```cpp
error_t aseprite_cache_load_ase(aseprite_cache_t* cache, const char* aseprite_path, ase_t** ase);
```

## Function Parameters

Parameter Name | Description
--- | ---
cache | The cache to load from.
aseprite_path | Path to the aseprite file.
ase | The raw `ase_t` loaded from the aseprite file.

## Return Value

Returns any error details on failure.

## Remarks

This function is typically not necessary to call. You might be looking for `aseprite_cache_load` instead. Only call this function if you know what you're doing.

## Related Functions
  
[aseprite_cache_load](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_load.md)  
