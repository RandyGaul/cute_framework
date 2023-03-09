# App Options

Category: [app](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Various options to control how the application starts up, such as fullscreen, or selecting a graphics backend.

## Values

Enum | Description
--- | ---
APP_OPTIONS_OPENGL_CONTEXT | Starts the app with an OpenGL 3.3 context.
APP_OPTIONS_OPENGLES_CONTEXT | Starts the app with an OpenGL ES 3.3 context.
APP_OPTIONS_D3D11_CONTEXT | Starts the app with a DirectX 11 context.
APP_OPTIONS_DEFAULT_GFX_CONTEXT | Picks a good default graphics context for the given platform.
APP_OPTIONS_FULLSCREEN | Starts the application in borderless full-screen mode.
APP_OPTIONS_RESIZABLE | Allows the window to be resized.
APP_OPTIONS_HIDDEN | Starts the application with the window hidden.
APP_OPTIONS_WINDOW_POS_CENTERED | Starts the application with the window centered on the screen.
APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT | Disables automatically mounting the folder the executable runs from to "/". See [cf_fs_mount](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_mount.md) for more details.
APP_OPTIONS_NO_AUDIO | Starts the application with no audio.
APP_OPTIONS_VSYNC | Starts the application without vertical sync.

## Code Example

> Creating a basic window and immediately destroying it.

```cpp
#include <cute.h>
using namespace cute;

int main(int argc, const char argv)
{
    uint32_t options = APP_OPTIONS_D3D11_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED;
    app_make("Fancy Window Title", 0, 0, 640, 480, options, argv[0]);
    app_destroy();
    return 0;
}
```

## Remarks

The [app_options](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/app_options.md) parameter of [cf_make_app](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_make_app.md) is a bitmask flag. Simply take the `APP_OPTIONS_` flags listed above and OR them together.

## Related Pages

[cf_make_app](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_make_app.md)  
[cf_destroy_app](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_destroy_app.md)  
