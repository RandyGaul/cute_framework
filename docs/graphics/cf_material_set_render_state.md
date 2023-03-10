[](../header.md ':include')

# cf_material_set_render_state

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Sets the render state for a material.

```cpp
CF_API void CF_CALL cf_material_set_render_state(CF_Material material, CF_RenderState render_state);
```

Parameters | Description
--- | ---
material | The material.
render_state | The new render state to set on `material`.

## Remarks

See [CF_RenderState](/graphics/cf_renderstate.md) for an overview.

## Related Pages

[CF_UniformType](/graphics/cf_uniformtype.md)  
[CF_Material](/graphics/cf_material.md)  
[cf_make_material](/graphics/cf_make_material.md)  
[cf_destroy_material](/graphics/cf_destroy_material.md)  
[cf_material_set_uniform_fs](/graphics/cf_material_set_uniform_fs.md)  
[cf_material_set_texture_vs](/graphics/cf_material_set_texture_vs.md)  
[cf_material_set_texture_fs](/graphics/cf_material_set_texture_fs.md)  
[cf_material_set_uniform_vs](/graphics/cf_material_set_uniform_vs.md)  
