
# texture_make

Creates a new texture ready to be used on the GPU.

## Syntax

```cpp
texture_t texture_make(pixel_t* pixels, int w, int h, sg_wrap mode = SG_WRAP_REPEAT);
```

## Function Parameters

Parameter Name | Description
--- | ---
pixels | Array of pixels to be memcpy'd straight onto the GPU in RGBA format.
w | Width of the `pixels` array.
h | Height of the `pixels` array.
mode | Texture wrap mode. Valid values are `SG_WRAP_REPEAT`, `SG_WRAP_CLAMP_TO_EDGE`, `SG_WRAP_CLAMP_TO_BORDER` and `SG_WRAP_MIRRORED_REPEAT`.

## Return Value

Returns the new texture.

## Remarks

This function creates a sokol_gfx.h texture (a `sg_image` instance). In order to learn how to properly use sokol graphics take a peek at the [primer](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/sokol.md) here.

## Related Functions
  
[texture_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/texture/texture_destroy.md)  
