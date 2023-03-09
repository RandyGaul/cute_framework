# slast

Category: [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Returns the last character in the string. Not the nul-byte.

```cpp
#define slast(s) cf_string_last(s)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.

## Return Value

Returns '\0' if `s` is `NULL`.

## Related Pages

[spush](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/spush.md)  
[spop](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/spop.md)  
[sfirst](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfirst.md)  
[sclear](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sclear.md)  
