[](../header.md ':include')

# cf_kv_buffer

Category: [serialization](/api_reference?id=serialization)  
GitHub: [cute_kv.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_kv.h)  
---

Fetches the write buffer pointer containing any data serialized so far.

```cpp
const char* cf_kv_buffer(CF_KeyValue* kv);
```

Parameters | Description
--- | ---
kv | The kv instance.

## Return Value

Returns `NULL` if the kv is not opened for write mode with [cf_kv_write](/serialization/cf_kv_write.md).

## Related Pages

[CF_KeyValue](/serialization/cf_keyvalue.md)  
[cf_kv_write](/serialization/cf_kv_write.md)  
[cf_kv_read](/serialization/cf_kv_read.md)  
[cf_kv_destroy](/serialization/cf_kv_destroy.md)  
[cf_kv_key](/serialization/cf_kv_key.md)  
