[//]: # (This file is automatically generated by Cute Framework's docs parser.)
[//]: # (Do not edit this file by hand!)
[//]: # (See: https://github.com/RandyGaul/cute_framework/blob/master/samples/docs_parser.cpp)
[](../header.md ':include')

# cf_png_cache_load

Category: [png_cache](/api_reference?id=png_cache)  
GitHub: [cute_png_cache.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_png_cache.h)  
---

Returns an image [CF_Png](/png_cache/cf_png.md) from the cache.

```cpp
CF_Result cf_png_cache_load(const char* png_path, CF_Png* png /*= NULL*/);
```

## Remarks

If it does not exist in the cache, it is loaded from disk and placed into the cache.

## Related Pages

[CF_Png](/png_cache/cf_png.md)  
[cf_png_defaults](/png_cache/cf_png_defaults.md)  
[cf_make_png_cache_sprite](/png_cache/cf_make_png_cache_sprite.md)  
[cf_make_png_cache_animation](/png_cache/cf_make_png_cache_animation.md)  
