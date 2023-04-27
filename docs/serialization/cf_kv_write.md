[](../header.md ':include')

# cf_kv_write

Category: [serialization](/api_reference?id=serialization)  
GitHub: [cute_kv.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_kv.h)  
---

Creates a new kv ready for writing.

```cpp
CF_KeyValue* cf_kv_write();
```

## Return Value

Returns a new [CF_KeyValue](/serialization/cf_keyvalue.md) on success. Returns `NULL` on failure.

## Remarks

You can write values as-needed by using [cf_kv_key](/serialization/cf_kv_key.md) and `cf_kv_val` functions.

Example:

```cpp
int a = 10;
int b = 12;

CF_KeyValue kv = kv_write();
cf_kv_key(kv, "a");
cf_kv_val(kv, &a);

cf_kv_key(kv, "b");
cf_kv_val(kv, &b);

printf("%s", cf_kv_buffer());
cf_kv_destroy(kv);
```

Which prints:

```
    a = 10,
    b = 12,
```

## Related Pages

[CF_KeyValue](/serialization/cf_keyvalue.md)  
[cf_kv_key](/serialization/cf_kv_key.md)  
[cf_kv_read](/serialization/cf_kv_read.md)  
[cf_kv_destroy](/serialization/cf_kv_destroy.md)  
[cf_read_reset](/serialization/cf_read_reset.md)  
