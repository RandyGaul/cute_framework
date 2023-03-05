# cf_query_pixel_format | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics_readme.md) | [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)

Returns true if a particular [CF_PixelFormat](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixelformat.md) is compatible with a certain [CF_PixelFormatOp](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixelformatop.md).

```cpp
bool cf_query_pixel_format(CF_PixelFormat format, CF_PixelFormatOp op);
```

## Remarks

Not all backends support each combination of format and operation, some backends don't support particular pixel formats at all.
Be sure to query the device with this function to make sure your use-case is supported.

## Related Pages

[CF_PixelFormat](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixelformat.md)  
[cf_pixel_format_op_to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixel_format_op_to_string.md)  
[CF_PixelFormatOp](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixelformatop.md)  
