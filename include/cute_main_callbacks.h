/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_MAIN_CALLBACKS_H
#define CF_MAIN_CALLBACKS_H

#include "cute_app.h"
#include <SDL3/SDL_main.h>

// Default SDL main callbacks for apps built with `SDL_MAIN_USE_CALLBACKS`.
// cute.h includes this header when `CF_MAIN`is defined, which must happen in
// exactly one source file: the one that gets SDL's `main`.

//--------------------------------------------------------------------------------------------------
// C API

#ifdef SDL_MAIN_USE_CALLBACKS
#ifndef CF_MAIN_CUSTOM_APP_EVENT

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Forwards every event to CF, so an app only has to write `SDL_AppInit`, `SDL_AppIterate`, and `SDL_AppQuit`.
// To opt out, define `CF_MAIN_CUSTOM_APP_EVENT`, then write your own `SDL_AppEvent` and call `cf_app_push_event` from it.
SDL_AppResult SDLCALL SDL_AppEvent(void* appstate, SDL_Event* event)
{
	(void)appstate;
	cf_app_push_event(event);
	return SDL_APP_CONTINUE;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CF_MAIN_CUSTOM_APP_EVENT
#endif // SDL_MAIN_USE_CALLBACKS

#endif // CF_MAIN_CALLBACKS_H
