
# kv_parse

Parses the text at `data` in a single-pass. Sets the `kv` to read mode `KV_STATE_READ`.

## Syntax

```cpp
error_t kv_parse(kv_t* kv, const void* data, size_t size);
```

## Function Parameters

Parameter Name | Description
--- | ---
kv | The kv instance.
data | Pointer to buffer to parse from.
size | Size of the `data` buffer in bytes.

## Remarks

This function is a part of the kv (key-value) serialization API. You can read more about [how this all works here](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/serialization).

## Related Functions
  
[kv_reset_read_state](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/kv_reset_read_state.md)  
