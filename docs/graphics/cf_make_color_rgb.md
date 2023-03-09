[](../header.md ':include')

# cf_make_color_rgb

Category: [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=graphics)  
GitHub: [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)  
---

Returns a [CF_Color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_color.md) made from RGB char inputs.

```cpp
CF_Color cf_make_color_rgb(uint8_t r, uint8_t g, uint8_t b)
```

Parameters | Description
--- | ---
r | The red component from 0 to 255.
g | The green component from 0 to 255.
b | The blue component from 0 to 255.

## Remarks

The alpha component is set to 1.0f;

## Related Pages

[CF_Color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_color.md)  
[cf_make_color_rgb_f](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_color_rgb_f.md)  
[cf_make_color_rgba_f](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_color_rgba_f.md)  
[cf_make_color_hex_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_color_hex_string.md)  
[cf_make_color_rgba](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_color_rgba.md)  
[cf_make_color_hex](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_color_hex.md)  
