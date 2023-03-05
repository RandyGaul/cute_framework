
# aseprite_cache_get_strpool_ptr

Returns a pointer to the strpool used by this png cache.

## Syntax

```cpp
strpool_t* aseprite_cache_get_strpool_ptr(aseprite_cache_t* cache);
```

## Function Parameters

Parameter Name | Description
--- | ---
cache | The png cache.

## Remarks

This is a low-level function, just in case anyone wants to get access to the internal string pool. Only use this function if you know what you're doing.
