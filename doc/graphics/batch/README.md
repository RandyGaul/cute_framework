# Batch

The batch is used to buffer up many different drawable things and organize them into draw calls suitable for high-performance rendering on the GPU. However, this batch is not your typical batcher. This one will build texture atlases internally on the the fly, and periodically needs to fetch pixels to build atlases.

You don't have to worry about texture atlases at all and can build and ship your game with separate images on disk. If you'd like to read more about the implementation of the batcher and why this is a good idea, go ahead and read the documentation in [`cute_spritebatch.h` in the `cute` folder](https://github.com/RandyGaul/cute_framework/blob/master/include/cute/cute_spritebatch.h).

## Get Pixels Function

Cute's batch for sprites is designed to work by periodically fetching pixels on an as-needed basis. The pixels are requested via [get_pixels_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/batch/get_pixels_fn.md). This function pointer can be supplied by either [aseprite cache](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache/aseprite_cache_get_pixels_fn.md) or [png cache](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/png_cache/png_cache_get_pixels_fn.md). The [get_pixels_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/batch/get_pixels_fn.md) is then handed to [batch_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/batch/batch_make.md).
