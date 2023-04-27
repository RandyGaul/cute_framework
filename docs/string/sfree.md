[](../header.md ':include')

# sfree

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Frees up all resources used by the string and sets it to `NULL`.

```cpp
#define sfree(s) cf_string_free(s)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.

## Related Pages

[spush](/string/spush.md)  
[spop](/string/spop.md)  
[sfit](/string/sfit.md)  
[sset](/string/sset.md)  
