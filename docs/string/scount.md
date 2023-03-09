[](../header.md ':include')

# scount

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Returns the number of characters in the string.

```cpp
#define scount(s) cf_string_count(s)
```

Parameters | Description
--- | ---
s | The string. Must not be `NULL`.

## Code Example

> Demonstrating decrement on [ssize](/string/ssize.md).

```cpp
char s = NULL;
spush(s, 'a');
CUTE_ASSERT(ssize(s) == 1);
ssize(s)--;
CUTE_ASSERT(ssize(a) == 0);
sfree(ssize);
```

## Remarks

Both "" and NULL count as empty. Returns a proper l-value, so you can assign or increment it.

## Related Pages

[slen](/string/slen.md)  
[ssize](/string/ssize.md)  
[sempty](/string/sempty.md)  
[scap](/string/scap.md)  
