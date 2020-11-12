# decode16

Decodes a single code-point.

## Syntax

```cpp
const wchar_t* decode16(const wchar_t* text, int* cp);
```

## Function Parameters

Parameter Name | Description
--- | ---
text | The string to decode from.
cp | The copepoint once decoded.

## Return Value

Once decoded the `text` pointer is incremented just beyond the decoded code point. This works quite well in a loop.

```cpp
const wchar_t* text = get_text();
int cp;

while (*text) {
	text = decode16(text, &cp);
	printf("%d\n", cp);
}
```

## Remarks

Please note this function takes no special care to handle malformed utf16 strings -- do not decode unsecure strings, especially from the net.

## Related Functions

[encode16](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/encode16.md)  
[codepoint16_size](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/codepoint16_size.md)  
[widen](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/widen.md)  
[shorten](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/shorten.md)  
