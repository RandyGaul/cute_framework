[](../header.md ':include')

# scat

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Appends the string b onto the end of a.

```cpp
#define scat(a, b) cf_string_append(a, b)
```

Parameters | Description
--- | ---
a | The string to modify. Can be `NULL`.
b | Used to append onto `a`.

## Remarks

You can technically do this with [sfmt](/string/sfmt.md), but this function is optimized much faster. Does the same thing as [sappend](/string/sappend.md).

## Related Pages

[sappend](/string/sappend.md)  
[sfmt_append](/string/sfmt_append.md)  
[sappend_range](/string/sappend_range.md)  
[scat_range](/string/scat_range.md)  
[sfmt](/string/sfmt.md)  
