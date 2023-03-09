# sprefix

Category: [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Check to see if the string's prefix matches.

```cpp
#define sprefix(s, prefix) cf_string_prefix(s, prefix)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
prefix | A string to compare against the beginning of `s`.

## Return Value

Returns true if `prefix` is the prefix of `s`, false otherwise.

## Related Pages

[sfind](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfind.md)  
[ssuffix](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/ssuffix.md)  
[scontains](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/scontains.md)  
[sfirst_index_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfirst_index_of.md)  
[slast_index_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/slast_index_of.md)  
