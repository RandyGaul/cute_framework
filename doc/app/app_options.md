# app options

The bitmask flags passed into [app_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_make.md).

## Values

Enumeration Entry | Description
--- | ---
CUTE_APP_OPTIONS_OPENGL_CONTEXT | Creates an Open GL 3.3 context.
CUTE_APP_OPTIONS_OPENGLES_CONTEXT | Creates an Open GL ES 3.0 context.
CUTE_APP_OPTIONS_D3D11_CONTEXT | Initializes a DirectX 11 context.
CUTE_APP_OPTIONS_FULLSCREEN | Starts the application in borderless full-screen mode.
CUTE_APP_OPTIONS_RESIZABLE | Allows the window to be resized.
CUTE_APP_OPTIONS_HIDDEN | Starts the application with the window hidden.
CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | Starts the application with the window centered on the screen.
CUTE_APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT | Skips the default file system mount point. See TODO for details.

## Code Example


> Creating a window and immediately closing it.

```cpp
#include <cute.h>
using namespace cute;

int main(int argc, const char** argv)
{
	uint32_t options = CUTE_APP_OPTIONS_D3D11_CONTEXT | CUTE_APP_OPTIONS_WINDOW_POS_CENTERED;
	app_t* app = app_make("Fancy Window Title", 0, 0, 640, 480, options, argv[0]);
	app_destroy(app);
	return 0;
}
```

## Remarks

The `options` parameter of [cute_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/window/cute_make.md) is a bitmask flag. Simply take the `CUTE_APP_OPTIONS_*` flags listed above and OR them together.

## Related Functions

[cute_make](https://github.com/RandyGaul/cute_framework/tree/master/doc/window/cute_make.md)
