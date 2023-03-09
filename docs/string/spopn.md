[](../header.md ':include')

# spopn

Category: [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Removes n characters from the back of a string.

```cpp
#define spopn(s, n) (s = cf_string_pop_n(s, n))
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
n | Number of characters to pop.

## Related Pages

[spop](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/spop.md)  
[slast](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/slast.md)  
[serase](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/serase.md)  
