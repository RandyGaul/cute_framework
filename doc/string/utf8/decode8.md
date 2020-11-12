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

## Related Functions

[encode8]()  
