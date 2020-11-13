
# kv_set_base

The base must be in read mode. This function is used to support data inheritence and delta encoding.

## Syntax

```cpp
void kv_set_base(kv_t* kv, kv_t* base);
```

## Function Parameters

Parameter Name | Description
--- | ---
kv | The kv instance.
base | kv instance to use as a base.

## Remarks

This function is a part of the kv (key-value) serialization API. You can read more about [how this all works here](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/serialization).

### Data Inheritence

If a kv is in read mode any value missing from a kv will be fetched recursively from the base.

### Delta Encoding

If the kv is in write mode any value will first be recursively looked up in base. If found, it is only written if the new value is different from the value to be written.

## Related Functions
  
[kv_size_written](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/kv_size_written.md)  
