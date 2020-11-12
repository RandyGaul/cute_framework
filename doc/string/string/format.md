
# format

Creates a new string by "printf" style.

## Syntax

```cpp
string_t format(string_t fmt, ...);
```

## Function Parameters

Parameter Name | Description
--- | ---
fmt | The format string, just like when using `printf`.
... | All variables to converted to string, in order as appearing in `fmt`, just like `printf`.

## Return value

Returns an string after performing a conversion operation. An invalid string is returned if conversion fails.

## Remarks

This function uses some static memory, and is a *single-threaded* operation. The static memory can be cleaned up by calling [string_utils_cleanup_static_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/string_utils_cleanup_static_memory.md).

> An example showing how to use this function "printf" style.

```cpp
string_t s = format("Hello %s, it's nice to meet you! I've been here for %d days...\n", "Peter", 29);
printf("%s", s.c_str());
```

Which outputs the following.

```
Hello Peter, it's nice to meet you! I've been here for 29 days...
```

## Related Functions

[to_int](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_int.md)  
[to_float](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_float.md)  
[to_string](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_string.md)  
[to_array](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_array.md)  
[string_utils_cleanup_static_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/string_utils_cleanup_static_memory.md)  
