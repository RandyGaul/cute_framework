# scat | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string_readme.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Appends the string b onto the end of a.

```cpp
#define scat(a, b) cf_string_append(a, b)
```

Parameters | Description
--- | ---
a | The string to modify. Can be `NULL`.
b | Used to append onto `a`.

## Remarks

You can technically do this with [sfmt](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt.md), but this function is optimized much faster. Does the same thing as [sappend](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sappend.md).

## Related Pages

[sappend](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sappend.md)  
[sfmt_append](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt_append.md)  
[sappend_range](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sappend_range.md)  
[scat_range](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/scat_range.md)  
[sfmt](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt.md)  
