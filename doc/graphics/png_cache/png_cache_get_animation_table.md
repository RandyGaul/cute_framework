
# png_cache_get_animation_table

Looks up an animation table within the png cache by name.

## Syntax

```cpp
const animation_table_t* png_cache_get_animation_table(png_cache_t* cache, const char* sprite_name);
```

## Function Parameters

Parameter Name | Description
--- | ---
cache | The png cache.
sprite_name | Name of the sprite.

## Return Value

Returns the animation table.

## Related Functions
  
[png_cache_make_animation_table](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_make_animation_table.md)  
