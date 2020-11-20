
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

## Related Functions
  
[kv_val](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_val.md) will not do anything.  
