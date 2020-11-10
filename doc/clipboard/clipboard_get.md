# clipboard_get

Use this function to get the clipboard text, if it exists. The text is encoded as utf8. You own the pointer, and must call CUTE_FREE on it when done.

## Syntax

```cpp
char* clipboard_get(cute_t* cute);
```

## Return Value

Returns the clipboard text, if it exists. Returns `NULL` on failure. Call [error_get](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/error_get.md) for more information.

## Related Functions

[clipboard_set](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/clipboard_set.md)
