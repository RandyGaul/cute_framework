# sfmt_append | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string_readme.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Printf's into the end of the string, using the format string `fmt`.

```cpp
#define sfmt_append(s, fmt, ...) cf_string_fmt_append(s, fmt, (__VA_ARGS__))
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
fmt | The format string.
... | The parameters for the format string.

## Remarks

All printed data is appended to the end of the string. Will automatically adjust it's capacity as needed.

## Related Pages

[sfmt](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt.md)  
[sset](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sset.md)  
[svfmt](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/svfmt.md)  
[svfmt_append](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/svfmt_append.md)  
