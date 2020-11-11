
# window_message_box

Pops up an message box with a text description. Usually used for debugging or tools to show error information. If supported different window icons will be used, as per the `type` parameter.

## Syntax

```cpp
void window_message_box(app_t* app, window_message_box_type_t type, const char* title, const char* text);
```

## Remarks

### window_message_box_type_t

Enumeration Entry | Description
--- | ---
WINDOW_MESSAGE_BOX_TYPE_ERROR | For error messages.
WINDOW_MESSAGE_BOX_TYPE_WARNING | For warning messages.
WINDOW_MESSAGE_BOX_TYPE_INFORMATION | For informational messages.
