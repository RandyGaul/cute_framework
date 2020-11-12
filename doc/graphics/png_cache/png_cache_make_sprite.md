
# png_cache_make_sprite

Makes a sprite.

## Syntax

```cpp
sprite_t png_cache_make_sprite(png_cache_t* cache, const char* sprite_name, const animation_table_t* table = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
cache | The png cache.
sprite_name | Name of the sprite.
table | The table of animations for the sprite to use. You can supply the pointer to the animation table yourself in `table`, or just leave it NULL. If table is NULL then `sprite_name` is used to lookup the table within the png cache.

## Return Value

Returns the new sprite.

## Remarks

Each sprite must refer to an animation table previously constructed by [png_cache_make_animation_table](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_make_animation_table.md). 

## Related Functions
  
[png_cache_make_animation_table](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_make_animation_table.md)  
