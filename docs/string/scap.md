# scap | [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/README.md) | [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)

Gets the capacity of the string.

```cpp
#define scap(s) cf_string_cap(s)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.

## Remarks

This is not the number of characters, but the size of the internal buffer. The capacity automatically grows as necessary, but
you can use [sfit](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sfit.md) to ensure a minimum capacity manually, as an optimization.

## Related Pages

[slen](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/slen.md)  
[ssize](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/ssize.md)  
[scount](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/scount.md)  
[sempty](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sempty.md)  
