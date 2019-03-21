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
#include <cute_app_internal.h>
#include <cute_alloc.h>
#include <cute_buffer.h>
#include <cute_audio.h>
#include <cute_concurrency.h>
#include <cute_crypto_utils.h>
#include <cute_file_system.h>
#include <cute_file_system_utils.h>
#include <cute_net_utils.h>

#include <SDL2/SDL.h>
#include <glad/glad.h>

#define CUTE_SOUND_FORCE_SDL
#include <cute/cute_sound.h>

#define CUTE_SERIALIZE_IMPLEMENTATION
#define SERIALIZE_UNIT_TESTS
#define SERIALIZE_FREAD(buffer, element_size, element_count, stream) cute::file_system_read((cute::file_t*)stream, buffer, element_size * element_count)
#define SERIALIZE_FWRITE(buffer, element_size, element_count, stream) cute::file_system_write((cute::file_t*)stream, buffer, element_size * element_count)
#include <cute/cute_serialize.h>

namespace cute
{

app_t* app_make(const char* window_title, int x, int y, int w, int h, uint32_t options, const char* argv0, void* user_allocator_context)
{
	app_t* app = (app_t*)CUTE_ALLOC(sizeof(app_t), user_allocator_context);
	CUTE_CHECK(app);

	if (!(options & APP_OPTIONS_NO_GFX)) {
		SDL_InitSubSystem(SDL_INIT_VIDEO);
	}

	if (!(options & APP_OPTIONS_NO_AUDIO)) {
		SDL_InitSubSystem(SDL_INIT_AUDIO);
	}

	Uint32 flags = 0;
	if (options & APP_OPTIONS_GFX_GL) flags |= SDL_WINDOW_OPENGL;
	if (options & APP_OPTIONS_GFX_GLES) flags |= SDL_WINDOW_OPENGL;
	if (options & APP_OPTIONS_FULLSCREEN) flags |= SDL_WINDOW_FULLSCREEN;
	if (options & APP_OPTIONS_RESIZABLE) flags |= SDL_WINDOW_RESIZABLE;

	SDL_Window* window;
	if (options & APP_OPTIONS_WINDOW_POS_CENTERED) {
		window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
	} else {
		window = SDL_CreateWindow(window_title, x, y, w, h, flags);
	}
	CUTE_CHECK(window);
	CUTE_PLACEMENT_NEW(window) app_t;
	app->window = window;
	app->mem_ctx = user_allocator_context;

	if (options & APP_OPTIONS_GFX_GL) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	}

	if (options & APP_OPTIONS_GFX_GLES) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	}

	if ((options & APP_OPTIONS_GFX_GL) | (options & APP_OPTIONS_GFX_GLES)) {
		SDL_GL_SetSwapInterval(0);
		SDL_GL_CreateContext(window);
		gladLoadGLLoader(SDL_GL_GetProcAddress);
	}

	if (!(options & APP_OPTIONS_NO_AUDIO)) {
		app->cs = cs_make_context(NULL, 44100, 5, 1, 0);
		if (app->cs) cs_spawn_mix_thread(app->cs);
	}

	if (!(options & APP_OPTIONS_NO_NET)) {
		CUTE_CHECK(internal::crypto_init());
		CUTE_CHECK(internal::net_init());
	}

	int num_cores = core_count() - 1;
	if (num_cores) {
		app->threadpool = threadpool_create(num_cores, user_allocator_context);
	}

	CUTE_CHECK(internal::file_system_init(argv0));

	return app;

cute_error:
	CUTE_FREE(app, user_allocator_ctx);
	return NULL;
}

void app_destroy(app_t* app)
{
	if (app->cs) cs_shutdown_context(app->cs);
	SDL_DestroyWindow(app->window);
	SDL_Quit();
	CUTE_FREE(app, app->mem_ctx);
}

int is_running(app_t* app)
{
	return app->running;
}

void stop_running(app_t* app)
{
	app->running = 0;
}

void app_update(app_t* app, float dt)
{
	// TODO: Implement me.
	// Should poll input, render, do net stuff, deal with events.
}

}
