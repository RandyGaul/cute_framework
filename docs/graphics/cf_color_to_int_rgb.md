# cf_color_to_int_rgb | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/README.md) | [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)

Converts an RGBA [CF_Color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_color.md) to an integer.

```cpp
uint32_t cf_color_to_int_rgb(CF_Color c)
```

Parameters | Description
--- | ---
c | The color.

## Return Value

Returns an unsigned 32-bit integer of the packed pixel components. The first byte is the red component, the second byte is
the green component, the third byte is the blue component, the fourth byte is 0xFF or full-alpha.

## Related Pages

[cf_color_to_pixel](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_color_to_pixel.md)  
[cf_color_to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_color_to_string.md)  
[cf_color_to_int_rgba](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_color_to_int_rgba.md)  
