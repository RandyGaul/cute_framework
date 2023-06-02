[](../header.md ':include')

# cf_mesh_set_attributes

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Informs CF and the GPU what the memory layout of your vertices and instance data looks like.

```cpp
void cf_mesh_set_attributes(CF_Mesh mesh, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride, int instance_stride);
```

Parameters | Description
--- | ---
mesh | The mesh.
attributes | Vertex attributes to define the memory layout of the mesh vertices.
attribute_count | Number of attributes in `attributes`.
vertex_stride | Number of bytes between each vertex.
instance_stride | Number of bytes between each instance.

## Remarks

You must call this before uploading any data to the GPU. The max number of attributes is 16. Any more attributes beyond 16 will be ignored.

## Related Pages

[CF_Mesh](/graphics/cf_mesh.md)  
[cf_make_mesh](/graphics/cf_make_mesh.md)  
[cf_destroy_mesh](/graphics/cf_destroy_mesh.md)  
[cf_mesh_update_index_data](/graphics/cf_mesh_update_index_data.md)  
[cf_mesh_update_vertex_data](/graphics/cf_mesh_update_vertex_data.md)  
[cf_mesh_update_instance_data](/graphics/cf_mesh_update_instance_data.md)  
