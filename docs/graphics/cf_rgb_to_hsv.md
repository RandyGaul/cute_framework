# cf_rgb_to_hsv | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/README.md) | [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)

Returns a color converted from rgb form to HSV (Hue Saturation Value) form.

```cpp
CF_Color cf_rgb_to_hsv(CF_Color c)
```

Parameters | Description
--- | ---
c | The color.

## Remarks

Read here for more information about [HSV](https://en.wikipedia.org/wiki/HSL_and_HSV). Often times colors interpolated ([cf_color_lerp](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_color_lerp.md)) in HSV form
look way better than in RGB form. Sometimes it's a good idea to convert to HSV, interpolate, then convert back to RGB in order to interpolate
between two RGB colors. If you interpolate between RGB colors without the intermediate HSV conversion, you may find the middle color to be
some kind of ugly grey color. The intermediate HSV conversion may avoid this grey interpolation artifact.

## Related Pages

[CF_Color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_color.md)  
[cf_hsv_to_rgb](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_hsv_to_rgb.md)  
