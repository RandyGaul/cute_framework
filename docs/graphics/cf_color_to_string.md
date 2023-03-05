# cf_color_to_string | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/README.md) | [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)

Converts a [CF_Color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_color.md) to a dynamic string. Free it with [sfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfree.md) when done.

```cpp
char* cf_color_to_string(CF_Color c)
```

Parameters | Description
--- | ---
p | The pixel.

## Remarks

Since this function dynamically allocates a Cute Framework C-string, it must be free'd up with [sfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfree.md) when you're done with it.

## Related Pages

[cf_pixel_to_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixel_to_color.md)  
[cf_pixel_to_int_rgba](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixel_to_int_rgba.md)  
[cf_pixel_to_int_rgb](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixel_to_int_rgb.md)  
[cf_pixel_to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixel_to_string.md)  
