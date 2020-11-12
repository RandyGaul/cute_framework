
# to_float

Converts a string to float.

## Syntax

```cpp
float to_float(const string_t& x);
```

## Function Parameters

Parameter Name | Description
--- | ---
x | The string to convert.

## Return value

Returns an float after performing a string conversion operation. 0 is returned on a failed conversion.

## Remarks

This function uses some static memory, and is a *single-threaded* operation. The static memory can be cleaned up by calling [string_utils_cleanup_static_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/string_utils_cleanup_static_memory.md).

## Related Functions

[format](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/format.md)  
[to_int](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_int.md)  
[to_string](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_string.md)  
[to_array](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_array.md)  
[string_utils_cleanup_static_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/string_utils_cleanup_static_memory.md)  
