[](../header.md ':include')

# sinuke

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Frees up all resources used by the global string table built by [sintern](/string/sintern.md).

```cpp
#define sinuke() cf_sinuke()
```

## Remarks

All strings previously returned by [sintern](/string/sintern.md) are now invalid.

## Related Pages

[sintern](/string/sintern.md)  
[sintern_range](/string/sintern_range.md)  
[sivalid](/string/sivalid.md)  
[silen](/string/silen.md)  
