// Runs the app from SDL's main callbacks instead of a main loop.
//
// With `SDL_MAIN_USE_CALLBACKS` defined, `CF_MAIN` makes cute.h include SDL_main.h, which provides
// `main` and calls the `SDL_App*` functions below. SDL decides when each frame runs, which is
// required on platforms that own the loop, such as the browser. cute_main_callbacks.h supplies a
// default `SDL_AppEvent` that forwards events to CF with `cf_app_push_event`.
#define SDL_MAIN_USE_CALLBACKS
#define CF_MAIN
#include <cute.h>
#include <stdio.h>

typedef struct AppState
{
	char* text; // Typed text, as a dynamic string.
	int clicks;
} AppState;

static AppState state;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
	CF_Result result = cf_make_app(
		"App Callbacks",
		0, 0, 0, 640, 480,
		CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT,
		argv[0]
	);
	if (cf_is_error(result)) return SDL_APP_FAILURE;

	// Starts SDL text input, so typed characters arrive as text events.
	cf_input_enable_ime();

	*appstate = &state;
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	AppState* s = (AppState*)appstate;

	// One call per frame, the same as the body of a main loop.
	cf_app_update(NULL);

	if (cf_key_just_pressed(CF_KEY_ESCAPE)) {
		cf_app_signal_shutdown();
	}
	while (cf_input_text_has_data()) {
		sappend_UTF8(s->text, cf_input_text_pop_utf32());
	}
	if (cf_key_just_pressed(CF_KEY_BACKSPACE) && s->text) {
		sclear(s->text);
	}
	if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT)) {
		++s->clicks;
	}

	int w, h;
	cf_app_get_size(&w, &h);
	CF_V2 top_left = cf_v2(-(float)w * 0.5f + 8, (float)h * 0.5f - 8);

	char status[128];
	snprintf(status, sizeof(status), "Clicks: %d. Type to enter text, backspace clears, escape quits.", s->clicks);
	cf_draw_text(status, top_left, -1);
	if (s->text) {
		cf_draw_text(s->text, cf_v2(top_left.x, top_left.y - 24), -1);
	}

	cf_draw_circle_fill2(cf_screen_to_world(cf_v2(cf_mouse_x(), cf_mouse_y())), 8.0f);

	cf_app_draw_onto_screen(true);

	// Closing the window also stops the app. Returning SDL_APP_SUCCESS makes SDL call `SDL_AppQuit`.
	return cf_app_is_running() ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	// Also called when `SDL_AppInit` fails, so this must cope with a partially initialized app.
	sfree(state.text);
	cf_destroy_app();
}
