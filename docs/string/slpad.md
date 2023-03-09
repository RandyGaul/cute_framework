[](../header.md ':include')

# slpad

Category: [string](/api_reference?id=string)  
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

[strim](/string/strim.md)  
[sltrim](/string/sltrim.md)  
[srtrim](/string/srtrim.md)  
[serase](/string/serase.md)  
[srpad](/string/srpad.md)  
[sdedup](/string/sdedup.md)  
[sreplace](/string/sreplace.md)  
