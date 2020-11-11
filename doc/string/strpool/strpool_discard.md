
# strpool_discard

Removes a string from the pool. Any id referring to this string will be invalid forever.

## Syntax

```cpp
void strpool_discard(strpool_t* pool, strpool_id id)
```

## Function Parameters

Parameter Name | Description
--- | ---
pool | The string pool to remove the string from.
id | The string to be removed.

## Remarks

If the id is invalid this function will do nothing.

## Related Functions

[strpool_inject](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_inject.md)  
[strpool_cstr](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_cstr.md)  
[strpool_length](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_length.md)  
