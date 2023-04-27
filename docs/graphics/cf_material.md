[](../header.md ':include')

# CF_Material

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

An opaque handle representing a material.

## Remarks

Materials store inputs to shaders. They hold uniforms and textures. Uniforms are like global
variables inside of a shader stage (either the vertex or fragment shaders). For efficiency, all
uniforms are packed into a "uniform buffer", a contiguous chunk of memory on the GPU. We must
specify which uniform buffer each uniform belongs to.

A material can hold a large number of inputs, though there are hard-limits on how many inputs
an individual shader can accept, especially to keep shaders as cross-platform compatible as
possible.

When using sokol-shdc (see [CF_MAKE_SOKOL_SHADER](/graphics/cf_make_sokol_shader.md)) it will naturally enforce these limits for you, such as:

- Max number of uniform buffers for each shader stage (4)
- Max number of uniforms in a uniform buffer (16)
- Max number of vertex attributes (16) (less on GLES2, see [cf_query_resource_limit](/graphics/cf_query_resource_limit.md))
- Max number of textures for each shader stag (12)

## Related Pages

[CF_Texture](/graphics/cf_texture.md)  
[CF_Canvas](/graphics/cf_canvas.md)  
[cf_apply_shader](/graphics/cf_apply_shader.md)  
[CF_Shader](/graphics/cf_shader.md)  
[cf_make_material](/graphics/cf_make_material.md)  
[cf_destroy_material](/graphics/cf_destroy_material.md)  
[cf_material_set_render_state](/graphics/cf_material_set_render_state.md)  
[cf_material_set_texture_vs](/graphics/cf_material_set_texture_vs.md)  
[cf_material_set_texture_fs](/graphics/cf_material_set_texture_fs.md)  
[cf_material_set_uniform_vs](/graphics/cf_material_set_uniform_vs.md)  
[cf_material_set_uniform_fs](/graphics/cf_material_set_uniform_fs.md)  
