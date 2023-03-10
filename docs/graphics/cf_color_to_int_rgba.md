[](../header.md ':include')

# cf_color_to_int_rgba

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)  
---

Converts an RGB [CF_Color](/graphics/cf_color.md) to an integer.

```cpp
CF_INLINE uint32_t cf_color_to_int_rgba(CF_Color c)
```

Parameters | Description
--- | ---
c | The color.

## Return Value

Returns an unsigned 32-bit integer of the packed pixel components. The first byte is the red component, the second byte is
the green component, the third byte is the blue component, the fourth byte is the alpha component.

## Related Pages

[cf_color_to_pixel](/graphics/cf_color_to_pixel.md)  
[cf_color_to_int_rgb](/graphics/cf_color_to_int_rgb.md)  
[cf_color_to_string](/graphics/cf_color_to_string.md)  
