# svfmt | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/README.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Printf's into the string using the format string `fmt`.

```cpp
#define svfmt(s, fmt, args) cf_string_vfmt(s, fmt, args)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
fmt | The format string.
... | The parameters for the format string.

## Remarks

You probably are looking for [sfmt](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt.md) instead. The string will be overwritten from the beginning. Will automatically adjust it's
capacity as needed. args must be a `va_list`.

## Related Pages

[sfmt](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt.md)  
[sfmt_append](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt_append.md)  
[sset](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sset.md)  
[svfmt_append](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/svfmt_append.md)  
