# decode8

Decodes a single code-point.

## Syntax

```cpp
const char* decode8(const char* text, int* cp);
```

## Function Parameters

Parameter Name | Description
--- | ---
text | The string to decode from.
cp | The copepoint once decoded.

## Return Value

Once decoded the `text` pointer is incremented just beyond the decoded code point. This works quite well in a loop.

```cpp
const char* text = get_text();
int cp;

while (*text) {
	text = decode8(text, &cp);
	printf("%d\n", cp);
}
```

## Remarks

Please note this function takes no special care to handle malformed utf8 strings -- do not decode unsecure strings, especially from the net.

## Related Functions

[encode8](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/encode8.md)  
[codepoint8_size](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/codepoint8_size.md)  
[widen](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/widen.md)  
[shorten](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/shorten.md)  
