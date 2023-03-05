# sfree | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/README.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Frees up all resources used by the string and sets it to `NULL`.

```cpp
#define sfree(s) cf_string_free(s)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.

## Related Pages

[spush](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/spush.md)  
[spop](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/spop.md)  
[sfit](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfit.md)  
[sset](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sset.md)  
