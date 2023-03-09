[](../header.md ':include')

# cf_kv_val_string

Category: [serialization](/api_reference?id=serialization)  
GitHub: [cute_kv.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_kv.h)  
---

Serializes a C style string value.

```cpp
bool cf_kv_val_string(CF_KeyValue* kv, const char** str, size_t* size);
```

Parameters | Description
--- | ---
kv | The kv.
str | A pointer to the string to serialize.
size | The length of the string.

## Return Value

Returns true upon success, false otherwise.

## Remarks

You may call this function after succesfully calling [cf_kv_key](/serialization/cf_kv_key.md). See [CF_KeyValue](/serialization/cf_keyvalue.md) for an overview.

If the `kv` is in write made (made by [cf_kv_write](/serialization/cf_kv_write.md)) this function will write the value from `val`. If the `kv` is in read mode
(created by [cf_kv_read](/serialization/cf_kv_read.md)) then this function read the value and store it in `val`.

## Related Pages

[CF_KeyValue](/serialization/cf_keyvalue.md)  
[cf_kv_key](/serialization/cf_kv_key.md)  
[cf_kv_val_blob](/serialization/cf_kv_val_blob.md)  
