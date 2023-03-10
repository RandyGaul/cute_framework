[](../header.md ':include')

# cf_mesh_will_overflow_instance_data

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Returns true if a number of bytes to append would overflow the internal vertex buffer.

```cpp
CF_API bool CF_CALL cf_mesh_will_overflow_instance_data(CF_Mesh mesh, int append_count);
```

Parameters | Description
--- | ---
mesh | The mesh.
append_count | A number of bytes to append.

## Remarks

Use this when streaming data to the GPU to make sure the internal streaming buffers are not overrun.
You specified this size when creating the mesh. Use this function to understand if you're sending
too much to the GPU all at once. You might need to send less data or increase the size of your mesh's
internal buffers.

## Related Pages

[CF_Mesh](/graphics/cf_mesh.md)  
[cf_make_mesh](/graphics/cf_make_mesh.md)  
[cf_destroy_mesh](/graphics/cf_destroy_mesh.md)  
[cf_mesh_set_attributes](/graphics/cf_mesh_set_attributes.md)  
[cf_mesh_update_vertex_data](/graphics/cf_mesh_update_vertex_data.md)  
[cf_mesh_update_instance_data](/graphics/cf_mesh_update_instance_data.md)  
[cf_mesh_update_index_data](/graphics/cf_mesh_update_index_data.md)  
