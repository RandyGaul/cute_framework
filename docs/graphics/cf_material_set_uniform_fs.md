[](../header.md ':include')

# cf_material_set_uniform_fs

Category: [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Sets up a uniform value, used for inputs to fragment shaders.

```cpp
void cf_material_set_uniform_fs(CF_Material material, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length);
```

Parameters | Description
--- | ---
material | The material.
block_name | The block name acts like namespace, and groups together uniforms in a single contiguous chunk of memory. You should place
              uniforms that are related to each other, and accessed at the same time, into the same block.
name | The name of the uniform as it appears in the shader.
data | The value of the uniform.
type | The type of the uniform. See [CF_UniformType](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_uniformtype.md).
array_length | The number of elements in the uniform array. Usually this is just `1`, as in, not an array but just one variable.

## Remarks

Uniforms set here do not need to exist in the shader. It's completely acceptable (and encouraged) to setup many uniforms in a material.
Once the material is applied via [cf_apply_shader](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_apply_shader.md), all shader input uniforms (e.g. `uniform vec4 u_my_color`) are dynamically matched up
with uniform values stored in the [CF_Material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material.md). Any uniforms in the material that don't match up will simply be ignored, and cleared to 0
in the shader.

[CF_Material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material.md)'s design supports using one material with various shaders, or using various materials with one shader. Since uniforms are
grouped up into uniform blocks the performance overhead is usually quite minimal for setting a variety of uniform and shader combinations.

## Related Pages

[CF_UniformType](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_uniformtype.md)  
[CF_Material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material.md)  
[cf_make_material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_material.md)  
[cf_destroy_material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_destroy_material.md)  
[cf_material_set_render_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_render_state.md)  
[cf_material_set_texture_vs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_texture_vs.md)  
[cf_material_set_texture_fs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_texture_fs.md)  
[cf_material_set_uniform_vs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_uniform_vs.md)  
