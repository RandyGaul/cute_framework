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

namespace cute
{

struct cf_audio_t;

CUTE_API cf_audio_t* CUTE_CALL cf_audio_load_ogg(const char* path, void* user_allocator_context = NULL);
CUTE_API cf_audio_t* CUTE_CALL cf_audio_load_wav(const char* path, void* user_allocator_context = NULL);
CUTE_API cf_audio_t* CUTE_CALL cf_audio_load_ogg_from_memory(void* memory, int byte_count, void* user_allocator_context = NULL);
CUTE_API cf_audio_t* CUTE_CALL cf_audio_load_wav_from_memory(void* memory, int byte_count, void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL cf_audio_stream_ogg(const char* path, cf_promise_t promise, void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL cf_audio_stream_wav(const char* path, cf_promise_t promise, void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL cf_audio_stream_ogg_from_memory(void* memory, int byte_count, cf_promise_t promise, void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL cf_audio_stream_wav_from_memory(void* memory, int byte_count, cf_promise_t promise, void* user_allocator_context = NULL);
CUTE_API cf_error_t CUTE_CALL cf_audio_destroy(cf_audio_t* audio);
CUTE_API int CUTE_CALL cf_audio_ref_count(cf_audio_t* audio);

// -------------------------------------------------------------------------------------------------

CUTE_API cf_error_t CUTE_CALL cf_music_play(cf_audio_t* audio_source, float fade_in_time = 0);
CUTE_API cf_error_t CUTE_CALL cf_music_stop(float fade_out_time = 0);
CUTE_API void CUTE_CALL cf_music_set_volume(float volume);
CUTE_API void CUTE_CALL cf_music_set_pitch(float pitch);
CUTE_API void CUTE_CALL cf_music_set_loop(bool true_to_loop);
CUTE_API void CUTE_CALL cf_music_pause(cf_app_t* app);
CUTE_API void CUTE_CALL cf_music_resume(cf_app_t* app);
CUTE_API cf_error_t CUTE_CALL cf_music_switch_to(cf_audio_t* audio_source, float fade_out_time = 0, float fade_in_time = 0);
CUTE_API cf_error_t CUTE_CALL cf_music_crossfade(cf_audio_t* audio_source, float cross_fade_time = 0);

// -------------------------------------------------------------------------------------------------

struct cf_sound_params_t
{
	bool paused = false;
	bool looped = false;
	float volume = 1.0f;
	float pan = 0.5f;
	float pitch = 1.0f;
	float delay = 0;
};

struct cf_sound_t { uint64_t id = 0; };

CUTE_API cf_sound_t CUTE_CALL cf_sound_play(cf_audio_t* audio_source, cf_sound_params_t params = cf_sound_params_t(), cf_error_t* err = NULL);

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

}

#endif // CUTE_AUDIO_H
