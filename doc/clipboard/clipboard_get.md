# clipboard_get

Use this function to get the clipboard text, if it exists. The text is encoded as utf8. You own the pointer, and must call `free` (or `CUTE_FREE` if using custom allocators) on it when done.

## Syntax

```cpp
char* clipboard_get(app_t* app);
```

## Return Value

Returns the clipboard text, if it exists. Returns `NULL` on failure.

## Related Functions

[clipboard_set](https://github.com/RandyGaul/cute_framework/blob/master/doc/clipboard/clipboard_set.md)  
