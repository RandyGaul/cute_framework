[](../header.md ':include')

# cf_sprite_unload

Category: [sprite](/api_reference?id=sprite)  
GitHub: [cute_sprite.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_sprite.h)  
---

Unloads the sprite's image resources from the internal cache.

```cpp
CF_API void CF_CALL cf_sprite_unload(const char* aseprite_path);
```

Parameters | Description
--- | ---
aseprite_path | Virtual path to a .ase file.

## Remarks

Any live [CF_Sprite](/sprite/cf_sprite.md) instances for `aseprite_path` will now by "dangling". TODO - LINK_TO_VFS_TUTORIAL.

## Related Pages

[CF_Sprite](/sprite/cf_sprite.md)  
[cf_make_sprite](/sprite/cf_make_sprite.md)  
