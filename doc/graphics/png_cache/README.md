# PNG Cache

The png cache is used to load png images from disk in order to make sprites. You will mostly just care about these three functions.

* [png_cache_load](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_load.md)
* [png_cache_unload](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_unload.md)
* [png_cache_make_sprite](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_make_sprite.md)

It's a cache, which means it actually caches images loaded in RAM, so subsequent calls to [png_cache_load](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_load.md) won't have to fetch the image off of disk, as long as the image is currently cached in RAM.

The purpose of the cache is to hook up to Cute's batching mechanism. The function to do so is [png_cache_get_pixels_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_get_pixels_fn.md).

## Animations and Sprites

Since png files do not contain any kind of animation information (frame delays or sets of frames) you must specify all of the animation data yourself in order to make sprites. The various functions in this section are for setting up animation data. If you don't want to call all of these fairly complicated functions, a simpler alternative would be to use the [aseprite_cache](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/aseprite_cache), as aseprite files contain animation data inside of them. The png cache is basically a lower-level version of the aseprite cache.

* [png_cache_make_animation](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_make_animation.md)
* [png_cache_get_animation](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_get_animation.md)
* [png_cache_make_animation_table](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_make_animation_table.md)
* [png_cache_get_animation_table](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_get_animation_table.md)
* [png_cache_make_sprite](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_make_sprite.md)

The flow is to follow these steps by calling the above functions.

1. Make an animation. This is an array of frames with a name.
2. Make an animation table. This is a collection of animations in key-value form, key'd by the animation names.
3. Make a sprite by providing an animation table. The sprite references the table by pointer, and reads from it periodically to draw itself on screen.

## List of all PNG Functions

[png_cache_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_make.md)  
[png_cache_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_destroy.md)  
[png_cache_load](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_load.md)  
[png_cache_load_mem](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_load_mem.md)  
[png_cache_unload](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_unload.md)  
[png_cache_get_pixels_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_get_pixels_fn.md)  
[png_cache_get_strpool_ptr](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_get_strpool_ptr.md)  
[png_cache_make_animation](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_make_animation.md)  
[png_cache_get_animation](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_get_animation.md)  
[png_cache_make_animation_table](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_make_animation_table.md)  
[png_cache_get_animation_table](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_get_animation_table.md)  
[png_cache_make_sprite](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_make_sprite.md)  
