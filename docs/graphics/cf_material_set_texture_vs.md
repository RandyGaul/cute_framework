# cf_material_set_texture_vs | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/README.md) | [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)

Sets up a texture, used for inputs to vertex shaders.

```cpp
void cf_material_set_texture_vs(CF_Material material, const char* name, CF_Texture texture);
```

Parameters | Description
--- | ---
material | The material.
name | The name of the texture, for referring to within a vertex shader.
texture | Data (usually an image) for a shader to access.

## Remarks

See [CF_Texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_texture.md) and [CF_TextureParams](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_textureparams.md) for an overview.

## Related Pages

[CF_UniformType](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_uniformtype.md)  
[CF_Material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material.md)  
[cf_make_material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_material.md)  
[cf_destroy_material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_destroy_material.md)  
[cf_material_set_render_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_render_state.md)  
[cf_material_set_uniform_fs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_uniform_fs.md)  
[cf_material_set_texture_fs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_texture_fs.md)  
[cf_material_set_uniform_vs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_uniform_vs.md)  
