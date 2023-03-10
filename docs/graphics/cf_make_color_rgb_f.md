[](../header.md ':include')

# cf_make_color_rgb_f

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)  
---

Returns a [CF_Color](/graphics/cf_color.md) from RGB float inputs.

```cpp
CF_INLINE CF_Color cf_make_color_rgb_f(float r, float g, float b)
```

Parameters | Description
--- | ---
r | The red component from 0.0f to 1.0f.
g | The green component from 0.0f to 1.0f.
b | The blue component from 0.0f to 1.0f.

## Remarks

The alpha component is set to 1.0f;

## Related Pages

[CF_Color](/graphics/cf_color.md)  
[cf_make_color_hex_string](/graphics/cf_make_color_hex_string.md)  
[cf_make_color_rgba_f](/graphics/cf_make_color_rgba_f.md)  
[cf_make_color_rgb](/graphics/cf_make_color_rgb.md)  
[cf_make_color_rgba](/graphics/cf_make_color_rgba.md)  
[cf_make_color_hex](/graphics/cf_make_color_hex.md)  
