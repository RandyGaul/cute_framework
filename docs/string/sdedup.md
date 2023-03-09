[](../header.md ':include')

# sdedup

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Removes all consecutive occurances of `ch` from the string.

```cpp
#define sdedup(s, ch) cf_string_dedup(s, ch)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
ch | A character.

## Related Pages

[strim](/string/strim.md)  
[sltrim](/string/sltrim.md)  
[srtrim](/string/srtrim.md)  
[slpad](/string/slpad.md)  
[srpad](/string/srpad.md)  
[serase](/string/serase.md)  
[sreplace](/string/sreplace.md)  
