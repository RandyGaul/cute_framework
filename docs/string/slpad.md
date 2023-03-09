[](../header.md ':include')

# slpad

Category: [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Places n characters `ch` onto the front of the string.

```cpp
#define slpad(s, ch, n) cf_string_lpad(s, ch, n)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
ch | A character to insert.
n | Number of times to insert `ch`.

## Related Pages

[strim](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strim.md)  
[sltrim](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sltrim.md)  
[srtrim](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/srtrim.md)  
[serase](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/serase.md)  
[srpad](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/srpad.md)  
[sdedup](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sdedup.md)  
[sreplace](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sreplace.md)  
