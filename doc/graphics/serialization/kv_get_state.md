
# kv_get_state

Returns the state of the kv instance.

## Syntax

```cpp
kv_state_t kv_get_state(kv_t* kv);
```

## Function Parameters

Parameter Name | Description
--- | ---
kv | The kv instance.

## kv_state_t

Enumeration Entry | Description
--- | ---
KV_STATE_UNITIALIZED | kv hasn't been initialized to read or write mode yet.
KV_STATE_WRITE | kv is in write mode.
KV_STATE_READ | kv is in read mode.

## Remarks

This function is a part of the kv (key-value) serialization API. You can read more about [how this all works here](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/serialization).

## Related Functions
  
[kv_parse](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/kv_parse.md)  
[kv_set_write_buffer](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/kv_set_write_buffer.md)  
