# app_make

Use this function to construct an instance of your application window. This function initializes your window and any associated graphics context (if requested with the app options).

## Syntax

```cpp
app_t* app_make(const char* window_title, int x, int y, int w, int h, uint32_t options = 0, const char* argv0 = NULL, void* user_allocator_context = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
window_title | The title of the window in utf8 encoding.
x | The x position of the window.
y | The y position of the window.
w | The width of the window in pixels.
h | The height of the window in pixels.
options | 0 by default; a bitmask of [app options](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_options.md) flags.
argv0 | The first argument passed to your main function in the `argv` parameter.
user_allocator_context | Used for custom allocators, this can be set to `NULL`. See (TODO) for more details.

## Return Value

Returns a pointer to an `app_t` instance. Destroy it with [app_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_destroy.md) when you're done with it. Returns `NULL` on failure.

## Code Example

> Creating a window and closing it.

```cpp
#include <cute.h>
using namespace cute;

int main(int argc, const char** argv)
{
	// Create a window with a resolution of 640 x 480, along with a DirectX 11 context.
	app_t* app = app_make("Fancy Window Title", 50, 50, 640, 480, CUTE_APP_OPTIONS_D3D11_CONTEXT, argv[0]);

	while (app_is_running(app))
	{
		float dt = calc_dt();
		app_update(app, dt);
		// All your game logic and updates go here...
		app_present(app);
	}
	
	app_destroy(app);
	
	return 0;
}
```

## Remarks

The [options](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_options.md) parameter is a bitmask of flags. Different flags can be OR'd together. Parameters **w** and **h** are ignored if Cute is initialized to fullscreen.

## Related Functions

[app_is_running](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_is_running.md)  
[app_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_update.md)  
[app_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_destroy.md)  
