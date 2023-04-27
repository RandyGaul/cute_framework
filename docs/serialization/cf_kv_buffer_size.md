[](../header.md ':include')

# cf_kv_buffer_size

Category: [serialization](/api_reference?id=serialization)  
GitHub: [cute_kv.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_kv.h)  
---

Returns the size written to the write buffer so far.

```cpp
size_t cf_kv_buffer_size(CF_KeyValue* kv);
```

Parameters | Description
--- | ---
kv | The kv instance.

## Remarks

This size does not include the nul-terminator.

## Related Pages

[CF_KeyValue](/serialization/cf_keyvalue.md)  
[cf_kv_write](/serialization/cf_kv_write.md)  
[cf_kv_buffer](/serialization/cf_kv_buffer.md)  
[cf_kv_key](/serialization/cf_kv_key.md)  
