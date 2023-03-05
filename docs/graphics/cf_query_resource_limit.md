# cf_query_resource_limit | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/README.md) | [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)

Query the device for resource limits.

```cpp
int cf_query_resource_limit(CF_ResourceLimit limit);
```

## Remarks

One notable limit is on `CF_BACKEND_TYPE_GLES2` the number of vertex attributes is low.

## Related Pages

[CF_ResourceLimit](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_resourcelimit.md)  
[cf_resource_limit_to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_resource_limit_to_string.md)  
