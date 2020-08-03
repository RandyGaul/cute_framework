/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <cute_app.h>
#include <cute_alloc.h>
#include <cute_buffer.h>
#include <cute_audio.h>
#include <cute_concurrency.h>
#include <cute_crypto_utils.h>
#include <cute_file_system.h>
#include <cute_file_system_utils.h>
#include <cute_net.h>
#include <cute_c_runtime.h>
#include <cute_kv.h>
#include <cute_gfx.h>

#include <internal/cute_defines_internal.h>
#include <internal/cute_app_internal.h>
#include <internal/cute_file_system_internal.h>
#include <internal/cute_net_internal.h>
#include <internal/cute_crypto_internal.h>
#include <internal/cute_audio_internal.h>
#include <internal/cute_input_internal.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <glad/glad.h>

#ifdef _WIN32
#include <SDL2/SDL_syswm.h>
#endif

#define CUTE_SOUND_FORCE_SDL
#include <cute/cute_sound.h>

#define CUTE_SERIALIZE_IMPLEMENTATION
#define SERIALIZE_UNIT_TESTS
#define SERIALIZE_FREAD(buffer, element_size, element_count, stream) cute::file_system_read((cute::file_t*)stream, buffer, element_size * element_count)
#define SERIALIZE_FWRITE(buffer, element_size, element_count, stream) cute::file_system_write((cute::file_t*)stream, buffer, element_size * element_count)
#include <cute/cute_serialize.h>

namespace cute
{

// TODO: Refactor to use error_t reporting.

app_t* app_make(const char* window_title, int x, int y, int w, int h, uint32_t options, const char* argv0, void* user_allocator_context)
{
	SDL_SetMainReady();

	app_t* app = (app_t*)CUTE_ALLOC(sizeof(app_t), user_allocator_context);
	CUTE_CHECK_POINTER(app);
	app->options = options;

	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
		CUTE_FREE(app, user_allocator_context);
		return NULL;
	}

	Uint32 flags = 0;
	if (options & CUTE_APP_OPTIONS_OPENGL_CONTEXT) flags |= SDL_WINDOW_OPENGL;
	if (options & CUTE_APP_OPTIONS_OPENG_GL_ES_CONTEXT) flags |= SDL_WINDOW_OPENGL;
	if (options & CUTE_APP_OPTIONS_FULLSCREEN) flags |= SDL_WINDOW_FULLSCREEN;
	if (options & CUTE_APP_OPTIONS_RESIZABLE) flags |= SDL_WINDOW_RESIZABLE;
	if (options & CUTE_APP_OPTIONS_HIDDEN) flags |= (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED);

	SDL_Window* window;
	if (options & CUTE_APP_OPTIONS_WINDOW_POS_CENTERED) {
		window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
	} else {
		window = SDL_CreateWindow(window_title, x, y, w, h, flags);
	}
	CUTE_CHECK_POINTER(window);
	CUTE_PLACEMENT_NEW(app) app_t;
	app->window = window;
	app->mem_ctx = user_allocator_context;
	app->w = w;
	app->h = h;
	app->x = x;
	app->y = y;
	app->render_w = w;
	app->render_h = h;

#ifdef _WIN32
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;
	app->platform_handle = hwnd;
#endif

	if (options & CUTE_APP_OPTIONS_OPENGL_CONTEXT) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	}

	if (options & CUTE_APP_OPTIONS_OPENG_GL_ES_CONTEXT) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	}

	if ((options & CUTE_APP_OPTIONS_OPENGL_CONTEXT) | (options & CUTE_APP_OPTIONS_OPENG_GL_ES_CONTEXT)) {
		SDL_GL_SetSwapInterval(0);
		SDL_GL_CreateContext(window);
		gladLoadGLLoader(SDL_GL_GetProcAddress);
	}

	int num_threads_to_spawn = core_count() - 1;
	if (num_threads_to_spawn) {
		app->threadpool = threadpool_create(num_threads_to_spawn, user_allocator_context);
	}

	if (internal::file_system_init(argv0).is_error()) {
		goto cute_error;
	}

	return app;

cute_error:
	CUTE_FREE(app, user_allocator_ctx);
	return NULL;
}

void app_destroy(app_t* app)
{
	if (app->cute_sound) cs_shutdown_context(app->cute_sound);
	SDL_DestroyWindow(app->window);
	SDL_Quit();
	cute_threadpool_destroy(app->threadpool);
	audio_system_destroy(app->audio_system);
	int schema_count = app->entity_parsed_schemas.count();
	kv_t** schemas = app->entity_parsed_schemas.items();
	for (int i = 0; i < schema_count; ++i) kv_destroy(schemas[i]);
	app->~app_t();
	CUTE_FREE(app, app->mem_ctx);
	internal::file_system_destroy();
}

bool app_is_running(app_t* app)
{
	return app->running;
}

void app_stop_running(app_t* app)
{
	app->running = 0;
}

void app_update(app_t* app, float dt)
{
	app->dt = dt;
	pump_input_msgs(app);
	if (app->audio_system) audio_system_update(app->audio_system, dt);
}

// TODO - Move these init functions into audio/net headers.

error_t app_init_net(app_t* app)
{
	error_t err = crypto_init();
	if (err.is_error()) return err;
	return net_init();
}

error_t app_init_audio(app_t* app, int max_simultaneous_sounds)
{
	app->cute_sound = cs_make_context(NULL, 44100, 1024, 0, app->mem_ctx);
	if (app->cute_sound) {
		cs_spawn_mix_thread(app->cute_sound);
		app->audio_system = audio_system_make(max_simultaneous_sounds, app->mem_ctx);
		return error_success();
	} else {
		return error_failure(cs_error_reason);
	}
}

}
