
# texture_destroy

Destroys a texture.

## Syntax

```cpp
void CUTE_CALL texture_destroy(texture_t texture);
```

## Function Parameters

Parameter Name | Description
--- | ---
texture | The texture to destroy.

## Remarks

This function destroys a sokol_gfx.h texture (a `sg_image` instance). In order to learn how to properly use sokol graphics take a peek at the [primer here](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sokol.md).

If you just want to draw sprites on screen it is recommend you use [aseprite_cache](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/aseprite_cache) instead.

## Related Functions
  
[texture_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/texture/texture_make.md)  
