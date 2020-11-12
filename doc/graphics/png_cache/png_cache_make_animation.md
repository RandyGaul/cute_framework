
# png_cache_make_animation

Constructs an animation out of an array of frames, along with their delays in milliseconds. The animation is stored within the png cache.

## Syntax

```cpp
const animation_t* png_cache_make_animation(png_cache_t* cache, const char* name, const array<png_t>& pngs, const array<float>& delays);
```

## Function Parameters

Parameter Name | Description
--- | ---
cache | The png cache.
name | Name of the animation. This will be used to lookup the animation within animation tables.
pngs | The images for each frame of this animation.
delays | The delays (in milliseconds) for each frame of this animation.

## Related Functions
  
[png_cache_get_animation](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_get_animation.md)  
