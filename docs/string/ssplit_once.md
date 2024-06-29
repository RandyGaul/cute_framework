[](../header.md ':include')

# ssplit_once

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Splits a string about the character `ch` one time, scanning from left-to-right.

```cpp
#define ssplit_once(s, ch) cf_string_split_once(s, ch)
```

Parameters | Description
--- | ---
s | The string. Must be a dynamically allocated string from this string API. Does not work on string literals.
ch | A character to split about.

## Return Value

Returns the string to the left of `ch`.

## Remarks

`s` will contain the string to the right of `ch`.
Returns the string to the left of `ch`.
If `ch` isn't found, simply returns `NULL` and does not modify `s`.
You must call [sfree](/string/sfree.md) on the returned string.

This function is intended to be used in a loop, successively chopping off pieces of `s`.
A much easier, but slightly slower, version of this function is [ssplit](/string/ssplit.md), which returns
an array of strings.

## Related Pages

[ssplit](/string/ssplit.md)  
