
# window_size

Retrieves the size of the window in screen coordinates. If high DPI mode is enabled the coordinates may not be a 1:1 pixel ratio.

## Syntax

```cpp
void window_size(app_t* app, int* w, int* h);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
w | The current width of the window in screen coordinates.
h | The current height of the window in screen coordinates.

## Related Functions

[window_was_size_changed](https://github.com/RandyGaul/cute_framework/blob/master/doc/window/window_was_size_changed.md)  
