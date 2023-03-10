[](../header.md ':include')

# cf_pixel_lerp

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)  
---

Lerps from one [CF_Pixel](/graphics/cf_pixel.md) to another.

```cpp
CF_INLINE CF_Pixel cf_pixel_lerp(CF_Pixel a, CF_Pixel b, uint8_t s)
```

Parameters | Description
--- | ---
a | The first pixel.
b | The second pixel.
s | The interpolant from 0 to 255.

## Related Pages

[cf_mul_pixel](/graphics/cf_mul_pixel.md)  
[cf_div_pixel](/graphics/cf_div_pixel.md)  
[cf_add_pixel](/graphics/cf_add_pixel.md)  
[cf_sub_pixel](/graphics/cf_sub_pixel.md)  
[cf_pixel_premultiply](/graphics/cf_pixel_premultiply.md)  
