[](../header.md ':include')

# cf_mesh_append_index_data

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Appends index data onto the end of the mesh's internal index buffer.

```cpp
int cf_mesh_append_index_data(CF_Mesh mesh, uint32_t* indices, int count);
```

Parameters | Description
--- | ---
mesh | The mesh.
indices | A pointer to index data.
count | Number of bytes in `data`.

## Return Value

Returns the number of bytes appended.

## Remarks

The mesh must have been created with `CF_USAGE_TYPE_DYNAMIC` or `CF_USAGE_TYPE_STREAM` in order
to call this function more than once. This function can be called multiple times per frame. The
intended use-case is to stream bits of data to the GPU and issue a [cf_draw_elements](/graphics/cf_draw_elements.md) call. The
only elements that will be drawn are the elements from the last call to [cf_mesh_append_index_data](/graphics/cf_mesh_append_index_data.md),
all previously appended data will remain untouched.

## Related Pages

[CF_Mesh](/graphics/cf_mesh.md)  
[cf_make_mesh](/graphics/cf_make_mesh.md)  
[cf_destroy_mesh](/graphics/cf_destroy_mesh.md)  
[cf_mesh_set_attributes](/graphics/cf_mesh_set_attributes.md)  
[cf_mesh_update_vertex_data](/graphics/cf_mesh_update_vertex_data.md)  
[cf_mesh_update_instance_data](/graphics/cf_mesh_update_instance_data.md)  
[cf_mesh_update_index_data](/graphics/cf_mesh_update_index_data.md)  
