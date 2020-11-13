
# kv_error_state

Returns the error state of the kv instance.

## Syntax

```cpp
error_t kv_error_state(kv_t* kv);
```

## Function Parameters

Parameter Name | Description
--- | ---
kv | The kv instance.

## Return Value

Returns any errors from when any other of the kv functions have failed.

## Remarks

This function is a part of the kv (key-value) serialization API. You can read more about [how this all works here](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/serialization).
