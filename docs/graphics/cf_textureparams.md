[](../header.md ':include')

# CF_TextureParams

Category: [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

A collection of parameters to create a [CF_Texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_texture.md) with [cf_make_texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_texture.md).

Struct Members | Description
--- | ---
`CF_PixelFormat pixel_format` | The pixel format for this texture's data. See [CF_PixelFormat](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_pixelformat.md).
`CF_UsageType usage` | The memory access pattern for this texture on the GPU. See [CF_UsageType](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_usagetype.md).
`CF_Filter filter` | The filtering operation to use when fetching data out of the texture, on the GPU. See [CF_Filter](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_filter.md).
`CF_WrapMode wrap_u` | The texture wrapping behavior when addressing beyond [0,1] for the u-coordinate. See [CF_WrapMode](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_wrapmode.md).
`CF_WrapMode wrap_v` | The texture wrapping behavior when addressing beyond [0,1] for the v-coordinate. See [CF_WrapMode](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_wrapmode.md).
`int width` | Number of elements (usually pixels) along the width of the texture.
`int height` | Number of elements (usually pixels) along the height of the texture.
`bool render_target` | If true you can render to this texture via [CF_Canvas](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_canvas.md).
`int initial_data_size` | The size of the `initial_data` member. Must be non-zero for immutable textures.
`void* initial_data` | The intial byte data to fill the texture in with. Must not be `NULL` for immutable textures. See `CF_USAGE_TYPE_IMMUTABLE`.

## Remarks

You may get a set of good default values by calling [cf_texture_defaults](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_texture_defaults.md).

## Related Pages

[cf_update_texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_update_texture.md)  
[cf_texture_defaults](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_texture_defaults.md)  
[CF_Texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_texture.md)  
[cf_make_texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_texture.md)  
[cf_destroy_texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_destroy_texture.md)  
