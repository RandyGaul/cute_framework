# cf_app_signal_shutdown | [app](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/README.md) | [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)

Call this to end your main-loop; makes [cf_app_is_running](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_is_running.md) return false.

```cpp
void cf_app_signal_shutdown();
```

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
        if (my_game_should_quit()) {
            // The next call to app_is_running() will return false.
            cf_app_signal_shutdown();
        }
        app_draw_onto_screen();
    }
    
    app_destroy();
    
    return 0;
}
```

## Related Pages

[cf_make_app](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_make_app.md)  
[cf_destroy_app](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_destroy_app.md)  
[cf_app_is_running](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_is_running.md)  
