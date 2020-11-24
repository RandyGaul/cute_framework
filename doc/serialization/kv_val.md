
# kv_val

Fetches the value for the last key found by [kv_key](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_key.md).

## Syntax

```cpp
error_t kv_val(kv_t* kv, uint8_t* val);
error_t kv_val(kv_t* kv, uint16_t* val);
error_t kv_val(kv_t* kv, uint32_t* val);
error_t kv_val(kv_t* kv, uint64_t* val);

error_t kv_val(kv_t* kv, int8_t* val);
error_t kv_val(kv_t* kv, int16_t* val);
error_t kv_val(kv_t* kv, int32_t* val);
error_t kv_val(kv_t* kv, int64_t* val);

error_t kv_val(kv_t* kv, float* val);
error_t kv_val(kv_t* kv, double* val);
error_t kv_val(kv_t* kv, bool* val);
```

## Function Parameters

Parameter Name | Description
--- | ---
kv | The kv instance.
val | The value to be fetched when in read mode, or written when in write mode.

## Return Value

Returns error details upon failure.

## Remarks

If a particular key is not found then any calls to [kv_val](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_val.md) will do nothing.

This function is a part of the kv (key-value) serialization API. You can read more about [how this all works here](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/serialization).

### Type Conversions

When parsing kv stores all integers in 64-bit format internally. Similarly all floats are stored internally as doubles. Whenever kv_val is called the requested type of the val parameter will be typecasted internally when dealing with integers and floats.

## Related Functions
  
[kv_key](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_key.md)  
