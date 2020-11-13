# Aseprite Cache

The aseprite cache is used to load aseprites from disk in order to make sprites. You will mostly just care about these two functions.

* [aseprite_cache_load](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_load.md)
* [aseprite_cache_unload](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_unload.md)

It's a cache, which means it actually caches aseprites loaded in RAM, so subsequent calls to [aseprite_cache_load](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_load.md) won't have to fetch the aseprite file off of disk, as long as the aseprite is currently cached in RAM.

The purpose of the cache is to hook up to Cute's batching mechanism. The function to do so is [aseprite_cache_get_pixels_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_get_pixels_fn.md).

[aseprite_cache_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_make.md)  
[aseprite_cache_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_destroy.md)  
[aseprite_cache_load](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_load.md)  
[aseprite_cache_unload](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_unload.md)  
[aseprite_cache_load_ase](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_load_ase.md)  
[aseprite_cache_get_pixels_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_get_pixels_fn.md)  
[aseprite_cache_get_strpool_ptr](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_get_strpool_ptr.md)  
