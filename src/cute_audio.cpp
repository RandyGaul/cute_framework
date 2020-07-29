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
#include <cute_array.h>
#include <cute_doubly_list.h>

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

int audio_ref_count(audio_t* audio)
{
	return audio->playing_count;
}

// -------------------------------------------------------------------------------------------------

struct audio_instance_t
{
	cs_playing_sound_t sound;
	list_node_t node;
};

enum music_state_t
{
	MUSIC_STATE_NONE,
	MUSIC_STATE_PLAYING,
	MUSIC_STATE_FADE_OUT,
	MUSIC_STATE_FADE_IN,
	MUSIC_STATE_FADE_SWITCH_TO,
	MUSIC_STATE_FADE_CROSSFADE,
	MUSIC_STATE_PAUSED
};

struct audio_system_t
{
	float global_volume = 1.0f;
	float music_volume = 1.0f;
	float sound_volume = 1.0f;

	float fade = 0;
	music_state_t music_state = MUSIC_STATE_NONE;
	audio_instance_t* music_playing = NULL;
	audio_instance_t* music_next = NULL;

	audio_instance_t* playing_sounds_buffer;
	list_t playing_sounds;
	list_t free_sounds;
	void* mem_ctx = NULL;
};

error_t music_play(app_t* app, audio_t* audio_source, float fade_in_time)
{
	audio_system_t* as = app->audio_system;

	if (as->music_state == MUSIC_STATE_PLAYING) {
		return error_failure("Already playing some music. Use `music_switch_to` or `music_crossfade` instead.");
	} else if (as->music_state != MUSIC_STATE_NONE) {
		return error_failure("Currently fading other music, can not start playing new music right now.");
	}

	if (fade_in_time < 0) fade_in_time = 0;
	if (fade_in_time) as->music_state = MUSIC_STATE_FADE_IN;
	else as->music_state = MUSIC_STATE_PLAYING;
	as->fade = fade_in_time;

	if (list_empty(&as->free_sounds)) {
		return error_failure("Unable to play music. Audio instance buffer full.");
	}

	float initial_volume = fade_in_time == 0 ? as->music_volume : 0;
	audio_instance_t* inst_ptr = CUTE_LIST_HOST(audio_instance_t, node, list_pop_back(&as->free_sounds));
	inst_ptr->sound = cs_make_playing_sound(audio_source);
	inst_ptr->sound.paused = 0;
	inst_ptr->sound.looped = 1;
	inst_ptr->sound.volume0 = initial_volume;
	inst_ptr->sound.volume1 = initial_volume;
	int ret = cs_insert_sound(app->cute_sound, &inst_ptr->sound);
	if (!ret) return error_failure("cute_sound.h `cs_insert_sound` failed.");

	CUTE_ASSERT(as->music_playing == NULL);
	as->music_playing = inst_ptr;

	list_push_back(&as->playing_sounds, &inst_ptr->node);

	return error_success();
}

error_t music_stop(app_t* app, float fade_out_time)
{
	audio_system_t* as = app->audio_system;

	if (fade_out_time == 0) {
		// Immediately turn off all music if no fade out time.
		if (as->music_playing) as->music_playing->sound.active = 0;
		if (as->music_next) as->music_next->sound.active = 0;
		as->music_playing = NULL;
		as->music_next = NULL;
		as->music_state = MUSIC_STATE_NONE;
		return error_success();
	} else {
		// Otherwise enter fading out state.
		/*
			If playing, trivial.
			If already fading in any way, simply use that fade time and then swap to fade out.
			Special care to handle crossfade and whatnot.
			Come back to this later.
		*/
		return error_failure("Not yet implemented.");
	}
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

error_t sound_play(app_t* app, audio_t* audio_source, sound_params_t params)
{
	audio_system_t* as = app->audio_system;
	if (!as) return error_failure("Audio system not initialized.");

	if (list_empty(&as->free_sounds)) {
		return error_failure("Unable to play music. Audio instance buffer full.");
	}

	float pan = params.pan;
	if (pan > 1.0f) pan = 1.0f;
	else if (pan < 0.0f) pan = 0.0f;
	float panl = 1.0f - pan;
	float panr = pan;

	audio_instance_t* inst_ptr = CUTE_LIST_HOST(audio_instance_t, node, list_pop_back(&as->free_sounds));
	inst_ptr->sound = cs_make_playing_sound(audio_source);
	inst_ptr->sound.paused = params.paused;
	inst_ptr->sound.looped = params.looped;
	inst_ptr->sound.volume0 = params.volume;
	inst_ptr->sound.volume1 = params.volume;
	inst_ptr->sound.pan0 = panl;
	inst_ptr->sound.pan1 = panr;
	inst_ptr->sound.loaded_sound = audio_source;
	int ret = cs_insert_sound(app->cute_sound, &inst_ptr->sound);
	if (!ret) return error_failure("cute_sound.h `cs_insert_sound` failed.");

	list_push_back(&as->playing_sounds, &inst_ptr->node);

	return error_success();
}

// -------------------------------------------------------------------------------------------------

void audio_set_pan(app_t* app, float pan)
{
	audio_system_t* as = app->audio_system;
	if (!as) return;

	if (list_empty(&as->playing_sounds)) return;
	list_node_t* playing_sound = list_begin(&as->playing_sounds);
	list_node_t* end = list_end(&as->playing_sounds);

	do {
		audio_instance_t* inst = CUTE_LIST_HOST(audio_instance_t, node, playing_sound);
		cs_set_pan(&inst->sound, pan);
		playing_sound = playing_sound->next;
	} while (playing_sound != end);

	if (as->music_playing) cs_set_pan(&as->music_playing->sound, pan);
	if (as->music_next) cs_set_pan(&as->music_next->sound, pan);
}

void audio_set_global_volume(app_t* app, float volume)
{
	audio_system_t* as = app->audio_system;
	if (!as) return;

	as->global_volume = volume;
	float sound_volume = as->sound_volume * volume;
	float music_volume = as->music_volume * volume;

	if (list_empty(&as->playing_sounds)) return;
	list_node_t* playing_sound = list_begin(&as->playing_sounds);
	list_node_t* end = list_end(&as->playing_sounds);

	do {
		audio_instance_t* inst = CUTE_LIST_HOST(audio_instance_t, node, playing_sound);
		cs_set_volume(&inst->sound, sound_volume, sound_volume);
		playing_sound = playing_sound->next;
	} while (playing_sound != end);

	if (as->music_playing) cs_set_volume(&as->music_playing->sound, music_volume, music_volume);
	if (as->music_next) cs_set_volume(&as->music_next->sound, music_volume, music_volume);
}

void audio_set_sound_volume(app_t* app, float volume)
{
	audio_system_t* as = app->audio_system;
	if (!as) return;

	as->sound_volume = volume;
	float sound_volume = as->global_volume * volume;

	if (list_empty(&as->playing_sounds)) return;
	list_node_t* playing_sound = list_begin(&as->playing_sounds);
	list_node_t* end = list_end(&as->playing_sounds);

	do {
		audio_instance_t* inst = CUTE_LIST_HOST(audio_instance_t, node, playing_sound);
		if (inst != as->music_playing && inst != as->music_next) {
			cs_set_volume(&inst->sound, sound_volume, sound_volume);
		}
		playing_sound = playing_sound->next;
	} while (playing_sound != end);
}

// -------------------------------------------------------------------------------------------------
// Internal.

audio_system_t* audio_system_make(int pool_count, void* mem_ctx)
{
	audio_system_t* as = (audio_system_t*)CUTE_ALLOC(sizeof(audio_system_t), mem_ctx);
	CUTE_ASSERT(as);
	CUTE_PLACEMENT_NEW(as) audio_system_t;
	as->playing_sounds_buffer = (audio_instance_t*)CUTE_ALLOC(sizeof(audio_instance_t) * pool_count, mem_ctx);

	list_init(&as->playing_sounds);
	list_init(&as->free_sounds);
	for (int i = 0; i < pool_count; ++i) {
		audio_instance_t* inst = as->playing_sounds_buffer + i;
		list_init_node(&inst->node);
		list_push_back(&as->free_sounds, &inst->node);
	}

	as->mem_ctx = mem_ctx;
	return as;
}

void audio_system_destroy(audio_system_t* audio_system)
{
	CUTE_FREE(audio_system->playing_sounds_buffer, audio_system->mem_ctx);
	CUTE_FREE(audio_system, audio_system->mem_ctx);
}

void audio_system_update(audio_system_t* as, float dt)
{
	// Move any instances that finished playing to the free list.
	// Don't gargbage collect the music instances though.
	if (list_empty(&as->playing_sounds)) return;
	list_node_t* playing_sound = list_begin(&as->playing_sounds);
	list_node_t* end = list_end(&as->playing_sounds);

	do {
		audio_instance_t* inst = CUTE_LIST_HOST(audio_instance_t, node, playing_sound);
		list_node_t* next = playing_sound->next;
		if (inst != as->music_playing && inst != as->music_next) {
			if (!inst->sound.active) {
				list_remove(&inst->node);
				list_push_back(&as->free_sounds, &inst->node);
			}
		}
		playing_sound = next;
	} while (playing_sound != end);

	// Update fades and crossfades.
}

int sound_instance_size()
{
	return sizeof(sound_t);
}

}

#undef STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>
