[](../header.md ':include')

# cf_pixel_premultiply

Category: [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=graphics)  
GitHub: [cute_color.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_color.h)  
---

Returns a premultiplied [CF_Pixel](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixel.md) by its alpha component.

```cpp
CF_Pixel cf_pixel_premultiply(CF_Pixel c)
```

Parameters | Description
--- | ---
a | The pixel.

## Remarks

Read here for more information about [premultiplied alpha](https://limnu.com/premultiplied-alpha-primer-artists/).

## Related Pages

[cf_mul_pixel](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_mul_pixel.md)  
[cf_div_pixel](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_div_pixel.md)  
[cf_add_pixel](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_add_pixel.md)  
[cf_sub_pixel](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_sub_pixel.md)  
[cf_pixel_lerp](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixel_lerp.md)  
