[](../header.md ':include')

# cf_sprite_reload

Category: [sprite](/api_reference?id=sprite)  
GitHub: [cute_sprite.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_sprite.h)  
---

Reloads the sprite's pixels from disk.

```cpp
CF_Sprite cf_sprite_reload(const CF_Sprite* sprite);
```

Parameters | Description
--- | ---
sprite | The sprite to reload.

## Return Value

The reloaded sprite.

## Remarks

This function is designed to help support asset or image hotloading/reloading during development.
This function is not designed to be called once you ship your game.
All old instances of the sprite are now invalid and should be reset to this return value.

## Related Pages

[CF_Sprite](/sprite/cf_sprite.md)  
[cf_make_sprite](/sprite/cf_make_sprite.md)  
[cf_sprite_unload](/sprite/cf_sprite_unload.md)  
