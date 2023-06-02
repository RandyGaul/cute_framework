[](../header.md ':include')

# CF_DeviceFeature

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Some various device features that may or may not be supported on various backends.

## Values

Enum | Description
--- | ---
DEVICE_FEATURE_TEXTURE_CLAMP | Texture clamp addressing style, e.g. `CF_WRAP_MODE_CLAMP_TO_EDGE` or `CF_WRAP_MODE_CLAMP_TO_BORDER` .

## Remarks

Check to see if a particular feature is available on your backend with [cf_query_device_feature](/graphics/cf_query_device_feature.md).

## Related Pages

[cf_query_device_feature](/graphics/cf_query_device_feature.md)  
[cf_device_feature_to_string](/graphics/cf_device_feature_to_string.md)  
