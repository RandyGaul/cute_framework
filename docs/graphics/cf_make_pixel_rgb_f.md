# cf_make_pixel_rgb_f | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics_readme.md) | [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)

Returns a [CF_Pixel](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixel.md) made from RGB float inputs.

```cpp
CF_Pixel cf_make_pixel_rgb_f(float r, float g, float b)
```

Parameters | Description
--- | ---
r | The red component from 0.0f to 1.0f.
g | The green component from 0.0f to 1.0f.
b | The blue component from 0.0f to 1.0f.

## Remarks

The alpha component is set to 255.

## Related Pages

[CF_Pixel](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixel.md)  
[cf_make_pixel_hex_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_pixel_hex_string.md)  
[cf_make_pixel_rgba_f](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_pixel_rgba_f.md)  
[cf_make_pixel_rgb](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_pixel_rgb.md)  
[cf_make_pixel_rgba](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_pixel_rgba.md)  
[cf_make_pixel_hex](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_pixel_hex.md)  
