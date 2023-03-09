[](../header.md ':include')

# scat_range

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Appends a range of characters from string b onto the end of a.

```cpp
#define scat_range(a, b, b_end) cf_string_append_range(a, b, b_end)
```

Parameters | Description
--- | ---
a | The string to modify. Can be `NULL`.
b | Used to append onto `a`.

## Remarks

You can technically do this with [sfmt](/string/sfmt.md), but this function is optimized much faster. Does the same thing as [sappend_range](/string/sappend_range.md).

## Related Pages

[sappend](/string/sappend.md)  
[scat](/string/scat.md)  
[sappend_range](/string/sappend_range.md)  
[sfmt_append](/string/sfmt_append.md)  
[sfmt](/string/sfmt.md)  
