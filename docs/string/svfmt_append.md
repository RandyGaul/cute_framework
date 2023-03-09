# svfmt_append

Category: [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Printf's into the string using the format string `fmt`.

```cpp
#define svfmt_append(s, fmt, args) cf_string_vfmt_append(s, fmt, args)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
fmt | The format string.
... | The parameters for the format string.

## Remarks

You probably are looking for [sfmt_append](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt_append.md) instead. The string will be overwritten from the beginning. Will automatically adjust it's
capacity as needed. args must be a `va_list`.

## Related Pages

[sfmt](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt.md)  
[sfmt_append](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfmt_append.md)  
[svfmt](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/svfmt.md)  
[sset](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sset.md)  
