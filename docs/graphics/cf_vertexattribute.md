# CF_VertexAttribute | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/README.md) | [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)

Describes the memory layout of vertex attributes.

Struct Members | Description
--- | ---
`const char* name` | The name of the vertex attribute as it appears in the shader.
`CF_VertexFormat format` | The layout in memory of one vertex. See [CF_VertexFormat](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_vertexformat.md).
`int offset` | The offset in memory from the beginning of a vertex to this attribute.
`CF_AttributeStep step_type` | The step behavior to distinguish between vertex-stepping and instance-stepping. See [CF_AttributeStep](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_attributestep.md).

## Remarks

An attribute is a component of a vertex, usually one, two, three or four floats/integers. A vertex is an input
to a vertex shader. A [CF_Mesh](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_mesh.md) is a collection of vertices and attribute layout definitions.

## Related Pages

[cf_mesh_set_attributes](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_mesh_set_attributes.md)  
