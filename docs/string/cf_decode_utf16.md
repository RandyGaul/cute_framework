# cf_decode_UTF16

Category: [string](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Decodes a single UTF16 character from the string as a UTF32 codepoint.

```cpp
const uint16_t* cf_decode_UTF16(const uint16_t* s, int* codepoint);
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
codepoint | An `int` codepoint in UTF32 form.

## Return Value

The return value is not a new string, but just s + count, where count is anywhere from 1 to 2.

## Remarks

You can use this function in a loop to decode one codepoint at a time, where each codepoint
represents a single UTF8 character.

```cpp
int cp;
const uint16_t tmp = my_string;
while (tmp) {
    tmp = cf_decode_UTF16(tmp, &cp);
    DoSomethingWithCodepoint(cp);
}
```

You can convert a UTF16 string to UTF8 by calling [sappend_UTF8](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sappend_utf8.md) on another string
instance inside the above example loop. Here's an example function to return a new string
instance in UTF8 form given a UTF16 string.

```cpp
char utf8(uint16_t text)
{
int cp;
char s = NULL;
while (text) {
    text = cf_decode_UTF16(text, &cp);
    s = sappend_UTF8(s, cp);
}
return s;
}
```

## Related Pages

[sappend_UTF8](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/sappend_utf8.md)  
[cf_decode_UTF8](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/cf_decode_utf8.md)  
