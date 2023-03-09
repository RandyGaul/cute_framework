[](../header.md ':include')

# stohex

Category: [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Converts a hex-string to a uint64_t and returns it.

```cpp
#define stohex(s) cf_string_tohex(s)
```

Parameters | Description
--- | ---
s | The string.

## Remarks

Supports srings that start with "0x", "#", or no prefix.

## Related Pages

[sint](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sint.md)  
[suint](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/suint.md)  
[sfloat](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfloat.md)  
[sdouble](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sdouble.md)  
[shex](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/shex.md)  
[sbool](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sbool.md)  
stint  
[stouint](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/stouint.md)  
[stofloat](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/stofloat.md)  
[stodouble](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/stodouble.md)  
[stobool](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/stobool.md)  
