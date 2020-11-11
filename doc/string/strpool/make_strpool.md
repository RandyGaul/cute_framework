
# make_strpool

Returns a new strpool, used for string pooling/interning - an efficient way of dealing with strings in a single-threaded manner.

## Syntax

```cpp
strpool_t* make_strpool(void* user_allocator_context = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
user_allocator_context | This can be set to `NULL`, it is for custom allocators. See TODO for more details.

## Return Value

Returns a string pool `strpool_t`.

## Related Functions

[destroy_strpool](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/destroy_strpool.md)  
[strpool_inject](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_inject.md)  
[strpool_discard](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_discard.md)  
