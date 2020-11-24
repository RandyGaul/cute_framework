
# kv_array_begin

Begins a kv array of specified length.

## Syntax

```cpp
error_t kv_array_begin(kv_t* kv, int* count, const char* key = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
kv | The kv instance.
count | Length of the array.
key | Optional (can be `NULL`). If not `NULL` will simply call `kv_key` for you. This is merely for convenience.

## Remarks

This function is a part of the kv (key-value) serialization API. You can read more about [how this all works here](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/serialization).

## Related Functions
  
[kv_key](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_key.md)  
[kv_object_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_object_begin.md)  
[kv_object_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_object_end.md)  
[kv_array_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/serialization/kv_array_end.md)  
