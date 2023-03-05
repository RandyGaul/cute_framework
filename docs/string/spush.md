# spush | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/README.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

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

[sset](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sset.md)  
[spop](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/spop.md)  
[sfit](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfit.md)  
[sfree](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfree.md)  
