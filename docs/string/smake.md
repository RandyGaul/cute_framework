# smake | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string_readme.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Returns a completely new string copy.

```cpp
#define smake(s) cf_string_make(s)
```

Parameters | Description
--- | ---
s | The string to duplicate.
b | Source for copying.

## Remarks

You must free the copy with [sfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfree.md) when done. Does the same thing as [sdup](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sdup.md).

## Related Pages

[sset](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sset.md)  
[sdup](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sdup.md)  
