
# kv_key

Looks for a particular key.

## Syntax

```cpp
error_t kv_key(kv_t* kv, const char* key, kv_type_t* type = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
kv | The kv instance.
key | Key to search for.
type | Optional parameter to specify the key type to search for. `kv_key` will return an error if the requested type does not match the type found.

## Return Value

Returns error details upon failure.

## Remarks

If a particular key is not found then any subsequent calls to [kv_val](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_val.md) will not do anything.

This function is a part of the kv (key-value) serialization API. You can read more about [how this all works here](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/serialization).

### kv_type_t

Enumeration Entry | Description
--- | ---
KV_TYPE_NULL | No type, or error detected.
KV_TYPE_INT64 | Any [integer type](https://www.zentut.com/c-tutorial/c-integer/).
KV_TYPE_DOUBLE | Any [float type](https://www.zentut.com/c-tutorial/c-float/) (`float` or `double`).
KV_TYPE_STRING | `const char*`.
KV_TYPE_ARRAY | A kv array. Read more about array types in the [Arrays section here](https://github.com/RandyGaul/cute_framework/tree/master/doc/serialization).
KV_TYPE_OBJECT | A kv object. Read more about the object type here in the [Objects section](https://github.com/RandyGaul/cute_framework/tree/master/doc/serialization).

## Related Functions
  
[kv_val](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_val.md)  
