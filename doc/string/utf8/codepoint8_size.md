# codepoint8_size

Returns the size of a codepoint once encoded in a utf8 string.

## Syntax

```cpp
int codepoint8_size(int cp);
```

## Function Parameters

Parameter Name | Description
--- | ---
cp | The copepoint.

## Return Value

Returns the size of a codepoint once encoded in a utf8 string.

## Remarks

This function is mostly useful when encoding a utf8 string from codepoints while making sure to not overrun the buffer you're writing to.

## Related Functions

[encode8](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/encode8.md)  
[decode8](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/decode8.md)  
[widen](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/widen.md)  
[shorten](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/shorten.md)  
