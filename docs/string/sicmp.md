# sicmp | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string_readme.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Compares two strings, ignoring case.

```cpp
#define sicmp(a, b) cf_string_icmp(a, b)
```

Parameters | Description
--- | ---
a | The first string.
b | The second string.

## Remarks

Returns 0 if the two strings are equivalent. Otherwise returns 1 if a[i] > b[i], or -1 if a[i] < b[i].

## Related Pages

[scmp](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/scmp.md)  
[siequ](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/siequ.md)  
[sequ](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sequ.md)  
