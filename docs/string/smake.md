[](../header.md ':include')

# smake

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Returns a completely new string copy.

```cpp
#define smake(s) cf_string_make(s)
```

Parameters | Description
--- | ---
s | The string to duplicate.
b | Source for copying.

## Remarks

You must free the copy with [sfree](/string/sfree.md) when done. Does the same thing as [sdup](/string/sdup.md).

## Related Pages

[sset](/string/sset.md)  
[sdup](/string/sdup.md)  
