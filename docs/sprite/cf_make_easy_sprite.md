# cf_make_easy_sprite

Category: [sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=sprite)  
GitHub: [cute_sprite.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_sprite.h)  
---

Loads a single-frame sprite from a single png file.

```cpp
CF_Sprite cf_make_easy_sprite(const char* png_path);
```

Parameters | Description
--- | ---
png_path | Virtual path to the .png file.

## Return Value

Returns a [CF_Sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite.md) that can be drawn with `cf_sprite_draw`. The sprite is not animated,
as it's only a single-frame made from a png file.

## Remarks

The preferred way to make a sprite is [cf_make_sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_make_sprite.md), but this function provides a very simple way to get started
by merely loading a single .png image, or for games that don't require animations. TODO - LINK_TO_VFS_TUTORIAL.

## Related Pages

[CF_Sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_sprite.md)  
[cf_make_sprite](https://github.com/RandyGaul/cute_framework/blob/master/docs/sprite/cf_make_sprite.md)  
