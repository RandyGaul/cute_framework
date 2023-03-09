# sstatic

Category: [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Creates a string with an initial static storage backing.

```cpp
#define sstatic(s, buffer, buffer_size) cf_string_static(s, buffer, buffer_size)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
buffer | Pointer to a static memory buffer.
buffer_size | The size of `buffer` in bytes.

## Remarks

Will grow onto the heap if the size becomes too large. Call [sfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfree.md) when done.

## Related Pages

[sset](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sset.md)  
[sisdyna](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sisdyna.md)  
[spush](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/spush.md)  
