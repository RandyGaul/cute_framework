[](../header.md ':include')

# cf_kv_key

Category: [serialization](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=serialization)  
GitHub: [cute_kv.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_kv.h)  
---

Serializes a key name.

```cpp
bool cf_kv_key(CF_KeyValue* kv, const char* key, CF_KeyValueType* type);
```

Parameters | Description
--- | ---
kv | The kv.
key | The name of the value to serialize.
type | Can be `NULL`. The type of the value at `key`. This is mostly just useful for read mode.

## Return Value

Returns true upon success, false otherwise.

## Remarks

If the `kv` is in write made (made by [cf_kv_write](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_write.md)) this function will write the key name. If the `kv` is in read mode
(created by [cf_kv_read](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_read.md)) then this function will look for a matching key.

## Related Pages

[CF_KeyValue](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_keyvalue.md)  
[cf_kv_array_begin](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_array_begin.md)  
[cf_kv_val_int32](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_val_int32.md)  
[cf_kv_val_float](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_val_float.md)  
[cf_kv_val_bool](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_val_bool.md)  
[cf_kv_val_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_val_string.md)  
[cf_kv_val_blob](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_val_blob.md)  
[cf_kv_object_begin](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_object_begin.md)  
