[](../header.md ':include')

# cf_pixel_to_string

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)  
---

Converts a [CF_Pixel](/graphics/cf_pixel.md) to a dynamic string. Free it with [sfree](/string/sfree.md) when done.

```cpp
CF_INLINE char* cf_pixel_to_string(CF_Pixel p)
```

Parameters | Description
--- | ---
p | The pixel.

## Remarks

Since this function dynamically allocates a Cute Framework C-string, it must be free'd up with [sfree](/string/sfree.md) when you're done with it.

## Related Pages

[cf_pixel_to_color](/graphics/cf_pixel_to_color.md)  
[cf_pixel_to_int_rgba](/graphics/cf_pixel_to_int_rgba.md)  
[cf_pixel_to_int_rgb](/graphics/cf_pixel_to_int_rgb.md)  
