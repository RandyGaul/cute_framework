# CF_PixelFormatOp | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics_readme.md) | [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)

The various supported operations a pixel format can perform.

## Values

Enum | Description
--- | ---
PIXELFORMAT_OP_NEAREST_FILTER | Nearest-neighbor filtering. Good for pixel art.
PIXELFORMAT_OP_BILINEAR_FILTER | Bilinear filtering, good general purpose option.
PIXELFORMAT_OP_RENDER_TARGET | Used to render to a texture.
PIXELFORMAT_OP_ALPHA_BLENDING | Performs hardware-accelerated alpha-blending.
PIXELFORMAT_OP_MSAA | Performs hardware-accelerated multi-sample antialiasing.
PIXELFORMAT_OP_DEPTH | Performs hardware-accelerated depth-culling.

## Remarks

Not all types are supported on each backend. Be sure to check with [cf_query_pixel_format](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_query_pixel_format.md) if a particular pixel format
is available for your use-case.

## Related Pages

[CF_PixelFormat](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixelformat.md)  
[cf_pixel_format_op_to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixel_format_op_to_string.md)  
[cf_query_pixel_format](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_query_pixel_format.md)  
