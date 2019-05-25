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

#include <cute_audio.h>
#include <cute_alloc.h>

#include <internal/cute_app_internal.h>

#define STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>

#define CUTE_SOUND_IMPLEMENTATION
#define CUTE_SOUND_FORCE_SDL
#include <cute/cute_sound.h>
// TODO: Hookup allocs to cute_sound internals

namespace cute
{

struct audio_t : public cs_loaded_sound_t
{
	audio_t() { }
	audio_t(const cs_loaded_sound_t& sound) { *((cs_loaded_sound_t*)this) = sound; }

	promise_t user_promise;
	promise_t play_promise;
	void* mem_ctx;
};

using sound_t = cs_playing_sound_t;

static CUTE_INLINE audio_t* s_audio_make(audio_t audio_struct, void* user_allocator_context)
{
	if (audio_struct.channel_count) {
		audio_t* audio = (audio_t*)CUTE_ALLOC(sizeof(audio_t), user_allocator_context);
		CUTE_PLACEMENT_NEW(audio) audio_t;
		*audio = audio_struct;
		return audio;
	} else {
		return NULL;
	}
}

audio_t* audio_load_ogg(const char* path, void* user_allocator_context)
{
	audio_t audio_struct = cs_load_ogg(path);
	return s_audio_make(audio_struct, user_allocator_context);
}

audio_t* audio_load_wav(const char* path, void* user_allocator_context)
{
	audio_t audio_struct = cs_load_wav(path);
	return s_audio_make(audio_struct, user_allocator_context);
}

audio_t* audio_load_ogg_from_memory(void* memory, int byte_count, void* user_allocator_context)
{
	audio_t audio_struct;
	cs_read_mem_ogg(memory, byte_count, &audio_struct);
	return s_audio_make(audio_struct, user_allocator_context);
}

audio_t* audio_load_wav_from_memory(void* memory, int byte_count, void* user_allocator_context)
{
	audio_t audio_struct;
	cs_read_mem_wav(memory, byte_count, &audio_struct);
	return s_audio_make(audio_struct, user_allocator_context);
}

struct audio_param_t
{
	const char* path = NULL;
	void* memory = NULL;
	int byte_count = NULL;
	promise_t user_promise;
	void* mem_ctx = NULL;
};

static void s_stream_ogg_task_fn(void* param)
{
	audio_param_t* audio_param = (audio_param_t*)param;
	audio_t* audio = NULL;
	if (audio_param->path) {
		audio = audio_load_ogg(audio_param->path, audio_param->mem_ctx);
	} else {
		audio = audio_load_ogg_from_memory(audio_param->memory, audio_param->byte_count, audio_param->mem_ctx);
	}
	audio_param->user_promise.invoke(audio ? error_success() : error_failure("Failed to load ogg file."), audio);
	CUTE_FREE(audio_param, audio_param->mem_ctx);
}

static void s_stream_wav_task_fn(void* param)
{
	audio_param_t* audio_param = (audio_param_t*)param;
	audio_t* audio = NULL;
	if (audio_param->path) {
		audio = audio_load_wav(audio_param->path, audio_param->mem_ctx);
	} else {
		audio = audio_load_wav_from_memory(audio_param->memory, audio_param->byte_count, audio_param->mem_ctx);
	}
	audio_param->user_promise.invoke(audio ? error_success() : error_failure("Failed to load wav file."), audio);
	CUTE_FREE(audio_param, audio_param->mem_ctx);
}

void audio_stream_ogg(app_t* app, const char* path, promise_t promise, void* user_allocator_context)
{
	audio_param_t* param = (audio_param_t*)CUTE_ALLOC(sizeof(audio_param_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(param) audio_param_t;
	param->path = path;
	param->user_promise = promise;
	param->mem_ctx = user_allocator_context;

	threadpool_add_task(app->threadpool, s_stream_ogg_task_fn, param);
	cute_threadpool_kick(app->threadpool);
}

void audio_stream_wav(app_t* app, const char* path, promise_t promise, void* user_allocator_context)
{
	audio_param_t* param = (audio_param_t*)CUTE_ALLOC(sizeof(audio_param_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(param) audio_param_t;
	param->path = path;
	param->user_promise = promise;
	param->mem_ctx = user_allocator_context;

	threadpool_add_task(app->threadpool, s_stream_wav_task_fn, param);
	cute_threadpool_kick(app->threadpool);
}

void audio_stream_ogg_from_memory(app_t* app, void* memory, int byte_count, promise_t promise, void* user_allocator_context)
{
	audio_param_t* param = (audio_param_t*)CUTE_ALLOC(sizeof(audio_param_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(param) audio_param_t;
	param->memory = memory;
	param->byte_count = byte_count;
	param->user_promise = promise;
	param->mem_ctx = user_allocator_context;

	threadpool_add_task(app->threadpool, s_stream_ogg_task_fn, param);
	cute_threadpool_kick(app->threadpool);
}

void audio_stream_wav_from_memory(app_t* app, void* memory, int byte_count, promise_t promise, void* user_allocator_context)
{
	audio_param_t* param = (audio_param_t*)CUTE_ALLOC(sizeof(audio_param_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(param) audio_param_t;
	param->memory = memory;
	param->byte_count = byte_count;
	param->user_promise = promise;
	param->mem_ctx = user_allocator_context;

	threadpool_add_task(app->threadpool, s_stream_wav_task_fn, param);
	cute_threadpool_kick(app->threadpool);
}

error_t audio_destroy(audio_t* audio)
{
	if (audio_ref_count(audio) == 0) {
		void* mem_ctx = audio->mem_ctx;
		cs_free_sound(audio);
		CUTE_FREE(audio, mem_ctx);
		return error_success();
	} else {
		return error_failure("Reference count for audio was not zero.");
	}
}

int audio_ref_count(audio_t* audio_source)
{
	return audio_source->playing_count;
}

// -------------------------------------------------------------------------------------------------

error_t music_play(app_t* app, audio_t* audio_source, float fade_in_time, float delay)
{
	return error_success();
}

void music_stop(app_t* app, float fade_out_time)
{
}

void music_set_volume(app_t* app, float volume)
{
}

void music_set_pitch(app_t* app, float pitch)
{
}

void music_set_loop(app_t* app, int loop)
{
}

void music_pause(app_t* app)
{
}

void music_resume(app_t* app)
{
}

error_t music_switch_to(app_t* app, audio_t* audio_source, float fade_out_time, float fade_in_time)
{
	return error_success();
}

error_t music_crossfade_to(app_t* app, audio_t* audio_source, float cross_fade_time)
{
	return error_success();
}

// -------------------------------------------------------------------------------------------------

void sound_play(app_t* app, audio_t* audio_source, sound_def_t def)
{
}

// -------------------------------------------------------------------------------------------------

void audio_set_pan(app_t* app, float pan)
{
}

void audio_set_global_volume(app_t* app, float volume)
{
}

void audio_set_sound_volume(app_t* app, float volume)
{
}

namespace internal
{
	int sound_instance_size() { return sizeof(sound_t); }
}

}

#undef STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>
