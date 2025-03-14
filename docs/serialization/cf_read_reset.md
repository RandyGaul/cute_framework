[](../header.md ':include')

# cf_read_reset

Category: [serialization](/api_reference?id=serialization)  
GitHub: [cute_kv.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_kv.h)  
---

Resets the read position of `kv`.

```cpp
void cf_read_reset(CF_KeyValue* kv);
```

Parameters | Description
--- | ---
kv | The kv instance.

## Remarks

`kv` must be in read mode to use this function. After calling [cf_kv_key](/serialization/cf_kv_key.md) and various combinations of [cf_kv_object_begin](/serialization/cf_kv_object_begin.md)
or [cf_kv_array_begin](/serialization/cf_kv_array_begin.md) you might want to stop reading and reset back to the top-level object in your serialized file. This
function does not do any re-parsing, and merely clears/resets a few internal variables.

## Related Pages

[CF_KeyValue](/serialization/cf_keyvalue.md)  
[cf_kv_read](/serialization/cf_kv_read.md)  
[cf_kv_write](/serialization/cf_kv_write.md)  
[cf_kv_key](/serialization/cf_kv_key.md)  
