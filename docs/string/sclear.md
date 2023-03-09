[](../header.md ':include')

# sclear

Category: [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Sets the string size to zero.

```cpp
#define sclear(s) cf_string_clear(s)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.

## Remarks

Does not free up any resources.

## Related Pages

[spush](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/spush.md)  
[spop](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/spop.md)  
[sfirst](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfirst.md)  
[slast](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/slast.md)  
