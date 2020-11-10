# clipboard_set

Use this function to set the clipboard text as a utf8 encoded string.

## Syntax

```cpp
int clipboard_set(cute_t* cute, const char* string);
```

## Function Parameters

Parameter Name | Description
--- | ---
cute | The instance of the Cute framework to set the clipboard text of.
string | The utf8 encoded string containing the clipboard text.

## Return Value

Returns `0` on success, and `-1` on failure; call [error_get](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/error_get.md) for more information.

## Related Functions

[clipboard_get](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/clipboard_get.md)
