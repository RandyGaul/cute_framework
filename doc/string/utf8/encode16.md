# encode16

Encodes a single code-point.

## Syntax

```cpp
whcar_t* encode16(whcar_t* text, int cp);
```

## Function Parameters

Parameter Name | Description
--- | ---
text | The buffer to encode within.
cp | The copepoint to encode.

## Return Value

Once encoded the `text` pointer is incremented just beyond the encoded code point. This works quite well in a loop.

```cpp
whcar_t* buf = get_buffer();
int sz = get_buffer_size();
int size_encoded = 0;
std::vector<int> codepoints = get_copepoints();

for (int i = 0; i < codepoints.size(); ++i) {
	int cp = codepoints[i];
	int size_to_encode = codepoint16_size(cp);
	if (size_encoded + size_to_encode < sz) {
		buf = encode16(buf, cp);
	} else {
		break;
	}
}
```

## Remarks

Please note this function takes no special care to handle malformed utf16 strings -- do not encode unsecure strings, especially from the net.

## Related Functions

[decode16](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/decode16.md)  
[codepoint16_size](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/codepoint16_size.md)  
[widen](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/widen.md)  
[shorten](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/shorten.md)  
