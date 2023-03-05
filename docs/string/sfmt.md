# sfmt | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string_readme.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Printf's into the string using the format string `fmt`.

```cpp
#define sfmt(s, fmt, ...) cf_string_fmt(s, fmt, (__VA_ARGS__))
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
fmt | The format string.
... | The parameters for the format string.

## Remarks

The string will be overwritten from the beginning. Will automatically adjust capacity as needed.

## Related Pages

[sset](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sset.md)  
[sfmt_append](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt_append.md)  
[svfmt](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/svfmt.md)  
[svfmt_append](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/svfmt_append.md)  
