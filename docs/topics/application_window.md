[](../header.md ':include')

## Creating a Window

Here's a good starting program to copy + paste. It creates a window in the center of the screen with dimensions 640x480 pixels. This snippet runs a typical game loop after window creation.

```cpp
#include <cute.h>

int main(int argc, char* argv[])
{
	// Create a window with a resolution of 640 x 480.
	int options = CF_APP_OPTIONS_DEFAULT_GFX_CONTEXT | CF_APP_OPTIONS_WINDOW_POS_CENTERED;
	CF_Result result = cf_make_app("Fancy Window Title", 0, 0, 640, 480, options, argv[0]);
	if (cf_is_error(result)) return -1;

	while (cf_app_is_running())
	{
		cf_app_update(NULL);
		// All your game logic and updates go here...
		cf_app_draw_onto_screen();
	}

	cf_destroy_app();

	return 0;
}
```

Now is a good time to check out the [App Options](https://randygaul.github.io/cute_framework/#/app/app_options) page to see what kind of windows are avialable. This covers things like letting the window resize, what kind of graphics context to initialize, whether or not audio is enabled, etc.

## Gathering Input

Once your window is created your app is now up and running. You have access to inputs from the keyboard, mouse, joypad, touch, and IME inputs. For more in-depth reading about input, see the [Input](https://randygaul.github.io/cute_framework/#/topics/input) page.

The basic way to gather input is to call functions such as [`cf_key_down`](https://randygaul.github.io/cute_framework/#/input/cf_key_down) or [`cf_mouse_down`](https://randygaul.github.io/cute_framework/#/input/cf_mouse_down). These will return true for as long as the corresponding key or mouse button are currently down. Feel free to call these any time after calling [`cf_app_update`](https://randygaul.github.io/cute_framework/#/app/cf_app_update).

Here is a basic demonstration of capturing keyboard and mouse inputs.

```cpp
#include <cute.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_DEFAULT_GFX_CONTEXT | CF_APP_OPTIONS_WINDOW_POS_CENTERED;
	CF_Result result = cf_make_app("Fancy Window Title", 0, 0, 640, 480, options, argv[0]);
	if (cf_is_error(result)) return -1;

	while (cf_app_is_running())
	{
		cf_app_update(NULL);
		
		// Test for keyboard input.
		if (cf_key_down(CF_KEY_Z)) {
			printf("Key Z is currently DOWN\n");
		}
		
		// Test for mouse input.
		if (cf_mouse_down(CF_MOUSE_BUTTON_LEFT)) {
			printf("Mouse left is currently DOWN\n");
		}
		
		// Showcase the "just pressed" API for keyboard/mouse.
		if (cf_key_just_pressed(CF_KEY_X)) {
			printf("Key X was just pressed\n");
		}
		if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_RIGHT)) {
			printf("Mouse right is currently DOWN\n");
		}
		
		cf_app_draw_onto_screen();
	}

	cf_destroy_app();

	return 0;
}
```

## Window Events

The window can undergo a few different events such as resizing, moving, minimizing, etc. To know when one of these events occurs simply call one of the associated functions to fetch the state of the event.

> Listening and tracing window resize events.

```cpp
if (cf_app_gained_focus()) {
	printf("App gained focus.\n");
}
if (cf_app_lost_focus()) {
	printf("App has lost focus.\n");
}
```

For a full demonstration you can check out the sample on [Window Events](https://github.com/RandyGaul/cute_framework/blob/master/samples/window_events.c).


## App Canvas

If you've already learned about [Low Level Graphics](https://randygaul.github.io/cute_framework/#/topics/low_leveL_graphics) this section will make more sense. The application has its own internal canvas. A canvas is a graphical texture the application can render onto. The app's built-in canvas automatically gathers up all drawings from [`cute_draw.h`](https://randygaul.github.io/cute_framework/#/api_reference?id=draw) and displays them onto the screen. Whenever the app is resized you may want to also resize the app's internal canvas.

TODO -- Resizing sample.
