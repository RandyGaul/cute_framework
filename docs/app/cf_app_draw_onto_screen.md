[](../header.md ':include')

# cf_app_draw_onto_screen

Category: [app](/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Draws the app onto the screen.

```cpp
CF_API int CF_CALL cf_app_draw_onto_screen();
```

## Return Value

Returns the number of draw calls for this frame.

## Code Example

> Creating a basic 640x480 window for your game.

```cpp
#include <cute.h>
using namespace cute;

int main(int argc, const char argv)
{
    // Create a window with a resolution of 640 x 480, along with a DirectX 11 context.
    app_make("Fancy Window Title", 50, 50, 640, 480, CF_APP_OPTIONS_D3D11_CONTEXT, argv[0]);
    
    while (app_is_running())
    {
        app_update();
        // All your game logic and updates go here...
        app_draw_onto_screen();
    }
    
    app_destroy();
    
    return 0;
}
```

## Remarks

Call this at tnhe end of your main loop.

## Related Pages

[cf_make_app](/app/cf_make_app.md)  
[cf_app_is_running](/app/cf_app_is_running.md)  
[cf_app_signal_shutdown](/app/cf_app_signal_shutdown.md)  
[cf_destroy_app](/app/cf_destroy_app.md)  
