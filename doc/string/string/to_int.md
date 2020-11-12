
# to_int

Converts a string to int.

## Syntax

```cpp
int to_int(const string_t& x);
```
## Return value

Returns an integer after performing a string conversion operation. 0 is returned on a failed conversion.

## Remarks

This function uses some static memory, and is a *single-threaded* operation. The static memory can be cleaned up by calling [string_utils_cleanup_static_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/string_utils_cleanup_static_memory.md).

## Related Functions

[to_float](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_float.md)  
[to_string](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_string.md)  
[to_array](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_array.md)  
[string_utils_cleanup_static_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/string_utils_cleanup_static_memory.md)  
