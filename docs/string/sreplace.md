# sreplace | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/README.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Replaces all substrings `replace_me` with the substring `with_me`.

```cpp
#define sreplace(s, replace_me, with_me) cf_string_replace(s, replace_me, with_me)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
replace_me | Substring to replace.
with_me | The replacement string.

## Remarks

Supports srings that start with "0x", "#", or no prefix.

## Related Pages

[strim](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strim.md)  
[sltrim](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sltrim.md)  
[srtrim](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/srtrim.md)  
[slpad](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/slpad.md)  
[srpad](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/srpad.md)  
[sdedup](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sdedup.md)  
[serase](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/serase.md)  
