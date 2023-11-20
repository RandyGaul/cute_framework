[](../header.md ':include')

# sempty

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Returns whether or not the string is empty.

```cpp
#define sempty(s) cf_string_empty(s)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.

## Remarks

Both "" and NULL count as empty.

## Related Pages

[slen](/string/slen.md)  
[scount](/string/scount.md)  
[scap](/string/scap.md)  
