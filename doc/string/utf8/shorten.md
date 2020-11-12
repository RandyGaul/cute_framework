
# shorten

Converts a utf16 string to a utf8 string.

## Syntax

```cpp
void shorten(const wchar_t* in, char* out);
void shorten(const wchar_t* in, int in_len, char* out);
void shorten(const wchar_t* in, char* out, int out_len);
void shorten(const wchar_t* in, int in_len, char* out, int out_len);
```

## Function Parameters

Parameter Name | Description
--- | ---
in | The input utf16 string to decode.
out | The output utf8 string to encode.
in_len | The length of the input utf16 string.
out_len | The length of the output utf8 string.

## Remarks

Widening or shortening a string is a useful operation on the Windows platform, as many Windows functinos expect wide strings (`whchar_t`) without any utf8 alternatives. Unfortunately contemporary code libraries use utf8 as the default encoding, and so wide format is a legacy issue that must be dealt with as a special case for Windows.

These functions don't perform any logic for range checking. It is up to you to make sure no buffers are overrun. One simple strategy is to size the output buffer to the worst case maximum size.

Please note these functions take no special care to handle malformed strings -- do not widen/shorten unsecure strings, especially from the net.

## Related Functions

[widen](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/utf8/widen.md)  
