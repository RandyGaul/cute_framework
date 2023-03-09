[](../header.md ':include')

# CF_ResourceLimit

Category: [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Some backends have limits on specific GPU resources.

## Values

Enum | Description
--- | ---
RESOURCE_LIMIT_TEXTURE_DIMENSION | Limit on the number of dimensions a texture can have, e.g. 2 or 3.
RESOURCE_LIMIT_VERTEX_ATTRIBUTE_MAX | Limit on the number of vertex attributes. Notably lower on `CF_BACKEND_TYPE_GLES2`.

## Related Pages

[cf_query_resource_limit](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_query_resource_limit.md)  
[cf_resource_limit_to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_resource_limit_to_string.md)  
