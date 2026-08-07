/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_MAIN_H
#define CF_MAIN_H

// Deliberately not included by cute.h -- the entry-point glue below has external linkage and must
// live in exactly one translation unit, so including this header is itself the opt-in. Include it
// alongside cute.h in the one file that owns your app's entry point, not anywhere else.

#if defined(CF_MAIN)
#	error "cute_main.h: define either CF_MAIN or include cute_main.h, not both -- they are two different entry points."
#endif

// If something already pulled in <SDL3/SDL_main.h>, its include guard would silently swallow the
// callback machinery below and your program would link with no `main` at all. Catch it here
// instead: include cute_main.h before any other header that reaches SDL_main.h.
#ifdef SDL_main_h_
#	error "cute_main.h: <SDL3/SDL_main.h> was already included. Include cute_main.h first, before any header that reaches it."
#endif

#define SDL_MAIN_USE_CALLBACKS
#include "cute_app.h"
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_log.h>

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @function cf_main_init
 * @category app
 * @brief    Your app's startup function -- implement it and call `cf_make_app` inside.
 * @param    argc  The argument count, forwarded from the entry point.
 * @param    argv  The argument vector, forwarded from the entry point. Pass `argv[0]` along to `cf_make_app`.
 * @return   Return the `CF_Result` from `cf_make_app` (or your own error). An error result aborts startup.
 * @remarks  Include `cute_main.h` in exactly one source file to let the host drive your main loop instead of
 *           writing your own `while` loop -- don't define `main` yourself, implement `cf_main_init`,
 *           `cf_main_update` and `cf_main_quit` instead. All three must live in this one file: in C++ they
 *           are only declared here, so defining one in a file that lacks this include gives it C++ linkage
 *           and you get an undefined-symbol error at link time.
 *
 *           Called once before the first `cf_main_update`. Your `cf_make_app` call must include
 *           `CF_APP_OPTIONS_MAIN_CALLBACKS_BIT` -- startup fails with a message if it doesn't, rather than
 *           leaving you with a window that silently receives no input.
 *
 *           The exact same code then runs on desktop, web, and mobile -- no platform-specific main-loop
 *           forks needed. The app quits once `cf_app_is_running` returns false (window close, or call
 *           `cf_app_signal_shutdown`). The platform decides the frame rate, so drive animation off
 *           `CF_DELTA_TIME` rather than assuming 60hz.
 *
 *           Note for fixed-timestep games (`cf_set_fixed_timestep`): events are delivered between frames,
 *           so all sub-steps of one frame share the same input snapshot (the classic loop re-polls the OS
 *           queue per sub-step).
 *
 *           Implemented on [SDL's main callbacks](https://wiki.libsdl.org/SDL3/README-main-functions): CF
 *           defines `SDL_AppInit`/`SDL_AppIterate`/`SDL_AppEvent`/`SDL_AppQuit` for you. If you want those
 *           four yourself, skip this header entirely: define `SDL_MAIN_USE_CALLBACKS`, write them, pass
 *           `CF_APP_OPTIONS_MAIN_CALLBACKS_BIT` to `cf_make_app`, and forward every event to
 *           `cf_app_process_event` (see cute_app.h).
 * @related  cf_main_update cf_main_quit cf_make_app cf_app_process_event CF_AppOptionFlagBits
 */
CF_Result cf_main_init(int argc, char* argv[]);

/**
 * @function cf_main_update
 * @category app
 * @brief    Your app's frame function -- one frame of the main loop.
 * @remarks  Call `cf_app_update` at the top and `cf_app_draw_onto_screen` at the bottom, with your game logic in
 *           between, exactly like the body of a classic `while (cf_app_is_running())` loop. The platform decides
 *           the call rate (e.g. the browser's requestAnimationFrame on web). The app quits once `cf_app_is_running`
 *           returns false -- call `cf_app_signal_shutdown` to request that.
 * @related  cf_main_init cf_main_quit cf_app_update cf_app_draw_onto_screen cf_app_signal_shutdown
 */
void cf_main_update(void);

/**
 * @function cf_main_quit
 * @category app
 * @brief    Your app's cleanup function -- called once at shutdown.
 * @remarks  Free your own resources here. `cf_destroy_app` is called for you right afterwards. Note this runs even
 *           when `cf_main_init` failed, so don't assume the app was created.
 * @related  cf_main_init cf_main_update cf_destroy_app
 */
void cf_main_quit(void);

SDL_AppResult SDLCALL SDL_AppInit(void** appstate, int argc, char* argv[])
{
	(void)appstate;
	if (cf_is_error(cf_main_init(argc, argv))) return SDL_APP_FAILURE;
	// Without the bit CF would keep polling an event queue the callback harness already drained,
	// leaving a window with no keyboard, no mouse and no way to quit. Fail loudly instead.
	if (!(cf_app_get_options() & CF_APP_OPTIONS_MAIN_CALLBACKS_BIT)) {
		SDL_SetError("cf_make_app was called without CF_APP_OPTIONS_MAIN_CALLBACKS_BIT, which cute_main.h requires. Add it to the options passed to cf_make_app in cf_main_init.");
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL SDL_AppIterate(void* appstate)
{
	(void)appstate;
	cf_main_update();
	return cf_app_is_running() ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}

SDL_AppResult SDLCALL SDL_AppEvent(void* appstate, SDL_Event* event)
{
	(void)appstate;
	cf_app_process_event(event);
	return SDL_APP_CONTINUE;
}

void SDLCALL SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	(void)appstate;
	(void)result;
	cf_main_quit();
	cf_destroy_app();
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CF_MAIN_H
