[](../header.md ':include')

# CF_AttributeStep

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Describes how attribute data is interpreted between each invocation of a vertex shader.

## Values

Enum | Description
--- | ---
ATTRIBUTE_STEP_PER_VERTEX | Take a step forward in the vertex buffer (the `stride`) once per vertex.
ATTRIBUTE_STEP_PER_INSTANCE | Take a step forward in the vertex buffer (the `stride`) once per instance.

## Related Pages

[cf_mesh_set_attributes](/graphics/cf_mesh_set_attributes.md)  
[cf_attribute_step_string](/graphics/cf_attribute_step_string.md)  
[CF_VertexAttribute](/graphics/cf_vertexattribute.md)  
