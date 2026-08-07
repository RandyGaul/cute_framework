// Lets the host drive the main loop instead of writing your own `while` loop. The same three
// functions run on desktop, web and mobile -- no `#ifdef CF_EMSCRIPTEN` fork anywhere.
//
// Try dragging the window edge and holding it: the orbiting dot keeps moving and the clock keeps
// ticking, where a classic `while (cf_app_is_running())` loop freezes until you let go.
//
// Including cute_main.h is itself the opt-in -- do it in exactly one source file, the one that
// owns your app's entry point.
#include <cute.h>
#include <cute_main.h>

#include <stdio.h>

static float s_elapsed;
static int s_presses;

CF_Result cf_main_init(int argc, char* argv[])
{
	(void)argc;
	// CF checks this result for you -- no `if (cf_is_error(result)) return -1;` needed.
	// CF_APP_OPTIONS_MAIN_CALLBACKS_BIT is required here: it stops CF from polling an event
	// queue it no longer owns. Leave it out and CF refuses to start rather than handing you a
	// window that silently receives no input.
	return cf_make_app("Main Callbacks", 0, 0, 0, 640, 480,
		CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT | CF_APP_OPTIONS_MAIN_CALLBACKS_BIT, argv[0]);
}

void cf_main_update(void)
{
	// One frame of the main loop -- exactly the body of a classic `while` loop.
	cf_app_update(NULL);

	// The host picks the frame rate (requestAnimationFrame on web), so animate off delta time
	// instead of assuming 60hz.
	s_elapsed += CF_DELTA_TIME;
	if (cf_key_just_pressed(CF_KEY_SPACE)) s_presses++;

	cf_draw_circle_fill2(cf_v2(cosf(s_elapsed * 2.0f) * 120.0f, sinf(s_elapsed * 2.0f) * 120.0f), 16.0f);

	char buf[64];
	snprintf(buf, sizeof(buf), "elapsed %.2fs", s_elapsed);
	cf_draw_text(buf, cf_v2(-60, 30), -1);
	snprintf(buf, sizeof(buf), "space pressed %d", s_presses);
	cf_draw_text(buf, cf_v2(-60, 10), -1);

	cf_app_draw_onto_screen(true);
}

void cf_main_quit(void)
{
	// Free your own resources here -- cf_destroy_app runs for you right afterwards.
}
