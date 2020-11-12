# codepoint16_size

Returns the size of a codepoint once encoded in a utf16 string.

## Syntax

```cpp
int codepoint16_size(int cp);
```

## Function Parameters

Parameter Name | Description
--- | ---
cp | The copepoint.

## Return Value

Returns the size of a codepoint once encoded in a utf16 string.

## Remarks

This function is mostly useful when encoding a utf8 string from codepoints while making sure to not overrun the buffer you're writing to.

## Related Functions

[encode16](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/encode16.md)  
[decode16](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/decode16.md)  
[widen](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/widen.md)  
[shorten](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/shorten.md)  
