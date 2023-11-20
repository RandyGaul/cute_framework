[](../header.md ':include')

# scap

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Gets the capacity of the string.

```cpp
#define scap(s) cf_string_cap(s)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.

## Remarks

This is not the number of characters, but the size of the internal buffer. The capacity automatically grows as necessary, but
you can use [sfit](/string/sfit.md) to ensure a minimum capacity manually, as an optimization.

## Related Pages

[slen](/string/slen.md)  
[scount](/string/scount.md)  
[sempty](/string/sempty.md)  
