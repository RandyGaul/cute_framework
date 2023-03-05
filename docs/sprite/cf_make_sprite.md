# cf_make_sprite | [sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/README.md) | [cute_sprite.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_sprite.h)

Loads a sprite from an aseprite file.

```cpp
CF_Sprite cf_make_sprite(const char* aseprite_path);
```

Parameters | Description
--- | ---
aseprite_path | Virtual path to a .ase file.

## Return Value

Returns a [CF_Sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite.md) that can be drawn with `cf_sprite_draw`.

## Remarks

This function caches the sprite internally. Subsequent calls to load the same sprite will be very fast; you can use
this function directly to fetch sprites that were already loaded. If you want to load sprites with your own custom
animation data, instead of using the .ase/.aseprite format, you can try out `cf_png_cache_load` for a more low-level option.
TODO - LINK_TO_VFS_TUTORIAL.

## Related Pages

[CF_Sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite.md)  
[cf_sprite_unload](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite_unload.md)  
