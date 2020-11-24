
# kv_object_end

Endss a kv object, ending the previous search scope for future calls to [kv_key](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_key).

## Syntax

```cpp
error_t kv_object_end(kv_t* kv);
```

## Function Parameters

Parameter Name | Description
--- | ---
kv | The kv instance.

## Return Value

Returns any error details upon failure.

## Remarks

This function is a part of the kv (key-value) serialization API. You can read more about [how this all works here](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/serialization).

## Related Functions
  
[kv_key](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_key.md)  
[kv_object_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_object_begin.md)  
[kv_array_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_array_begin.md)  
[kv_array_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_array_end.md)  
