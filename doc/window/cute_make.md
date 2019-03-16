# cute_make

Use this function to construct an instance of your game. This function initializes your window, graphics, and audio.

## Syntax

```cpp
cute_t* cute_make(const char* window_title, int x, int y, int w, int h, uint32_t options = 0);
```

## Function Parameters

Parameter Name | Description
--- | ---
window_title | The title of the window in utf8 encoding.
x | The x position of the window.
y | The y position of the window.
w | The width of the window in pixels.
h | The height of the window in pixels.
options | 0 by default; a bitmask of [cute_options_t](https://github.com/RandyGaul/cute_framework/blob/master/doc/window/cute_options_t.md) flags.

## Code Example

## Remarks

## Related Functions

---

window
