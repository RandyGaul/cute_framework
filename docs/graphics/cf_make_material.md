[](../header.md ':include')

# cf_make_material

Category: [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Creates a new material.

```cpp
CF_Material cf_make_material();
```

## Remarks

A material holds render state (see [CF_RenderState](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_renderstate.md)), texture inputs (see [CF_Texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_texture.md)), as well as shader inputs called
uniforms (see [CF_UniformType](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_uniformtype.md)). For an overview see [CF_Material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material.md).

## Related Pages

[CF_UniformType](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_uniformtype.md)  
[CF_Material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material.md)  
[cf_material_set_uniform_fs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_uniform_fs.md)  
[cf_destroy_material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_destroy_material.md)  
[cf_material_set_render_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_render_state.md)  
[cf_material_set_texture_vs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_texture_vs.md)  
[cf_material_set_texture_fs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_texture_fs.md)  
[cf_material_set_uniform_vs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_uniform_vs.md)  
