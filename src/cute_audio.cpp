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
#include <cute_math.h>
#include <cute_file_system.h>

#include <internal/cute_app_internal.h>

#define STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>

#define CUTE_SOUND_IMPLEMENTATION
#define CUTE_SOUND_FORCE_SDL
#ifdef CUTE_EMSCRIPTEN
#	define CUTE_SOUND_SCALAR_MODE
#endif
#include <cute/cute_sound.h>

// TODO - Use cute file system to access disk
// TODO: Hookup allocs to cute_sound internals

//#include <cute_debug_printf.h>

struct cf_audio_t : public cs_loaded_sound_t
{
	cf_audio_t() { }
	cf_audio_t(const cs_loaded_sound_t& sound) { *((cs_loaded_sound_t*)this) = sound; }

	cf_promise_t user_promise;
	cf_promise_t play_promise;
	void* mem_ctx = NULL;
};

using cs_sound_t = cs_playing_sound_t;

static CUTE_INLINE cf_audio_t* cf_s_audio_make(cf_audio_t audio_struct, void* user_allocator_context)
{
	if (audio_struct.channel_count) {
		cf_audio_t* audio = (cf_audio_t*)CUTE_ALLOC(sizeof(cf_audio_t), user_allocator_context);
		CUTE_PLACEMENT_NEW(audio) cf_audio_t;
		*audio = audio_struct;
		return audio;
	} else {
		return NULL;
	}
}

cf_audio_t* cf_audio_load_ogg(const char* path, void* user_allocator_context)
{
	void* data;
	size_t sz;
	cf_file_system_read_entire_file_to_memory(path, &data, &sz, NULL);
	cf_audio_t audio_struct;
	cs_read_mem_ogg(data, (int)sz, &audio_struct);
	CUTE_FREE(data, NULL);
	return cf_s_audio_make(audio_struct, user_allocator_context);
}

cf_audio_t* cf_audio_load_wav(const char* path, void* user_allocator_context)
{
	void* data;
	size_t sz;
	cf_file_system_read_entire_file_to_memory(path, &data, &sz, NULL);
	cf_audio_t audio_struct;
	cs_read_mem_wav(data, (int)sz, &audio_struct);
	CUTE_FREE(data, NULL);
	return cf_s_audio_make(audio_struct, user_allocator_context);
}

cf_audio_t* cf_audio_load_ogg_from_memory(void* memory, int byte_count, void* user_allocator_context)
{
	cf_audio_t audio_struct;
	cs_read_mem_ogg(memory, byte_count, &audio_struct);
	return cf_s_audio_make(audio_struct, user_allocator_context);
}

cf_audio_t* cf_audio_load_wav_from_memory(void* memory, int byte_count, void* user_allocator_context)
{
	cf_audio_t audio_struct;
	cs_read_mem_wav(memory, byte_count, &audio_struct);
	return cf_s_audio_make(audio_struct, user_allocator_context);
}

struct cf_audio_param_t
{
	const char* path = NULL;
	void* memory = NULL;
	int byte_count = 0;
	cf_promise_t user_promise;
	void* mem_ctx = NULL;
};

static void cf_s_stream_ogg_task_fn(void* param)
{
	cf_audio_param_t* audio_param = (cf_audio_param_t*)param;
	cf_audio_t* audio = NULL;
	if (audio_param->path) {
		audio = cf_audio_load_ogg(audio_param->path, audio_param->mem_ctx);
	} else {
		audio = cf_audio_load_ogg_from_memory(audio_param->memory, audio_param->byte_count, audio_param->mem_ctx);
	}
	audio_param->user_promise.invoke(audio ? cf_error_success() : cf_error_failure("Failed to load ogg file."), audio);
	CUTE_FREE(audio_param, audio_param->mem_ctx);
}

static void cf_s_stream_wav_task_fn(void* param)
{
	cf_audio_param_t* audio_param = (cf_audio_param_t*)param;
	cf_audio_t* audio = NULL;
	if (audio_param->path) {
		audio = cf_audio_load_wav(audio_param->path, audio_param->mem_ctx);
	} else {
		audio = cf_audio_load_wav_from_memory(audio_param->memory, audio_param->byte_count, audio_param->mem_ctx);
	}
	audio_param->user_promise.invoke(audio ? cf_error_success() : cf_error_failure("Failed to load wav file."), audio);
	CUTE_FREE(audio_param, audio_param->mem_ctx);
}

void cf_audio_stream_ogg(const char* path, cf_promise_t promise, void* user_allocator_context)
{
	cf_audio_param_t* param = (cf_audio_param_t*)CUTE_ALLOC(sizeof(cf_audio_param_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(param) cf_audio_param_t;
	param->path = path;
	param->user_promise = promise;
	param->mem_ctx = user_allocator_context;

	cf_threadpool_add_task(cf_app->threadpool, cf_s_stream_ogg_task_fn, param);
	cute_threadpool_kick(cf_app->threadpool);
}

void cf_audio_stream_wav(const char* path, cf_promise_t promise, void* user_allocator_context)
{
	cf_audio_param_t* param = (cf_audio_param_t*)CUTE_ALLOC(sizeof(cf_audio_param_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(param) cf_audio_param_t;
	param->path = path;
	param->user_promise = promise;
	param->mem_ctx = user_allocator_context;

	cf_threadpool_add_task(cf_app->threadpool, cf_s_stream_wav_task_fn, param);
	cute_threadpool_kick(cf_app->threadpool);
}

void cf_audio_stream_ogg_from_memory(void* memory, int byte_count, cf_promise_t promise, void* user_allocator_context)
{
	cf_audio_param_t* param = (cf_audio_param_t*)CUTE_ALLOC(sizeof(cf_audio_param_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(param) cf_audio_param_t;
	param->memory = memory;
	param->byte_count = byte_count;
	param->user_promise = promise;
	param->mem_ctx = user_allocator_context;

	cf_threadpool_add_task(cf_app->threadpool, cf_s_stream_ogg_task_fn, param);
	cute_threadpool_kick(cf_app->threadpool);
}

void cf_audio_stream_wav_from_memory(void* memory, int byte_count, cf_promise_t promise, void* user_allocator_context)
{
	cf_audio_param_t* param = (cf_audio_param_t*)CUTE_ALLOC(sizeof(cf_audio_param_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(param) cf_audio_param_t;
	param->memory = memory;
	param->byte_count = byte_count;
	param->user_promise = promise;
	param->mem_ctx = user_allocator_context;

	cf_threadpool_add_task(cf_app->threadpool, cf_s_stream_wav_task_fn, param);
	cute_threadpool_kick(cf_app->threadpool);
}

cf_error_t cf_audio_destroy(cf_audio_t* audio)
{
	if (cf_audio_ref_count(audio) == 0) {
		void* mem_ctx = audio->mem_ctx;
		cs_free_sound(audio);
		CUTE_FREE(audio, mem_ctx);
		return cf_error_success();
	} else {
		return cf_error_failure("Reference count for audio was not zero.");
	}
}

int cf_audio_ref_count(cf_audio_t* audio)
{
	return audio->playing_count;
}

// -------------------------------------------------------------------------------------------------

struct cf_audio_instance_t
{
	uint64_t id;
	cs_playing_sound_t sound;
	cf_list_node_t node;
};

enum cf_music_state_t
{
	CF_MUSIC_STATE_NONE,
	CF_MUSIC_STATE_PLAYING,
	CF_MUSIC_STATE_FADE_OUT,
	CF_MUSIC_STATE_FADE_IN,
	CF_MUSIC_STATE_SWITCH_TO_0,
	CF_MUSIC_STATE_SWITCH_TO_1,
	CF_MUSIC_STATE_CROSSFADE,
	CF_MUSIC_STATE_PAUSED
};

struct cf_audio_system_t
{
	float global_volume = 1.0f;
	float music_volume = 1.0f;
	float sound_volume = 1.0f;

	float t = 0;
	float fade = 0;
	float fade_switch_1 = 0;
	cf_music_state_t music_state = CF_MUSIC_STATE_NONE;
	cf_music_state_t music_state_to_resume_from_paused = CF_MUSIC_STATE_NONE;
	cf_audio_instance_t* music_playing = NULL;
	cf_audio_instance_t* music_next = NULL;

	uint64_t instance_id_gen = 1;
	cf_dictionary<uint64_t, cf_audio_instance_t*> instance_map;
	cf_audio_instance_t* playing_sounds_buffer;
	cf_list_t playing_sounds;
	cf_list_t free_sounds;
	void* mem_ctx = NULL;
};

static cf_audio_instance_t* cf_s_inst(cf_audio_t* src, float volume)
{
	cf_audio_system_t* as = cf_app->audio_system;
	cf_audio_instance_t* inst_ptr = CUTE_LIST_HOST(cf_audio_instance_t, node, cf_list_pop_back(&as->free_sounds));
	inst_ptr->sound = cs_make_playing_sound(src);
	inst_ptr->sound.paused = 1;
	inst_ptr->sound.looped = 1;
	inst_ptr->sound.volume0 = volume;
	inst_ptr->sound.volume1 = volume;
	cs_insert_sound(cf_app->cute_sound, &inst_ptr->sound);
	return inst_ptr;
}

static cf_audio_instance_t* cf_s_inst(cf_audio_t* src, cf_sound_params_t params)
{
	cf_audio_system_t* as = cf_app->audio_system;
	cf_audio_instance_t* inst_ptr = CUTE_LIST_HOST(cf_audio_instance_t, node, cf_list_pop_back(&as->free_sounds));
	float pan = params.pan;
	if (pan > 1.0f) pan = 1.0f;
	else if (pan < 0.0f) pan = 0.0f;
	float panl = 1.0f - pan;
	float panr = pan;
	inst_ptr->sound = cs_make_playing_sound(src);
	inst_ptr->sound.paused = params.paused;
	inst_ptr->sound.looped = params.looped;
	inst_ptr->sound.volume0 = params.volume;
	inst_ptr->sound.volume1 = params.volume;
	inst_ptr->sound.pan0 = panl;
	inst_ptr->sound.pan1 = panr;
	cs_insert_sound(cf_app->cute_sound, &inst_ptr->sound);
	inst_ptr->id = as->instance_id_gen++;
	as->instance_map.insert(inst_ptr->id, inst_ptr);
	return inst_ptr;
}

cf_error_t cf_music_play(cf_audio_t* audio_source, float fade_in_time)
{
	cf_audio_system_t* as = cf_app->audio_system;
	if (!as) return cf_error_failure("Audio system not initialized.");

	if (as->music_state != CF_MUSIC_STATE_PLAYING) {
		cf_error_t err = cf_music_stop(0);
		if (err.is_error()) return err;
	}

	if (fade_in_time < 0) fade_in_time = 0;
	if (fade_in_time) {
		as->music_state = CF_MUSIC_STATE_FADE_IN;
		CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_FADE_IN\n");
	} else {
		as->music_state = CF_MUSIC_STATE_PLAYING;
		CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_PLAYING\n");
	}
	as->fade = fade_in_time;
	as->t = 0;

	if (cf_list_empty(&as->free_sounds)) {
		return cf_error_failure("Unable to play music. Audio instance buffer full.");
	}

	float initial_volume = fade_in_time == 0 ? as->music_volume : 0;
	CUTE_ASSERT(as->music_playing == NULL);
	CUTE_ASSERT(as->music_next == NULL);
	cf_audio_instance_t* inst = cf_s_inst(audio_source, initial_volume);
	inst->sound.paused = 0;
	as->music_playing = inst;
	cf_list_push_back(&as->playing_sounds, &inst->node);

	return cf_error_success();
}

cf_error_t cf_music_stop(float fade_out_time)
{
	cf_audio_system_t* as = cf_app->audio_system;
	if (!as) return cf_error_failure("Audio system not initialized.");

	if (fade_out_time < 0) fade_out_time = 0;

	if (fade_out_time == 0) {
		// Immediately turn off all music if no fade out time.
		if (as->music_playing) as->music_playing->sound.active = 0;
		if (as->music_next) as->music_next->sound.active = 0;
		as->music_playing = NULL;
		as->music_next = NULL;
		as->music_state = CF_MUSIC_STATE_NONE;
		CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_NONE\n");
		return cf_error_success();
	} else {
		switch (as->music_state) {
		case CF_MUSIC_STATE_NONE:
			break;

		case CF_MUSIC_STATE_PLAYING:
			as->music_state = CF_MUSIC_STATE_FADE_OUT;
			CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_FADE_OUT\n");
			as->fade = fade_out_time;
			as->t = 0;
			break;

		case CF_MUSIC_STATE_FADE_OUT:
			return cf_error_success();

		case CF_MUSIC_STATE_FADE_IN:
		{
			as->music_state = CF_MUSIC_STATE_FADE_OUT;
			CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_FADE_OUT\n");
			as->t = cf_smoothstep(((as->fade - as->t) / as->fade));
			as->fade = fade_out_time;
		}	break;

		case CF_MUSIC_STATE_SWITCH_TO_0:
		{
			as->music_state = CF_MUSIC_STATE_FADE_OUT;
			CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_FADE_OUT\n");
			as->t = cf_smoothstep(((as->fade - as->t) / as->fade));
			as->fade = fade_out_time;
			as->music_next = NULL;
		}	break;

		case CF_MUSIC_STATE_SWITCH_TO_1:
			// Fall-through.

		case CF_MUSIC_STATE_CROSSFADE:
		{
			as->music_state = CF_MUSIC_STATE_FADE_OUT;
			CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_FADE_OUT\n");
			as->t = cf_smoothstep(((as->fade - as->t) / as->fade));
			as->fade = fade_out_time;
			as->music_playing = as->music_next;
			as->music_next = NULL;
		}	break;

		case CF_MUSIC_STATE_PAUSED:
			return cf_error_failure("Can not start a fade out while the music is paused.");
		}

		return cf_error_success();
	}
}

void cf_music_set_volume(float volume)
{
	cf_audio_system_t* as = cf_app->audio_system;
	if (!as) return;

	as->music_volume = volume;
	float music_volume = as->global_volume * volume;

	if (as->music_playing) cs_set_volume(&as->music_playing->sound, music_volume, music_volume);
	if (as->music_next) cs_set_volume(&as->music_next->sound, music_volume, music_volume);
}

void cf_music_set_pitch(float pitch)
{
	// Todo.
}

void cf_music_set_loop(bool true_to_loop)
{
	cf_audio_system_t* as = cf_app->audio_system;
	if (!as) return;
	if (as->music_playing) as->music_playing->sound.looped = true_to_loop ? 1 : 0;
	if (as->music_next) as->music_next->sound.looped = true_to_loop ? 1 : 0;
}

void cf_music_pause()
{
	cf_audio_system_t* as = cf_app->audio_system;
	if (!as) return;
	if (as->music_state == CF_MUSIC_STATE_PAUSED) return;
	if (as->music_playing) as->music_playing->sound.paused = 1;
	if (as->music_next) as->music_next->sound.paused = 1;
	as->music_state_to_resume_from_paused = as->music_state;
	as->music_state = CF_MUSIC_STATE_PAUSED;
	CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_PAUSED\n");
}

void cf_music_resume()
{
	cf_audio_system_t* as = cf_app->audio_system;
	if (!as) return;
	if (as->music_state != CF_MUSIC_STATE_PAUSED) return;
	if (as->music_playing) as->music_playing->sound.paused = 0;
	if (as->music_next) as->music_next->sound.paused = 0;
	as->music_state = as->music_state_to_resume_from_paused;
	CUTE_DEBUG_PRINTF("music_state_to_resume_from_paused\n");
}

cf_error_t cf_music_switch_to(cf_audio_t* audio_source, float fade_out_time, float fade_in_time)
{
	cf_audio_system_t* as = cf_app->audio_system;
	if (!as) return cf_error_failure("Audio system not initialized.");

	if (fade_in_time < 0) fade_in_time = 0;
	if (fade_out_time < 0) fade_out_time = 0;

	switch (as->music_state) {
	case CF_MUSIC_STATE_NONE:
		return cf_music_play(audio_source, fade_in_time);

	case CF_MUSIC_STATE_PLAYING:
	{
		CUTE_ASSERT(as->music_next == NULL);
		cf_audio_instance_t* inst = cf_s_inst(audio_source, fade_in_time == 0 ? as->music_volume : 0);
		as->music_next = inst;
		cf_list_push_back(&as->playing_sounds, &inst->node);

		as->fade = fade_out_time;
		as->fade_switch_1 = fade_in_time;
		as->t = 0;
		as->music_state = CF_MUSIC_STATE_SWITCH_TO_0;
		CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_SWITCH_TO_0\n");
	}	break;

	case CF_MUSIC_STATE_FADE_OUT:
	{
		CUTE_ASSERT(as->music_next == NULL);
		cf_audio_instance_t* inst = cf_s_inst(audio_source, fade_in_time == 0 ? as->music_volume : 0);
		as->music_next = inst;
		cf_list_push_back(&as->playing_sounds, &inst->node);

		as->fade_switch_1 = fade_in_time;
		as->music_state = CF_MUSIC_STATE_SWITCH_TO_0;
		CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_SWITCH_TO_0\n");
	}	break;

	case CF_MUSIC_STATE_FADE_IN:
	{
		CUTE_ASSERT(as->music_next == NULL);
		cf_audio_instance_t* inst = cf_s_inst(audio_source, fade_in_time == 0 ? as->music_volume : 0);
		as->music_next = inst;
		cf_list_push_back(&as->playing_sounds, &inst->node);

		as->fade_switch_1 = fade_in_time;
		as->t = cf_smoothstep(((as->fade - as->t) / as->fade));
		as->music_state = CF_MUSIC_STATE_SWITCH_TO_0;
		CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_SWITCH_TO_0\n");
	}	break;

	case CF_MUSIC_STATE_SWITCH_TO_0:
	{
		CUTE_ASSERT(as->music_next != NULL);
		cf_audio_instance_t* inst = cf_s_inst(audio_source, fade_in_time == 0 ? as->music_volume : 0);
		as->music_next->sound.active = 0;
		as->music_next = inst;
		as->fade_switch_1 = fade_in_time;
		cf_list_push_back(&as->playing_sounds, &inst->node);
	}	break;

	case CF_MUSIC_STATE_CROSSFADE:
	case CF_MUSIC_STATE_SWITCH_TO_1:
	{
		CUTE_ASSERT(as->music_next != NULL);
		cf_audio_instance_t* inst = cf_s_inst(audio_source, fade_in_time == 0 ? as->music_volume : 0);
		as->music_playing = as->music_next;
		as->music_next = inst;
		cf_list_push_back(&as->playing_sounds, &inst->node);

		as->t = cf_smoothstep(((as->fade - as->t) / as->fade));
		as->fade_switch_1 = fade_in_time;
		as->fade = fade_out_time;
		as->music_state = CF_MUSIC_STATE_SWITCH_TO_0;
		CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_SWITCH_TO_0\n");
	}	break;

	case CF_MUSIC_STATE_PAUSED:
		return cf_error_failure("Can not switch music while paused.");
	}

	return cf_error_success();
}

cf_error_t cf_music_crossfade(cf_audio_t* audio_source, float cross_fade_time)
{
	cf_audio_system_t* as = cf_app->audio_system;
	if (!as) return cf_error_failure("Audio system not initialized.");

	if (cross_fade_time < 0) cross_fade_time = 0;

	switch (as->music_state) {
	case CF_MUSIC_STATE_NONE:
		return cf_music_play(audio_source, cross_fade_time);

	case CF_MUSIC_STATE_PLAYING:
	{
		CUTE_ASSERT(as->music_next == NULL);
		as->music_state = CF_MUSIC_STATE_CROSSFADE;
		CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_CROSSFADE\n");

		cf_audio_instance_t* inst = cf_s_inst(audio_source, cross_fade_time == 0 ? as->music_volume : 0);
		inst->sound.paused = 0;
		as->music_next = inst;
		cf_list_push_back(&as->playing_sounds, &inst->node);

		as->fade = cross_fade_time;
		as->t = 0;
	}	break;

	case CF_MUSIC_STATE_FADE_OUT:
		CUTE_ASSERT(as->music_next == NULL);
		// Fall-through.

	case CF_MUSIC_STATE_FADE_IN:
	{
		as->music_state = CF_MUSIC_STATE_CROSSFADE;
		CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_CROSSFADE\n");

		cf_audio_instance_t* inst = cf_s_inst(audio_source, cross_fade_time == 0 ? as->music_volume : 0);
		inst->sound.paused = 0;
		as->music_next = inst;
		cf_list_push_back(&as->playing_sounds, &inst->node);

		as->fade = cross_fade_time;
	}	break;

	case CF_MUSIC_STATE_SWITCH_TO_0:
	{
		as->music_state = CF_MUSIC_STATE_CROSSFADE;
		CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_CROSSFADE\n");
		as->music_next->sound.active = 0;

		cf_audio_instance_t* inst = cf_s_inst(audio_source, cross_fade_time == 0 ? as->music_volume : 0);
		inst->sound.paused = 0;
		as->music_next = inst;
		cf_list_push_back(&as->playing_sounds, &inst->node);

		as->fade = cross_fade_time;
	}	break;

	case CF_MUSIC_STATE_SWITCH_TO_1:
		// Fall-through.

	case CF_MUSIC_STATE_CROSSFADE:
	{
		as->music_state = CF_MUSIC_STATE_CROSSFADE;
		CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_CROSSFADE\n");
		as->music_playing->sound.active = 0;
		as->music_playing = as->music_next;

		cf_audio_instance_t* inst = cf_s_inst(audio_source, cross_fade_time == 0 ? as->music_volume : 0);
		inst->sound.paused = 0;
		as->music_next = inst;
		cf_list_push_back(&as->playing_sounds, &inst->node);

		as->fade = cross_fade_time;
	}	break;

	case CF_MUSIC_STATE_PAUSED:
		return cf_error_failure("Can not start a crossfade while music is paused.");
	}

	return cf_error_success();
}

// -------------------------------------------------------------------------------------------------

cf_sound_params_t cf_sound_params_defaults()
{
	cf_sound_params_t params = {};
	params.volume = 1.0f;
	params.pan = 0.5f;
	params.pitch = 1.0f;
	return params;
}

cf_sound_t cf_sound_play(cf_audio_t* audio_source, cf_sound_params_t params, cf_error_t* err)
{
	cf_audio_system_t* as = cf_app->audio_system;
	if (!as) {
		if (err) *err = cf_error_failure("Audio system not initialized.");
		return cf_sound_t();
	}

	if (cf_list_empty(&as->free_sounds)) {
		if (err) *err = cf_error_failure("Unable to play music. Audio instance buffer full.");
		return cf_sound_t();
	}

	cf_audio_instance_t* inst = cf_s_inst(audio_source, params);
	cf_list_push_back(&as->playing_sounds, &inst->node);

	if (err) *err = cf_error_success();
	cf_sound_t sound;
	sound.id = inst->id;
	return sound;
}

static cf_audio_instance_t* cf_s_get_inst(cf_sound_t sound)
{
	cf_audio_system_t* as = cf_app->audio_system;
	CUTE_ASSERT(as);
	cf_audio_instance_t* inst;
	if (as->instance_map.find(sound.id, &inst).is_error()) {
		return inst;
	} else {
		return NULL;
	}
}

bool cf_sound_is_active(cf_sound_t sound)
{
	return cf_s_get_inst(sound) ? true : false;
}

bool cf_sound_get_is_paused(cf_sound_t sound)
{
	cf_audio_instance_t* inst = cf_s_get_inst(sound);
	if (inst) {
		cs_lock(cf_app->cute_sound);
		bool paused = inst->sound.paused;
		cs_unlock(cf_app->cute_sound);
		return paused;
	} else return 0;
}

bool cf_sound_get_is_looped(cf_sound_t sound)
{
	cf_audio_instance_t* inst = cf_s_get_inst(sound);
	if (inst) {
		cs_lock(cf_app->cute_sound);
		bool looped = inst->sound.looped;
		cs_unlock(cf_app->cute_sound);
		return looped;
	} else return 0;
}

float cf_sound_get_volume(cf_sound_t sound)
{
	cf_audio_instance_t* inst = cf_s_get_inst(sound);
	if (inst) {
		cs_lock(cf_app->cute_sound);
		float volume = inst->sound.volume0;
		cs_unlock(cf_app->cute_sound);
		return volume;
	} else return 0;
}

int cf_sound_get_sample_index(cf_sound_t sound)
{
	cf_audio_instance_t* inst = cf_s_get_inst(sound);
	if (inst) {
		cs_lock(cf_app->cute_sound);
		int index = inst->sound.sample_index;
		cs_unlock(cf_app->cute_sound);
		return index;
	} else return 0;
}

void cf_sound_set_is_paused(cf_sound_t sound, bool true_for_paused)
{
	cf_audio_instance_t* inst = cf_s_get_inst(sound);
	if (inst) {
		cs_lock(cf_app->cute_sound);
		inst->sound.paused = true_for_paused;
		cs_unlock(cf_app->cute_sound);
	}
}

void cf_sound_set_is_looped(cf_sound_t sound, bool true_for_looped)
{
	cf_audio_instance_t* inst = cf_s_get_inst(sound);
	if (inst) {
		cs_lock(cf_app->cute_sound);
		inst->sound.looped = true_for_looped;
		cs_unlock(cf_app->cute_sound);
	}
}

void cf_sound_set_volume(cf_sound_t sound, float volume)
{
	cf_audio_instance_t* inst = cf_s_get_inst(sound);
	if (inst) {
		cs_lock(cf_app->cute_sound);
		inst->sound.volume0 = volume;
		inst->sound.volume1 = volume;
		cs_unlock(cf_app->cute_sound);
	}
}

void cf_sound_set_sample_index(cf_sound_t sound, int sample_index)
{
	cf_audio_instance_t* inst = cf_s_get_inst(sound);
	if (inst) {
		cs_lock(cf_app->cute_sound);
		inst->sound.sample_index = sample_index;
		cs_unlock(cf_app->cute_sound);
	}
}

// -------------------------------------------------------------------------------------------------

void cf_audio_set_pan(float pan)
{
	cf_audio_system_t* as = cf_app->audio_system;
	if (!as) return;

	if (cf_list_empty(&as->playing_sounds)) return;
	cf_list_node_t* playing_sound = cf_list_begin(&as->playing_sounds);
	cf_list_node_t* end = cf_list_end(&as->playing_sounds);

	do {
		cf_audio_instance_t* inst = CUTE_LIST_HOST(cf_audio_instance_t, node, playing_sound);
		cs_set_pan(&inst->sound, pan);
		playing_sound = playing_sound->next;
	} while (playing_sound != end);

	if (as->music_playing) cs_set_pan(&as->music_playing->sound, pan);
	if (as->music_next) cs_set_pan(&as->music_next->sound, pan);
}

void cf_audio_set_global_volume(float volume)
{
	cf_audio_system_t* as = cf_app->audio_system;
	if (!as) return;

	// TODO -- Optimizaton.
	// Can expose global volume in cute_sound.h and simply pass it along, so this can be
	// done within the mixer, instead of touching every single instance.

	as->global_volume = volume;
	float sound_volume = as->sound_volume * volume;
	float music_volume = as->music_volume * volume;

	if (cf_list_empty(&as->playing_sounds)) return;
	cf_list_node_t* playing_sound = cf_list_begin(&as->playing_sounds);
	cf_list_node_t* end = cf_list_end(&as->playing_sounds);

	do {
		cf_audio_instance_t* inst = CUTE_LIST_HOST(cf_audio_instance_t, node, playing_sound);
		cs_set_volume(&inst->sound, sound_volume, sound_volume);
		playing_sound = playing_sound->next;
	} while (playing_sound != end);

	if (as->music_playing) cs_set_volume(&as->music_playing->sound, music_volume, music_volume);
	if (as->music_next) cs_set_volume(&as->music_next->sound, music_volume, music_volume);
}

void cf_audio_set_sound_volume(float volume)
{
	cf_audio_system_t* as = cf_app->audio_system;
	if (!as) return;

	as->sound_volume = volume;
	float sound_volume = as->global_volume * volume;

	if (cf_list_empty(&as->playing_sounds)) return;
	cf_list_node_t* playing_sound = cf_list_begin(&as->playing_sounds);
	cf_list_node_t* end = cf_list_end(&as->playing_sounds);

	do {
		cf_audio_instance_t* inst = CUTE_LIST_HOST(cf_audio_instance_t, node, playing_sound);
		if (inst != as->music_playing && inst != as->music_next) {
			cs_set_volume(&inst->sound, sound_volume, sound_volume);
		}
		playing_sound = playing_sound->next;
	} while (playing_sound != end);
}

void cf_audio_set_pause(bool true_for_paused)
{
	// TODO -- Expose a global paused variable in cute sound context.
}

// -------------------------------------------------------------------------------------------------
// Internal.

cf_audio_system_t* cf_audio_system_make(int pool_count, void* mem_ctx)
{
	cf_audio_system_t* as = (cf_audio_system_t*)CUTE_ALLOC(sizeof(cf_audio_system_t), mem_ctx);
	CUTE_ASSERT(as);
	CUTE_PLACEMENT_NEW(as) cf_audio_system_t;
	as->playing_sounds_buffer = (cf_audio_instance_t*)CUTE_ALLOC(sizeof(cf_audio_instance_t) * pool_count, mem_ctx);

	cf_list_init(&as->playing_sounds);
	cf_list_init(&as->free_sounds);
	for (int i = 0; i < pool_count; ++i) {
		cf_audio_instance_t* inst = as->playing_sounds_buffer + i;
		cf_list_init_node(&inst->node);
		cf_list_push_back(&as->free_sounds, &inst->node);
	}

	as->mem_ctx = mem_ctx;

	CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_NONE\n");

	return as;
}

void cf_audio_system_destroy(cf_audio_system_t* audio_system)
{
	if (audio_system) {
		CUTE_FREE(audio_system->playing_sounds_buffer, audio_system->mem_ctx);
		audio_system->~cf_audio_system_t();
		CUTE_FREE(audio_system, audio_system->mem_ctx);
	}
}

void cf_audio_system_update(cf_audio_system_t* as, float dt)
{
	// Move any instances that finished playing to the free list.
	// Don't gargbage collect the music instances though.
	if (cf_list_empty(&as->playing_sounds)) return;
	cf_list_node_t* playing_sound = cf_list_begin(&as->playing_sounds);
	cf_list_node_t* end = cf_list_end(&as->playing_sounds);

	do {
		cf_audio_instance_t* inst = CUTE_LIST_HOST(cf_audio_instance_t, node, playing_sound);
		cf_list_node_t* next = playing_sound->next;
		if (inst != as->music_playing && inst != as->music_next) {
			if (!inst->sound.active) {
				as->instance_map.remove(inst->id);
				cf_list_remove(&inst->node);
				cf_list_push_back(&as->free_sounds, &inst->node);
			}
		}
		playing_sound = next;
	} while (playing_sound != end);

	switch (as->music_state) {
	case CF_MUSIC_STATE_FADE_OUT:
	{
		as->t += dt;
		if (as->t >= as->fade) {
			as->music_state = CF_MUSIC_STATE_NONE;
			CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_NONE\n");
			as->music_playing->sound.active = 0;
			as->music_playing = NULL;
		} else {
			float t = cf_smoothstep(((as->fade - as->t) / as->fade));
			float volume = as->music_volume * as->global_volume * t;
			cs_set_volume(&as->music_playing->sound, volume, volume);
		}
	}	break;

	case CF_MUSIC_STATE_FADE_IN:
	{
		as->t += dt;
		if (as->t >= as->fade) {
			as->music_state = CF_MUSIC_STATE_PLAYING;
			CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_PLAYING\n");
			as->t = as->fade;
		}
		float t = cf_smoothstep(1.0f - ((as->fade - as->t) / as->fade));
		float volume = as->music_volume * as->global_volume * t;
		cs_set_volume(&as->music_playing->sound, volume, volume);
	}	break;

	case CF_MUSIC_STATE_SWITCH_TO_0:
	{
		as->t += dt;
		if (as->t >= as->fade) {
			as->music_state = CF_MUSIC_STATE_SWITCH_TO_1;
			CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_SWITCH_TO_1\n");
			as->music_playing->sound.active = 0;
			cs_set_volume(&as->music_playing->sound, 0, 0);
			as->t = 0;
			as->fade = as->fade_switch_1;
			as->fade_switch_1 = 0;
			as->music_next->sound.paused = 0;
		} else {
			float t = cf_smoothstep(((as->fade - as->t) / as->fade));
			float volume = as->music_volume * as->global_volume * t;
			cs_set_volume(&as->music_playing->sound, volume, volume);
		}
	}	break;

	case CF_MUSIC_STATE_SWITCH_TO_1:
	{
		as->t += dt;
		if (as->t >= as->fade) {
			as->music_state = CF_MUSIC_STATE_PLAYING;
			CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_PLAYING\n");
			as->t = as->fade;
			float volume = as->music_volume * as->global_volume;
			cs_set_volume(&as->music_next->sound, volume, volume);
			as->music_playing = as->music_next;
			as->music_next = NULL;
		} else {
			float t = cf_smoothstep(1.0f - ((as->fade - as->t) / as->fade));
			float volume = as->music_volume * as->global_volume * t;
			cs_set_volume(&as->music_next->sound, volume, volume);
		}
	}	break;

	case CF_MUSIC_STATE_CROSSFADE:
	{
		as->t += dt;
		if (as->t >= as->fade) {
			as->music_state = CF_MUSIC_STATE_PLAYING;
			CUTE_DEBUG_PRINTF("CF_MUSIC_STATE_PLAYING\n");
			float volume = as->music_volume * as->global_volume;
			cs_set_volume(&as->music_next->sound, volume, volume);
			as->music_playing = as->music_next;
			as->music_next = NULL;
		} else {
			float t0 = cf_smoothstep(((as->fade - as->t) / as->fade));
			float t1 = cf_smoothstep(1.0f - ((as->fade - as->t) / as->fade));
			float v0 = as->music_volume * as->global_volume * t0;
			float v1 = as->music_volume * as->global_volume * t1;
			cs_set_volume(&as->music_playing->sound, v0, v0);
			cs_set_volume(&as->music_next->sound, v1, v1);
		}
	}	break;

	default:
		break;
	}
}

int cf_sound_instance_size()
{
	return sizeof(cs_sound_t);
}

#undef STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>
