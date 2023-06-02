[](../header.md ':include')

# cf_kv_key_count

Category: [serialization](/api_reference?id=serialization)  
GitHub: [cute_kv.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_kv.h)  
---

Returns the number of keys within the current object.

```cpp
int cf_kv_key_count(CF_KeyValue* kv);
```

Parameters | Description
--- | ---
kv | The kv.

## Remarks

The kv must have entered an object with `kv_object_begin` in order for this function to make sense.

## Related Pages

[CF_KeyValue](/serialization/cf_keyvalue.md)  
[cf_kv_key](/serialization/cf_kv_key.md)  
[cf_kv_val_int32](/serialization/cf_kv_val_int32.md)  
[cf_kv_val_float](/serialization/cf_kv_val_float.md)  
[cf_kv_val_bool](/serialization/cf_kv_val_bool.md)  
[cf_kv_val_string](/serialization/cf_kv_val_string.md)  
[cf_kv_val_blob](/serialization/cf_kv_val_blob.md)  
[cf_kv_object_begin](/serialization/cf_kv_object_begin.md)  
[cf_kv_array_begin](/serialization/cf_kv_array_begin.md)  
[cf_kv_key_at](/serialization/cf_kv_key_at.md)  
