[](../header.md ':include')

# CF_Mesh

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

An opaque handle representing a mesh.

## Remarks

A mesh is a container of triangles, along with optional indices and instance data. After a mesh
is created the layout of the vertices in memory must be described. We use an array of
[CF_VertexAttribute](/graphics/cf_vertexattribute.md) to define how the GPU will interpret the vertices we send it.

[CF_VertexAttribute](/graphics/cf_vertexattribute.md) are also used to specify instance data by setting `step_type` to
`CF_ATTRIBUTE_STEP_PER_INSTANCE` instead of the default `CF_ATTRIBUTE_STEP_PER_VERTEX`.

Data for meshes can be immutable, dynamic, or streamed, just like textures. Immutable meshes are
perfect for terrain or building rendering, anything static in the world. Dynamic meshes can be
occasionally updated, but are still more like an immutable mesh in terms of performance. Streamed
meshes can be updated each frame, perfect for streaming data to the GPU.

## Related Pages

[CF_Texture](/graphics/cf_texture.md)  
[CF_Canvas](/graphics/cf_canvas.md)  
[CF_Material](/graphics/cf_material.md)  
[CF_Shader](/graphics/cf_shader.md)  
[cf_make_mesh](/graphics/cf_make_mesh.md)  
[cf_destroy_mesh](/graphics/cf_destroy_mesh.md)  
[cf_mesh_set_attributes](/graphics/cf_mesh_set_attributes.md)  
[cf_mesh_update_vertex_data](/graphics/cf_mesh_update_vertex_data.md)  
[cf_mesh_update_instance_data](/graphics/cf_mesh_update_instance_data.md)  
[cf_mesh_update_index_data](/graphics/cf_mesh_update_index_data.md)  
[cf_apply_mesh](/graphics/cf_apply_mesh.md)  
