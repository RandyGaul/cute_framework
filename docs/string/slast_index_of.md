[](../header.md ':include')

# slast_index_of

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Scanning from right-to-left, returns the first index of `ch` found.

```cpp
#define slast_index_of(s, ch) cf_string_last_index_of(s, ch)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
ch | A character to search for.

## Return Value

Returns -1 if none are found.

## Related Pages

[sfirst_index_of](/string/sfirst_index_of.md)  
[sfind](/string/sfind.md)  
