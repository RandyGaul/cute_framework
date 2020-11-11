
# window_was_size_changed

Retrieves the size of the window in screen coordinates. If high DPI mode is enabled the coordinates may not be a 1:1 pixel ratio.

## Syntax

```cpp
bool window_was_size_changed(app_t* app);
```

## Return Value

Returns true if the window was moved, false otherwise.

## Related Functions

[window_position](https://github.com/RandyGaul/cute_framework/blob/master/doc/window/window_position.md)  
