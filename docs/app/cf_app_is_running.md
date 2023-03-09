# cf_app_is_running

Category: [app](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Returns true while the app should keep running. Call this as your main loop condition.

```cpp
bool cf_app_is_running();
```

## Return Value

Returns true if the application should continue running.

## Code Example

> Creating a basic 640x480 window for your game.

```cpp
#include <cute.h>
using namespace cute;

int main(int argc, const char argv)
{
    // Create a window with a resolution of 640 x 480, along with a DirectX 11 context.
    app_make("Fancy Window Title", 50, 50, 640, 480, CUTE_APP_OPTIONS_D3D11_CONTEXT, argv[0]);
    
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

Some OS events, like clicking the red X on the app, will signal the app should shutdown.
This will cause this function to return false. You may manually call [cf_app_signal_shutdown](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_signal_shutdown.md)
to signal a shutdown.

## Related Pages

[cf_make_app](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_make_app.md)  
[cf_destroy_app](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_destroy_app.md)  
[cf_app_signal_shutdown](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_signal_shutdown.md)  
