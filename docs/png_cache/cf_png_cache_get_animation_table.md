[](../header.md ':include')

# cf_png_cache_get_animation_table

Category: [png_cache](/api_reference?id=png_cache)  
GitHub: [cute_png_cache.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_png_cache.h)  
---

Looks up an animation table within the png cache by name.

```cpp
const htbl CF_Animation** cf_png_cache_get_animation_table(const char* sprite_name);
```

Parameters | Description
--- | ---
sprite_name | A unique name for the animation table.

## Return Value

Returns a hashtable of unique sprite names to [CF_Animation](/sprite/cf_animation.md)'s, see [cf_make_png_cache_sprite](/png_cache/cf_make_png_cache_sprite.md).

## Related Pages

[CF_Png](/png_cache/cf_png.md)  
[cf_png_cache_load](/png_cache/cf_png_cache_load.md)  
[cf_make_png_cache_animation](/png_cache/cf_make_png_cache_animation.md)  
[cf_make_png_cache_animation_table](/png_cache/cf_make_png_cache_animation_table.md)  
[cf_make_png_cache_sprite](/png_cache/cf_make_png_cache_sprite.md)  
