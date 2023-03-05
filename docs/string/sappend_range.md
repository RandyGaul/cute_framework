# sappend_range | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string_readme.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Appends a range of characters from string b onto the end of a.

```cpp
#define sappend_range(a, b, b_end) cf_string_append_range(a, b, b_end)
```

Parameters | Description
--- | ---
a | The string to modify. Can be `NULL`.
b | Used to append onto `a`.

## Remarks

You can technically do this with [sfmt](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt.md), but this function is optimized much faster. Does the same thing as [scat_range](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/scat_range.md).

## Related Pages

[sappend](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sappend.md)  
[scat](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/scat.md)  
[sfmt_append](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt_append.md)  
[scat_range](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/scat_range.md)  
[sfmt](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt.md)  
