[](../header.md ':include')

# cf_kv_val_uint16

Category: [serialization](/api_reference?id=serialization)  
GitHub: [cute_kv.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_kv.h)  
---

Serializes an unsigned 16-bit value.

```cpp
CF_API bool CF_CALL cf_kv_val_uint16(CF_KeyValue* kv, uint16_t* val);
```

Parameters | Description
--- | ---
kv | The kv.
val | The value to serialize.

## Return Value

Returns true upon success, false otherwise.

## Remarks

You may call this function after succesfully calling [cf_kv_key](/serialization/cf_kv_key.md). See [CF_KeyValue](/serialization/cf_keyvalue.md) for an overview.

If the `kv` is in write made (made by [cf_kv_write](/serialization/cf_kv_write.md)) this function will write the value from `val`. If the `kv` is in read mode
(created by [cf_kv_read](/serialization/cf_kv_read.md)) then this function read the value and store it in `val`.

## Related Pages

[CF_KeyValue](/serialization/cf_keyvalue.md)  
[cf_kv_key](/serialization/cf_kv_key.md)  
[cf_kv_val_uint8](/serialization/cf_kv_val_uint8.md)  
[cf_kv_val_uint64](/serialization/cf_kv_val_uint64.md)  
[cf_kv_val_uint32](/serialization/cf_kv_val_uint32.md)  
