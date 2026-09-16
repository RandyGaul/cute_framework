# Application Callbacks

SDL3 allows defining an application as a series of [callbacks](https://wiki.libsdl.org/SDL3/README-main-functions#main-callbacks-in-sdl3) instead of a `main` function with a loop.
The [existing way](../getting_started.md#example-game-window) of writing a game loop still works on most platforms, including [web](./emscripten.md).
However, opting in to callbacks provides several benefits, such as:

* The application is not [frozen during a window move or resize](https://wiki.libsdl.org/SDL3/AppFreezeDuringDrag).
* Better platform integration, such as:
  * Frame pacing using [`requestAnimationFrame`](https://developer.mozilla.org/en-US/docs/Web/API/Window/requestAnimationFrame) on the web.
  * [Game Center integration](https://wiki.libsdl.org/SDL3/README-ios#game-center) on iOS.

## Opting In

In exactly one source file, define both `SDL_MAIN_USE_CALLBACKS` and `CF_MAIN` before including `cute.h`.
SDL then provides `main`, and your app implements `SDL_AppInit`, `SDL_AppIterate`, and `SDL_AppQuit` instead.

```c
#define SDL_MAIN_USE_CALLBACKS
#define CF_MAIN
#include <cute.h>

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
	CF_Result result = cf_make_app("Fancy Window Title", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (cf_is_error(result)) return SDL_APP_FAILURE;
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	cf_app_update(NULL);

	// All your game logic and updates go here...

	cf_app_draw_onto_screen(true);
	return cf_app_is_running() ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	cf_destroy_app();
}
```

> [!IMPORTANT]
> Both macros must be defined before the first `#include <cute.h>` in that file.

`CF_MAIN` on its own (without `SDL_MAIN_USE_CALLBACKS`) keeps its usual meaning: SDL provides the platform entry point, such as `WinMain` or the iOS/Android entry point, and calls your regular `main` function.

## Migrating from a Main Loop to Callbacks

Each part of a traditional `main` function moves into one callback:

| Main loop                                      | Callbacks                                              |
|------------------------------------------------|--------------------------------------------------------|
| Code before the loop, such as `cf_make_app`    | `SDL_AppInit`                                          |
| The body of `while (cf_app_is_running())`      | `SDL_AppIterate`, called once per frame                |
| Code after the loop, such as `cf_destroy_app`  | `SDL_AppQuit`                                          |
| Local variables that live across frames        | Globals, or a struct passed through `appstate`         |

`SDL_AppIterate` must do a single frame and return.
Don't run a loop inside it.

The [Game Loop and Time](./game_loop_and_time.md) topic applies unchanged: call [`cf_app_update`](../app/function/cf_app_update.md) at the beginning of each `SDL_AppIterate`, passing an update function if you use a fixed timestep.

The pointer written to `*appstate` in `SDL_AppInit` is passed back to the other callbacks.

## Quitting

Return `SDL_APP_SUCCESS` (or `SDL_APP_FAILURE`) from `SDL_AppIterate` to stop the app.
Closing the window or calling [`cf_app_signal_shutdown`](../app/function/cf_app_signal_shutdown.md) makes [`cf_app_is_running`](../app/function/cf_app_is_running.md) return false, so returning based on it, as in the example above, handles both cases.

SDL then calls `SDL_AppQuit`, which should clean up and call [`cf_destroy_app`](../app/function/cf_destroy_app.md).

> [!NOTE]
> `SDL_AppQuit` is also called when `SDL_AppInit` returns `SDL_APP_FAILURE`, so it must cope with an app that was never fully created.
> [`cf_destroy_app`](../app/function/cf_destroy_app.md) does nothing if no app exists.

## Handling events

With callbacks, SDL delivers events to `SDL_AppEvent` instead of letting the app poll for them.
`cute.h` provides a default `SDL_AppEvent` that forwards every event to CF with [`cf_app_push_event`](../app/function/cf_app_push_event.md).
Input is then handled at the same point as in a main loop: during [`cf_app_update`](../app/function/cf_app_update.md).
Functions such as [`cf_key_just_pressed`](../input/function/cf_key_just_pressed.md) continue to work unchanged.

### Handling Events Yourself

Define `CF_MAIN_CUSTOM_APP_EVENT` to write your own `SDL_AppEvent`.
Handle what you need, then forward the event to CF:

```c
#define SDL_MAIN_USE_CALLBACKS
#define CF_MAIN
#define CF_MAIN_CUSTOM_APP_EVENT
#include <cute.h>

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	switch (event->type) {
	case SDL_EVENT_DROP_FILE:
		// Copy the path if you need it after this call returns.
		printf("Dropped: %s\n", event->drop.data);
		break;
	}

	cf_app_push_event(event);
	return SDL_APP_CONTINUE;
}
```

A custom `SDL_AppEvent` is the only place to handle:

* **App lifecycle events**: `SDL_EVENT_TERMINATING`, `SDL_EVENT_LOW_MEMORY`, `SDL_EVENT_WILL_ENTER_BACKGROUND`...
  SDL delivers these immediately, possibly from another thread, and mobile platforms expect them to be handled before `SDL_AppEvent` returns.
  CF ignores them when they're pushed.
* **Data behind pointers in an event**: For example: the file name of `SDL_EVENT_DROP_FILE` or the MIME types of `SDL_EVENT_CLIPBOARD_UPDATE`.
  SDL frees it before CF handles the queued event, so read or copy it in `SDL_AppEvent`.
  The text of `SDL_EVENT_TEXT_INPUT` and `SDL_EVENT_TEXT_EDITING` are copied by CF so [`cf_input_text_pop_utf32`](../input/function/cf_input_text_pop_utf32.md) and Dear ImGui text input keep working.
* **Stopping the app from an event** by returning `SDL_APP_SUCCESS` or `SDL_APP_FAILURE`.

> [!IMPORTANT]
> Every event must reach `cf_app_push_event`, or CF never sees input, window events, or the quit request.
> Avoid calling `SDL_PollEvent` or `SDL_PumpEvents` yourself inside `SDL_AppIterate`.

## Web

With callbacks, SDL drives frames with `requestAnimationFrame`, so there's no need to call `emscripten_set_main_loop` as shown in [Web Builds with Emscripten](./emscripten.md).

To run frames at a fixed rate instead, set [`SDL_HINT_MAIN_CALLBACK_RATE`](https://wiki.libsdl.org/SDL3/SDL_HINT_MAIN_CALLBACK_RATE) hint.
Alternatively, use a [fixed timestep](./game_loop_and_time.md#fixed-timestep).

> [!NOTE]
> CF still links with `-sASYNCIFY` on the web, and [`cf_sleep`](../time/function/cf_sleep.md) still yields to the browser.
> Do not set `SDL_HINT_EMSCRIPTEN_ASYNCIFY` to `"0"`.
> Without it, `cf_sleep(0)` stops yielding and CF can hang while waiting on the GPU.

## Sample

[`app_callbacks.c`](https://github.com/RandyGaul/cute_framework/blob/master/samples/app_callbacks.c) is a complete app using callbacks, with keyboard, mouse, and text input.
