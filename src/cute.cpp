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

#include <cute.h>
#include <cute_internal.h>
#include <cute_alloc.h>
#include <cute_buffer.h>
#include <cute_audio.h>
#include <cute_concurrency.h>

#include <SDL2/SDL.h>
#include <glad/glad.h>

#define CUTE_SOUND_FORCE_SDL
#include <cute/cute_sound.h>

namespace cute
{

cute_t* cute_make(const char* window_title, int x, int y, int w, int h, uint32_t options, void* user_allocator_context)
{
	cute_t* cute = (cute_t*)CUTE_ALLOC(sizeof(cute_t), user_allocator_context);
	if (!cute) return NULL;

	if (!(options & CUTE_OPTIONS_NO_GFX)) {
		SDL_InitSubSystem(SDL_INIT_VIDEO);
	}

	if (!(options & CUTE_OPTIONS_NO_AUDIO)) {
		SDL_InitSubSystem(SDL_INIT_AUDIO);
	}

	Uint32 flags = 0;
	if (options & CUTE_OPTIONS_GFX_GL) flags |= SDL_WINDOW_OPENGL;
	if (options & CUTE_OPTIONS_GFX_GLES) flags |= SDL_WINDOW_OPENGL;
	if (options & CUTE_OPTIONS_FULLSCREEN) flags |= SDL_WINDOW_FULLSCREEN;
	if (options & CUTE_OPTIONS_RESIZABLE) flags |= SDL_WINDOW_RESIZABLE;

	SDL_Window* window;
	if (options & CUTE_OPTIONS_WINDOW_POS_CENTERED) {
		window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
	} else {
		window = SDL_CreateWindow(window_title, x, y, w, h, flags);
	}
	CUTE_PLACEMENT_NEW(window) cute_t;
	cute->window = window;
	cute->mem_ctx = user_allocator_context;

	if (options & CUTE_OPTIONS_GFX_GL) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	}

	if (options & CUTE_OPTIONS_GFX_GLES) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	}

	if ((options & CUTE_OPTIONS_GFX_GL) | (options & CUTE_OPTIONS_GFX_GLES)) {
		SDL_GL_SetSwapInterval(0);
		SDL_GL_CreateContext(window);
		gladLoadGLLoader(SDL_GL_GetProcAddress);
	}

	if (!(options & CUTE_OPTIONS_NO_AUDIO)) {
		cute->cs = cs_make_context(NULL, 44100, 5, 1, 0);
		if (cute->cs) cs_spawn_mix_thread(cute->cs);
	}

	int num_cores = core_count() - 1;
	if (num_cores) {
		cute->threadpool = threadpool_create(num_cores, user_allocator_context);
	}

	return cute;
}

void cute_destroy(cute_t* cute)
{
	if (cute->cs) cs_shutdown_context(cute->cs);
	SDL_DestroyWindow(cute->window);
	SDL_Quit();
	CUTE_FREE(cute, cute->mem_ctx);
}

int is_running(cute_t* cute)
{
	return cute->running;
}

void stop_running(cute_t* cute)
{
	cute->running = 0;
}

void cute_update(cute_t* cute, float dt)
{
	// TODO: Implement me.
	// Should poll input, render, do net stuff, deal with events.
}

float calc_dt()
{
	static int first = 1;
	static double inv_freq;
	static uint64_t prev;

	uint64_t now = SDL_GetPerformanceCounter();

	if (first) {
		first = 0;
		prev = now;
		inv_freq = 1.0 / (double)SDL_GetPerformanceFrequency();
	}

	float dt = (float)((double)(now - prev) * inv_freq);
	prev = now;
	return dt;
}

}
