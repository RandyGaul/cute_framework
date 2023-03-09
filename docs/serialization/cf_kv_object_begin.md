[](../header.md ':include')

# cf_kv_object_begin

Category: [serialization](/api_reference?id=serialization)  
GitHub: [cute_kv.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_kv.h)  
---

Begins the serializatino of a KV object.

```cpp
bool cf_kv_object_begin(CF_KeyValue* kv, const char* key);
```

Parameters | Description
--- | ---
kv | The kv.
key | A key to the object.

## Return Value

Returns true upon success, false otherwise.

## Remarks

This function operates similarly to `kv_key`. See `kv_key` for details. After calling this function a series of `kv_key` and
`kv_val_` calls can be made, one for each member of the object. Call [cf_kv_object_end](/serialization/cf_kv_object_end.md) to complete serialization of the object.
See [CF_KeyValue](/serialization/cf_keyvalue.md) for an overview and some examples.

## Related Pages

[CF_KeyValue](/serialization/cf_keyvalue.md)  
[cf_kv_key](/serialization/cf_kv_key.md)  
[cf_kv_object_end](/serialization/cf_kv_object_end.md)  
