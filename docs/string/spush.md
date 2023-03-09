[](../header.md ':include')

# spush

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Pushes character `ch` onto the end of the string.

```cpp
#define spush(s, ch) cf_string_push(s, ch)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
ch | A character to push onto the end of the string.

## Remarks

Does not overwite the nul-byte. If the string is empty a nul-byte is pushed afterwards. Can be NULL, will create a new string and assign `s` if so.

## Related Pages

[sset](/string/sset.md)  
[spop](/string/spop.md)  
[sfit](/string/sfit.md)  
[sfree](/string/sfree.md)  
