
# to_string

Converts ints/floats to a string.

## Syntax

```cpp
string_t to_string(int x);
string_t to_string(uint64_t x);
string_t to_string(float x);
```

## Function Parameters

Parameter Name | Description
--- | ---
x | The value to convert to a string.

## Return value

Returns an string after performing a conversion operation. An invalid string is returned if conversion fails.

## Remarks

This function uses some static memory, and is a *single-threaded* operation. The static memory can be cleaned up by calling [string_utils_cleanup_static_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/string_utils_cleanup_static_memory.md).

## Related Functions

[format](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/format.md)  
[to_int](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_int.md)  
[to_float](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_float.md)  
[to_array](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_array.md)  
[string_utils_cleanup_static_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/string_utils_cleanup_static_memory.md)  
