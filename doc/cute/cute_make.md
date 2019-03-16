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

## Return Value

Returns a pointer to a `cute_t` instance, representing a mixture of the application window, optional audio, and optional graphics. Returns `NULL` on failure; call [cute_get_error](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute_get_error.md) for more information.

## Code Example

> Creating a window and printing out a string to the screen with Cute's default font.

```cpp
#include <cute.h>
using namespace cute;

// Initialize the cute framework, make a window, initialize audio, and setup DirectX 9.
cute_t* cute = cute_make("Fancy Window Title", 0, 0, 640, 480, CUTE_OPTIONS_GFX_D3D9);

while (is_running(cute))
{
	int font_x = 0, font_y = 0;
	font_print(cute, font_x, font_y, "Hello, world!");
	cute_update(cute);
}
```

## Remarks

The [options](https://github.com/RandyGaul/cute_framework/blob/master/doc/window/cute_options_t.md) parameter is a flag. Different options can be OR'd together. Parameters **w** and **h** are ignored if Cute is initialized to fullscreen.

## Related Functions

[cute_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/window/cute_destroy.md)
