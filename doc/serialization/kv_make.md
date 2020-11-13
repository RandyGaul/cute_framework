
# kv_make

Makes a kv instance.

## Syntax

```cpp
kv_t* kv_make(void* user_allocator_context = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
user_allocator_context | Used for custom allocators, this can be set to `NULL`. See (TODO) for more details.

## Return Value

Returns a new kv instance.

## Remarks

This function is a part of the kv (key-value) serialization API. You can read more about [how this all works here](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics/serialization).

## Related Functions
  
[kv_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/graphics/image/kv_destroy.md)  
