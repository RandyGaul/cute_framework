
# png_cache_make_animation_table

Constructs an animation table given an array of animations. The table is stored within the png cache.

## Syntax

```cpp
const animation_table_t* png_cache_make_animation_table(png_cache_t* cache, const char* sprite_name, const array<const animation_t*>& animations);
```

## Function Parameters

Parameter Name | Description
--- | ---
cache | The png cache.
sprite_name | Name of the sprite.
animations | Array of animations. These will construct the key-value table returned, where keys are animation names, and values are `animation_t` pointers.

## Return Value

Returns the constructed animation table.

## Remarks

Each animation in the returned table is key'd by its own name in this table. The sprite can pick which animation to play by calling [sprite_t::play](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sprite/play.md).

## Related Functions
  
[png_cache_make_animation](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_make_animation.md)  
[png_cache_get_animation_table](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_get_animation_table.md)  
