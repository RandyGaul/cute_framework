# sdup

Category: [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Returns a completely new string copy.

```cpp
#define sdup(s) cf_string_dup(s)
```

Parameters | Description
--- | ---
s | The string to duplicate.

## Remarks

You must free the copy with [sfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfree.md) when done. Does the same thing as [smake](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/smake.md).

## Related Pages

[sset](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sset.md)  
[smake](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/smake.md)  
