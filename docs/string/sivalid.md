# sivalid | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/README.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Returns true if the string is a static, stable, unique pointer from [sintern](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sintern.md).

```cpp
#define sivalid(s) (((cf_intern_t*)s - 1)->cookie == CF_INTERN_COOKIE)
```

Parameters | Description
--- | ---
s | The string.

## Remarks

This is not a secure method -- do not use it on any potentially dangerous strings. It's designed to be very simple and fast, nothing more.

## Related Pages

[sintern](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sintern.md)  
[sintern_range](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sintern_range.md)  
[sinuke](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sinuke.md)  
[silen](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/silen.md)  
