# sinuke | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/README.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Frees up all resources used by the global string table built by [sintern](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sintern.md).

```cpp
#define sinuke() cf_sinuke()
```

## Remarks

All strings previously returned by [sintern](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sintern.md) are now invalid.

## Related Pages

[sintern](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sintern.md)  
[sintern_range](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sintern_range.md)  
[sivalid](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sivalid.md)  
[silen](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/silen.md)  
