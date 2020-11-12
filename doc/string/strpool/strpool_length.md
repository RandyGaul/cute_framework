
# strpool_length

Returns the length of a string.

## Syntax

```cpp
size_t strpool_length(const strpool_t* pool, strpool_id id);
```

## Function Parameters

Parameter Name | Description
--- | ---
pool | The string pool.
id | The string to lookup.

## Return Value

Returns the length of a string, the NUL byte is not included in the length. It is similar to `strlen`.

## Related Functions
  
[strpool_inject](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_inject.md)  
[strpool_discard](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_discard.md)  
[strpool_cstr](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_cstr.md)  
