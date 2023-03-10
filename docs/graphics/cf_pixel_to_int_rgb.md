[](../header.md ':include')

# cf_pixel_to_int_rgb

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)  
---

Converts an RGB [CF_Pixel](/graphics/cf_pixel.md) to an integer.

```cpp
CF_INLINE uint32_t cf_pixel_to_int_rgb(CF_Pixel p)
```

Parameters | Description
--- | ---
p | The pixel.

## Return Value

Returns an unsigned 32-bit integer of the packed pixel components. The first byte is the red component, the second byte is
the green component, the third byte is the blue component, the fourth byte is 0xFF or full-alpha.

## Related Pages

[cf_pixel_to_color](/graphics/cf_pixel_to_color.md)  
[cf_pixel_to_int_rgba](/graphics/cf_pixel_to_int_rgba.md)  
[cf_pixel_to_string](/graphics/cf_pixel_to_string.md)  
