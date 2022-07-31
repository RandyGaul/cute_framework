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

#ifndef CUTE_AUDIO_H
#define CUTE_AUDIO_H

#include "cute_defines.h"
#include "cute_concurrency.h"
#include "cute_error.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct cf_audio_t cf_audio_t;
typedef struct cf_app_t cf_app_t;

CUTE_API cf_audio_t* CUTE_CALL cf_audio_load_ogg(const char* path, void* user_allocator_context /*= NULL*/);
CUTE_API cf_audio_t* CUTE_CALL cf_audio_load_wav(const char* path, void* user_allocator_context /*= NULL*/);
CUTE_API cf_audio_t* CUTE_CALL cf_audio_load_ogg_from_memory(void* memory, int byte_count, void* user_allocator_context /*= NULL*/);
CUTE_API cf_audio_t* CUTE_CALL cf_audio_load_wav_from_memory(void* memory, int byte_count, void* user_allocator_context /*= NULL*/);
CUTE_API void CUTE_CALL cf_audio_stream_ogg(const char* path, cf_promise_t promise, void* user_allocator_context /*= NULL*/);
CUTE_API void CUTE_CALL cf_audio_stream_wav(const char* path, cf_promise_t promise, void* user_allocator_context /*= NULL*/);
CUTE_API void CUTE_CALL cf_audio_stream_ogg_from_memory(void* memory, int byte_count, cf_promise_t promise, void* user_allocator_context /*= NULL*/);
CUTE_API void CUTE_CALL cf_audio_stream_wav_from_memory(void* memory, int byte_count, cf_promise_t promise, void* user_allocator_context /*= NULL*/);
CUTE_API cf_error_t CUTE_CALL cf_audio_destroy(cf_audio_t* audio);
CUTE_API int CUTE_CALL cf_audio_ref_count(cf_audio_t* audio);

// -------------------------------------------------------------------------------------------------

CUTE_API cf_error_t CUTE_CALL cf_music_play(cf_audio_t* audio_source, float fade_in_time /*= 0*/);
CUTE_API cf_error_t CUTE_CALL cf_music_stop(float fade_out_time /*= 0*/);
CUTE_API void CUTE_CALL cf_music_set_volume(float volume);
CUTE_API void CUTE_CALL cf_music_set_pitch(float pitch);
CUTE_API void CUTE_CALL cf_music_set_loop(bool true_to_loop);
CUTE_API void CUTE_CALL cf_music_pause(cf_app_t* app);
CUTE_API void CUTE_CALL cf_music_resume(cf_app_t* app);
CUTE_API cf_error_t CUTE_CALL cf_music_switch_to(cf_audio_t* audio_source, float fade_out_time /*= 0*/, float fade_in_time /*= 0*/);
CUTE_API cf_error_t CUTE_CALL cf_music_crossfade(cf_audio_t* audio_source, float cross_fade_time /*= 0*/);

// -------------------------------------------------------------------------------------------------

typedef struct cf_sound_params_t
{
	bool paused; /*= false;*/
	bool looped; /*= false;*/
	float volume; /*= 1.0f;*/
	float pan; /*= 0.5f;*/
	float pitch; /*= 1.0f;*/
	float delay; /*= 0;*/
} cf_sound_params_t;

typedef struct cf_sound_t { uint64_t id; /*= 0;*/ } cf_sound_t;

/**
* 	paused = false;
*	looped = false;
*	volume = 1.0f;
*	pan = 0.5f;
*	pitch = 1.0f;
*	delay = 0;
*/
CUTE_API cf_sound_params_t CUTE_CALL cf_sound_params_defaults();
CUTE_API cf_sound_t CUTE_CALL cf_sound_play(cf_audio_t* audio_source, cf_sound_params_t params /*= cf_sound_params_defaults()*/, cf_error_t* err /*= NULL*/);

CUTE_API bool CUTE_CALL cf_sound_is_active(cf_sound_t sound);
CUTE_API bool CUTE_CALL cf_sound_get_is_paused(cf_sound_t sound);
CUTE_API bool CUTE_CALL cf_sound_get_is_looped(cf_sound_t sound);
CUTE_API float CUTE_CALL cf_sound_get_volume(cf_sound_t sound);
CUTE_API int CUTE_CALL cf_sound_get_sample_index(cf_sound_t sound);
CUTE_API void CUTE_CALL cf_sound_set_is_paused(cf_sound_t sound, bool true_for_paused);
CUTE_API void CUTE_CALL cf_sound_set_is_looped(cf_sound_t sound, bool true_for_looped);
CUTE_API void CUTE_CALL cf_sound_set_volume(cf_sound_t sound, float volume);
CUTE_API void CUTE_CALL cf_sound_set_sample_index(cf_sound_t sound, int sample_index);

// -------------------------------------------------------------------------------------------------

CUTE_API void CUTE_CALL cf_audio_set_pan(float pan);
CUTE_API void CUTE_CALL cf_audio_set_global_volume(float volume);
CUTE_API void CUTE_CALL cf_audio_set_sound_volume(float volume);
CUTE_API void CUTE_CALL cf_audio_set_pause(bool true_for_paused);

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef CUTE_CPP

namespace cute
{
using audio_t = cf_audio_t;
using app_t = cf_app_t;
using promise_t = cf_promise_t;

struct sound_params_t
{
	bool paused = false;
	bool looped = false;
	float volume = 1.0f;
	float pan = 0.5f;
	float pitch = 1.0f;
	float delay = 0;

	operator cf_sound_params_t()
	{
		return *((cf_sound_params_t*)this);
	}

	operator cf_sound_params_t* ()
	{
		return (cf_sound_params_t*)this;
	}
};

struct sound_t
{
	uint64_t id = 0;

	operator cf_sound_t()
	{
		return *((cf_sound_t*)this);
	}

	operator cf_sound_t* ()
	{
		return (cf_sound_t*)this;
	}
};


CUTE_INLINE audio_t* audio_load_ogg(const char* path, void* user_allocator_context = NULL) { return cf_audio_load_ogg(path, user_allocator_context); }
CUTE_INLINE audio_t* audio_load_wav(const char* path, void* user_allocator_context = NULL) { return cf_audio_load_wav(path, user_allocator_context); }
CUTE_INLINE audio_t* audio_load_ogg_from_memory(void* memory, int byte_count, void* user_allocator_context = NULL) { return cf_audio_load_ogg_from_memory(memory, byte_count, user_allocator_context); }
CUTE_INLINE audio_t* audio_load_wav_from_memory(void* memory, int byte_count, void* user_allocator_context = NULL) { return cf_audio_load_wav_from_memory(memory, byte_count, user_allocator_context); }
CUTE_INLINE void audio_stream_ogg(const char* path, promise_t promise, void* user_allocator_context = NULL) { cf_audio_stream_ogg(path, promise, user_allocator_context); }
CUTE_INLINE void audio_stream_wav(const char* path, promise_t promise, void* user_allocator_context = NULL) { cf_audio_stream_wav(path, promise, user_allocator_context); }
CUTE_INLINE void audio_stream_ogg_from_memory(void* memory, int byte_count, promise_t promise, void* user_allocator_context = NULL) { cf_audio_stream_ogg_from_memory(memory, byte_count, promise, user_allocator_context); }
CUTE_INLINE void audio_stream_wav_from_memory(void* memory, int byte_count, promise_t promise, void* user_allocator_context = NULL) { cf_audio_stream_wav_from_memory(memory, byte_count, promise, user_allocator_context); }
CUTE_INLINE error_t audio_destroy(audio_t* audio) { return cf_audio_destroy(audio); }
CUTE_INLINE int audio_ref_count(audio_t* audio) { return cf_audio_ref_count(audio); }

// -------------------------------------------------------------------------------------------------

CUTE_INLINE error_t music_play(audio_t* audio_source, float fade_in_time = 0) { return cf_music_play(audio_source, fade_in_time); }
CUTE_INLINE error_t music_stop(float fade_out_time = 0) { return cf_music_stop(fade_out_time = 0); }
CUTE_INLINE void music_set_volume(float volume) { cf_music_set_volume(volume); }
CUTE_INLINE void music_set_pitch(float pitch) { cf_music_set_pitch(pitch); }
CUTE_INLINE void music_set_loop(bool true_to_loop) { cf_music_set_loop(true_to_loop); }
CUTE_INLINE void music_pause(app_t* app) { cf_music_pause(app); }
CUTE_INLINE void music_resume(app_t* app) { cf_music_resume(app); }
CUTE_INLINE error_t music_switch_to(audio_t* audio_source, float fade_out_time = 0, float fade_in_time = 0) { return cf_music_switch_to(audio_source, fade_out_time, fade_in_time); }
CUTE_INLINE error_t music_crossfade(audio_t* audio_source, float cross_fade_time = 0) { return cf_music_crossfade(audio_source, cross_fade_time); }

// -------------------------------------------------------------------------------------------------

CUTE_INLINE sound_t sound_play(audio_t* audio_source, sound_params_t params = sound_params_t(), error_t* err = NULL) { return *(sound_t*)&cf_sound_play(audio_source, params, err); }

CUTE_INLINE bool sound_is_active(sound_t sound) { return cf_sound_is_active(sound); }
CUTE_INLINE bool sound_get_is_paused(sound_t sound) { return cf_sound_get_is_paused(sound); }
CUTE_INLINE bool sound_get_is_looped(sound_t sound) { return cf_sound_get_is_looped(sound); }
CUTE_INLINE float sound_get_volume(sound_t sound) { return cf_sound_get_volume(sound); }
CUTE_INLINE int sound_get_sample_index(sound_t sound) { return cf_sound_get_sample_index(sound); }
CUTE_INLINE void sound_set_is_paused(sound_t sound, bool true_for_paused) { cf_sound_set_is_paused(sound, true_for_paused); }
CUTE_INLINE void sound_set_is_looped(sound_t sound, bool true_for_looped) { cf_sound_set_is_looped(sound, true_for_looped); }
CUTE_INLINE void sound_set_volume(sound_t sound, float volume) { cf_sound_set_volume(sound, volume); }
CUTE_INLINE void sound_set_sample_index(sound_t sound, int sample_index) { cf_sound_set_sample_index(sound, sample_index); }

// -------------------------------------------------------------------------------------------------

CUTE_INLINE void audio_set_pan(float pan) { cf_audio_set_pan(pan); }
CUTE_INLINE void audio_set_global_volume(float volume) { cf_audio_set_global_volume(volume); }
CUTE_INLINE void audio_set_sound_volume(float volume) { cf_audio_set_sound_volume(volume); }
CUTE_INLINE void audio_set_pause(bool true_for_paused) { cf_audio_set_pause(true_for_paused); }
}

#endif // CUTE_CPP

#endif // CUTE_AUDIO_H
