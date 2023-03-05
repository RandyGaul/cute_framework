# CF_WrapMode | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics_readme.md) | [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)

Wrap modes to define behavior when addressing a texture beyond the [0,1] range.

## Values

Enum | Description
--- | ---
WRAP_MODE_DEFAULT | The default is `CF_WRAP_MODE_REPEAT`.
WRAP_MODE_REPEAT | Repeats the image.
WRAP_MODE_CLAMP_TO_EDGE | Clamps a UV coordinate to the nearest edge pixel.
WRAP_MODE_CLAMP_TO_BORDER | Clamps a UV coordinate to the border color.
WRAP_MODE_MIRRORED_REPEAT | The same as `CF_WRAP_MODE_REPEAT` but mirrors back and forth.

## Related Pages

[CF_TextureParams](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_textureparams.md)  
[cf_wrap_mode_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_wrap_mode_string.md)  
