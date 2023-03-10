[](../header.md ':include')

# cf_kv_state

Category: [serialization](/api_reference?id=serialization)  
GitHub: [cute_kv.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_kv.h)  
---

Returns the [CF_KeyValueState](/serialization/cf_keyvaluestate.md) the `kv` was created in.

```cpp
CF_API CF_KeyValueState CF_CALL cf_kv_state(CF_KeyValue* kv);
```

Parameters | Description
--- | ---
kv | The kv instance.

## Remarks

`CF_KV_STATE_WRITE` means the `kv` was created with [cf_kv_write](/serialization/cf_kv_write.md). `CF_KV_STATE_READ` means the `kv` was created with [cf_kv_read](/serialization/cf_kv_read.md).
You can use this function in your serialization routines to do specific things for reading vs writing, whereas for most cases you
can use the same code for both reading and writing.

## Related Pages

[CF_KeyValue](/serialization/cf_keyvalue.md)  
[cf_kv_write](/serialization/cf_kv_write.md)  
[cf_kv_read](/serialization/cf_kv_read.md)  
[cf_kv_destroy](/serialization/cf_kv_destroy.md)  
[cf_read_reset](/serialization/cf_read_reset.md)  
[cf_kv_key](/serialization/cf_kv_key.md)  
