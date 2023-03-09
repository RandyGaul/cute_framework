# ssuffix

Category: [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Check to see if the string's suffix matches.

```cpp
#define ssuffix(s, suffix) cf_string_suffix(s, suffix)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
prefix | A string to compare against the end of `s`.

## Return Value

Returns true if `suffix` is the suffix of `s`, false otherwise.

## Related Pages

[sprefix](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sprefix.md)  
[sfind](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfind.md)  
[scontains](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/scontains.md)  
[sfirst_index_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfirst_index_of.md)  
[slast_index_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/slast_index_of.md)  
