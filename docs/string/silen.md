[](../header.md ':include')

# silen

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Returns the length of an intern'd string.

```cpp
#define silen(s) (((cf_intern_t*)s - 1)->len)
```

Parameters | Description
--- | ---
s | The string.

## Remarks

This is not a secure method -- do not use it on any potentially dangerous strings. It's designed to be very simple and fast, nothing more.
The return value is calculated in constant time, as opposed to calling `CUTE_STRLEN` (`strlen`).

## Related Pages

[sintern](/string/sintern.md)  
[sintern_range](/string/sintern_range.md)  
[sivalid](/string/sivalid.md)  
[sinuke](/string/sinuke.md)  
