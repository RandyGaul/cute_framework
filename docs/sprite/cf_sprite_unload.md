# cf_sprite_unload | [sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/README.md) | [cute_sprite.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_sprite.h)

Unloads the sprite's image resources from the internal cache.

```cpp
void cf_sprite_unload(const char* aseprite_path);
```

Parameters | Description
--- | ---
aseprite_path | Virtual path to a .ase file.

## Remarks

Any live [CF_Sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite.md) instances for `aseprite_path` will now by "dangling". TODO - LINK_TO_VFS_TUTORIAL.

## Related Pages

[CF_Sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite.md)  
[cf_make_sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_make_sprite.md)  
