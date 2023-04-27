[](../header.md ':include')

# slast

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Returns the last character in the string. Not the nul-byte.

```cpp
#define slast(s) cf_string_last(s)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.

## Return Value

Returns '\0' if `s` is `NULL`.

## Related Pages

[spush](/string/spush.md)  
[spop](/string/spop.md)  
[sfirst](/string/sfirst.md)  
[sclear](/string/sclear.md)  
