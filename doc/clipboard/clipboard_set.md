# clipboard_set

Use this function to set the clipboard text as a utf8 encoded string.

## Syntax

```cpp
error_t clipboard_set(cute_t* cute, const char* string);
```

## Function Parameters

Parameter Name | Description
--- | ---
cute | The instance of the Cute framework to set the clipboard text of.
string | The utf8 encoded string containing the clipboard text.

## Return Value

Returns an `error_t` containing any error details, if any.

## Related Functions

[clipboard_get](https://github.com/RandyGaul/cute_framework/blob/master/doc/clipboard/clipboard_get.md)
