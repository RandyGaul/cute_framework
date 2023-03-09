[](../header.md ':include')

# CF_UsageType

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

The access pattern for data sent to the GPU.

## Values

Enum | Description
--- | ---
USAGE_TYPE_IMMUTABLE | Can not be changed once created.
USAGE_TYPE_DYNAMIC | Can be changed occasionally, but not once per frame.
USAGE_TYPE_STREAM | Intended to be altered each frame, e.g. streaming data.

## Related Pages

[cf_make_mesh](/graphics/cf_make_mesh.md)  
[cf_usage_type_to_string](/graphics/cf_usage_type_to_string.md)  
[CF_TextureParams](/graphics/cf_textureparams.md)  
