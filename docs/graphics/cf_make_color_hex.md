[](../header.md ':include')

# cf_make_color_hex

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)  
---

Returns a [CF_Color](/graphics/cf_color.md) made from integer hex input.

```cpp
CF_Color cf_make_color_hex(int hex)
```

Parameters | Description
--- | ---
hex | An integer value, e.g. 0xFFAACC.

## Remarks

The opacity of the output color is set to 0xFF (fully opaque). Will assert if the value is greater than 0xFFFFFF.

## Related Pages

[CF_Color](/graphics/cf_color.md)  
[cf_make_color_rgb_f](/graphics/cf_make_color_rgb_f.md)  
[cf_make_color_rgba_f](/graphics/cf_make_color_rgba_f.md)  
[cf_make_color_rgb](/graphics/cf_make_color_rgb.md)  
[cf_make_color_rgba](/graphics/cf_make_color_rgba.md)  
[cf_make_color_hex_string](/graphics/cf_make_color_hex_string.md)  
