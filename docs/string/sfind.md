# sfind | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string_readme.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Scanning from left-to-right, returns a pointer to the substring `find`.

```cpp
#define sfind(s, find) cf_string_find(s, find)
```

Parameters | Description
--- | ---
s | The string.
find | A substring to search for.

## Return Value

Returns `NULL` if not found.

## Related Pages

[sfirst_index_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfirst_index_of.md)  
[slast_index_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/slast_index_of.md)  
