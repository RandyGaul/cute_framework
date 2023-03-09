[](../header.md ':include')

# sfirst_index_of

Category: [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Scanning from left-to-right, returns the first index of `ch` found.

```cpp
#define sfirst_index_of(s, ch) cf_string_first_index_of(s, ch)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
ch | A character to search for.

## Return Value

Returns -1 if none are found.

## Related Pages

[sfind](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfind.md)  
[slast_index_of](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/slast_index_of.md)  
