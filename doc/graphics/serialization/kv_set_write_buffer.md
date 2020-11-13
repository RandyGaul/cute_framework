
# kv_set_write_buffer

Sets the `kv` to write mode `KV_STATE_WRITE`, ready to serialize data to `buffer`.

## Syntax

```cpp
void kv_set_write_buffer(kv_t* kv, void* buffer, size_t size);
```

## Function Parameters

Parameter Name | Description
--- | ---
kv | The kv instance.
buffer | Pointer to buffer to write to.
size | Size of the `buffer` buffer in bytes.

## Remarks

This function is a part of the kv (key-value) serialization API. You can read more about [how this all works here](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/serialization).

## Related Functions
  
[kv_size_written](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/kv_size_written.md)  
