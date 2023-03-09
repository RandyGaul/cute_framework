[](../header.md ':include')

# cf_kv_read

Category: [serialization](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=serialization)  
GitHub: [cute_kv.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_kv.h)  
---

Parses a buffer of kv data for reading.

```cpp
CF_KeyValue* cf_kv_read(const void* data, size_t size, CF_Result* result_out);
```

Parameters | Description
--- | ---
data | A buffer of serialized kv data.
size | The number of bytes of `data`.
result_out | Can be `NULL`. Contains any errors if present.

## Return Value

Returns a new [CF_KeyValue](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_keyvalue.md) on success. Returns `NULL` on failure, and reports any errors in `result_out`.

## Remarks

All data is loaded up into memory at once. You can fetch out values as-needed by using [cf_kv_key](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_key.md) and `cf_kv_val_` functions.

Example:

```cpp
const char data =
    "a = 10,\n"
    "b = 13,\n"
;
size_t len = CUTE_STRLEN(string);

CF_KeyValue kv = cf_kv_parse((void)string, len, NULL);

int val;
if (cf_kv_key(kv, "a")) {
    cf_kv_val(kv, &val);
    printf("a was %d\n", val);
}
if (cf_kv_key(kv, "b")) {
    cf_kv_val(kv, &val);
    printf("b was %d\n", val);
}

cf_kv_destroy(kv);
```

Which prints:

```
a was 10
b was 13
```

## Related Pages

[CF_KeyValue](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_keyvalue.md)  
[cf_kv_write](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_write.md)  
[cf_kv_key](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_key.md)  
[cf_kv_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_destroy.md)  
[cf_read_reset](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_read_reset.md)  
