# string_utils_cleanup_static_memory

Cleans up all static memory from the string conversion utility functions.

## Syntax

```cpp
void string_utils_cleanup_static_memory();
```

## Remarks

Functions such as [to_int](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/to_int.md) and [to_float](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/to_float.md) make use of some static memory to perform conversions. This function cleans up that static memory. All functions that use this static memory are listed below in the Related Functions section.

## Related Functions

[operator+](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/operator+.md)  
[to_int](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/to_int.md)  
[to_float](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/to_float.md)  
[format](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/format.md)  
[to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/to_string.md)  
