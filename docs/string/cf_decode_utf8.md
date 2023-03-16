[](../header.md ':include')

# cf_decode_UTF8

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Decodes a single UTF8 character from the string as a UTF32 codepoint.

```cpp
const char* cf_decode_UTF8(const char* s, int* codepoint);
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
codepoint | An `int` codepoint in UTF32 form.

## Return Value

The return value is not a new string, but just s + bytes, where bytes is anywhere from 1 to 4.

## Code Example

> Decoding a UTF8 string one codepoint at a time.

```cpp
int cp;
const char tmp = my_string;
while (tmp) {
    tmp = cf_decode_UTF8(tmp, &cp);
    DoSomethingWithCodepoint(cp);
}
```

## Remarks

You can use this function in a loop to decode one codepoint at a time, where each codepoint
represents a single UTF8 character. If the decoded codepoint is invalid then the "replacement character"
0xFFFD will be recorded instead.

## Related Pages

[sappend_UTF8](/string/sappend_utf8.md)  
[cf_decode_UTF16](/string/cf_decode_utf16.md)  
