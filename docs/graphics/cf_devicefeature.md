[](../header.md ':include')

# CF_DeviceFeature

Category: [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Some various device features that may or may not be supported on various backends.

## Values

Enum | Description
--- | ---
DEVICE_FEATURE_INSTANCING | Instancing support, e.g. [cf_mesh_update_instance_data](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_mesh_update_instance_data.md).
DEVICE_FEATURE_MSAA | Hardware-accelerated multi-sample antialiasing (not supporting in Cute Framework yet).
DEVICE_FEATURE_TEXTURE_CLAMP | Texture clamp addressing style, e.g. `CF_WRAP_MODE_CLAMP_TO_EDGE` or `CF_WRAP_MODE_CLAMP_TO_BORDER` .

## Remarks

Check to see if a particular feature is available on your backend with [cf_query_device_feature](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_query_device_feature.md).

## Related Pages

[cf_query_device_feature](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_query_device_feature.md)  
[cf_device_feature_to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_device_feature_to_string.md)  
