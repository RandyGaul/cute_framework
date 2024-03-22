[](../header.md ':include')

# cf_material_clear_textures

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Clears all textures previously set by [cf_material_set_texture_vs](/graphics/cf_material_set_texture_vs.md) or [cf_material_set_texture_fs](/graphics/cf_material_set_texture_fs.md).

```cpp
void cf_material_clear_textures(CF_Material material);
```

Parameters | Description
--- | ---
material | The material.

## Remarks

See [CF_Texture](/graphics/cf_texture.md) and [CF_TextureParams](/graphics/cf_textureparams.md) for an overview.

## Related Pages

[CF_UniformType](/graphics/cf_uniformtype.md)  
[CF_Material](/graphics/cf_material.md)  
[cf_make_material](/graphics/cf_make_material.md)  
[cf_destroy_material](/graphics/cf_destroy_material.md)  
[cf_material_set_render_state](/graphics/cf_material_set_render_state.md)  
[cf_material_set_texture_vs](/graphics/cf_material_set_texture_vs.md)  
[cf_material_set_texture_fs](/graphics/cf_material_set_texture_fs.md)  
[cf_material_set_uniform_vs](/graphics/cf_material_set_uniform_vs.md)  
[cf_material_set_uniform_fs](/graphics/cf_material_set_uniform_fs.md)  
