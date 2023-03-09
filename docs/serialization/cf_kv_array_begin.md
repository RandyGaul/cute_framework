# cf_kv_array_begin

Category: [serialization](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=serialization)  
GitHub: [cute_kv.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_kv.h)  
---

Begins the serialization of an array.

```cpp
bool cf_kv_array_begin(CF_KeyValue* kv, int* count, const char* key);
```

Parameters | Description
--- | ---
kv | The kv.
count | The number of elements in the array.
key | A key to the object.

## Return Value

Returns true upon success, false otherwise.

## Remarks

This function operates similarly to `kv_key`. See `kv_key` for details. After calling this function a series of
`kv_val_` calls can be made, one for each element of the array. Call [cf_kv_array_end](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_array_end.md) to complete serialization of the array.
See [CF_KeyValue](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_keyvalue.md) for an overview and some examples.

## Related Pages

[CF_KeyValue](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_keyvalue.md)  
[cf_kv_key](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_key.md)  
[cf_kv_array_end](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_array_end.md)  
