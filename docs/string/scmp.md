# scmp | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/README.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Compares two strings.

```cpp
#define scmp(a, b) cf_string_cmp(a, b)
```

Parameters | Description
--- | ---
a | The first string.
b | The second string.

## Remarks

Returns 0 if the two strings are equivalent. Otherwise returns 1 if a[i] > b[i], or -1 if a[i] < b[i].

## Related Pages

[siequ](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/siequ.md)  
[sicmp](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sicmp.md)  
[sequ](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sequ.md)  
