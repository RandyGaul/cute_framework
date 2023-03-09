[](../header.md ':include')

# sfit

Category: [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Ensures the capacity of the string is at least n elements.

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

[sclear](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sclear.md)  
[scap](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/scap.md)  
