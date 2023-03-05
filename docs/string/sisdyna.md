# sisdyna | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string_readme.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Checks to see if a C string is a dynamic string from Cute Framework's string API, or not.

```cpp
#define sisdyna(s) cf_string_is_dynamic(s)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.

## Return Value

Returns true if `s` is a dynamically alloced string from this C string API.

## Remarks

This can be evaluated at compile time for string literals.

## Related Pages

[sstatic](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sstatic.md)  
[sset](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sset.md)  
[spush](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/spush.md)  
