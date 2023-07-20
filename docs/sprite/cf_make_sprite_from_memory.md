[](../header.md ':include')

# cf_make_sprite_from_memory

Category: [sprite](/api_reference?id=sprite)  
GitHub: [cute_sprite.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_sprite.h)  
---

Loads a sprite from an aseprite file already in memory.

```cpp
CF_Sprite cf_make_sprite_from_memory(const char* unique_name, const void* aseprite_data, int size);
```

Parameters | Description
--- | ---
unique_name | A completely unique string from all other sprites.

## Return Value

Returns a [CF_Sprite](/sprite/cf_sprite.md) that can be drawn with `cf_sprite_draw`.

## Remarks

This function caches the sprite internally. Subsequent calls to load the same sprite will be very fast; you can use
this function directly to fetch sprites that were already loaded. If you want to load sprites with your own custom
animation data, instead of using the .ase/.aseprite format, you can try out `cf_png_cache_load` for a more low-level option.
See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

## Related Pages

[CF_Sprite](/sprite/cf_sprite.md)  
[cf_make_easy_sprite_from_png](/sprite/cf_make_easy_sprite_from_png.md)  
[cf_make_easy_sprite_from_pixels](/sprite/cf_make_easy_sprite_from_pixels.md)  
[cf_easy_sprite_update_pixels](/sprite/cf_easy_sprite_update_pixels.md)  
