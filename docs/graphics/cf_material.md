# CF_Material | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics_readme.md) | [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)

An opaque handle representing a material.

## Remarks

Materials store inputs to shaders. They hold uniforms and textures. Uniforms are like global
variables inside of a shader stage (either the vertex or fragment shaders). For efficiency, all
uniforms are packed into a "uniform buffer", a contiguous chunk of memory on the GPU. We must
specify which uniform buffer each uniform belongs to.

A material can hold a large number of inputs, though there are hard-limits on how many inputs
an individual shader can accept, especially to keep shaders as cross-platform compatible as
possible.

When using sokol-shdc (see [CF_MAKE_SOKOL_SHADER](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_sokol_shader.md)) it will naturally enforce these limits for you, such as:

- Max number of uniform buffers for each shader stage (4)
- Max number of uniforms in a uniform buffer (16)
- Max number of vertex attributes (16) (less on GLES2, see [cf_query_resource_limit](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_query_resource_limit.md))
- Max number of textures for each shader stag (12)

## Related Pages

[CF_Texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_texture.md)  
[CF_Canvas](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_canvas.md)  
[cf_apply_shader](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_apply_shader.md)  
[CF_Shader](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_shader.md)  
[cf_make_material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_material.md)  
[cf_destroy_material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_destroy_material.md)  
[cf_material_set_render_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_render_state.md)  
[cf_material_set_texture_vs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_texture_vs.md)  
[cf_material_set_texture_fs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_texture_fs.md)  
[cf_material_set_uniform_vs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_uniform_vs.md)  
[cf_material_set_uniform_fs](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_uniform_fs.md)  
