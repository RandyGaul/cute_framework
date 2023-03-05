# cf_make_texture | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics_readme.md) | [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)

Returns a new [CF_Texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_texture.md).

```cpp
CF_Texture cf_make_texture(CF_TextureParams texture_params);
```

Parameters | Description
--- | ---
texture_params | The texture parameters as a [CF_TextureParams](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_textureparams.md).

## Return Value

Free it up with [cf_destroy_texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_destroy_texture.md) when done.

## Related Pages

[CF_TextureParams](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_textureparams.md)  
[CF_Texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_texture.md)  
[cf_update_texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_update_texture.md)  
[cf_destroy_texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_destroy_texture.md)  
