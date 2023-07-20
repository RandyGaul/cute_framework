[](../header.md ':include')

# sfit

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Ensures the capacity of the string is at least n+1 elements.

```cpp
#define sfit(s, n) cf_string_fit(s, n)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
n | The number of elements for the new internal capacity.

## Remarks

Does not change the size/count of the string, or the len. This function is just here for optimization purposes.

## Related Pages

[sclear](/string/sclear.md)  
[scap](/string/scap.md)  
