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
#include "cute_result.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct CF_Audio CF_Audio;

CUTE_API CF_Audio* CUTE_CALL cf_audio_load_ogg(const char* path /*= NULL*/);
CUTE_API CF_Audio* CUTE_CALL cf_audio_load_wav(const char* path /*= NULL*/);
CUTE_API CF_Audio* CUTE_CALL cf_audio_load_ogg_from_memory(void* memory, int byte_count);
CUTE_API CF_Audio* CUTE_CALL cf_audio_load_wav_from_memory(void* memory, int byte_count);
CUTE_API void CUTE_CALL cf_audio_destroy(CF_Audio* audio);

// -------------------------------------------------------------------------------------------------

CUTE_API void CUTE_CALL cf_audio_set_pan(float pan);
CUTE_API void CUTE_CALL cf_audio_set_global_volume(float volume);
CUTE_API void CUTE_CALL cf_audio_set_sound_volume(float volume);
CUTE_API void CUTE_CALL cf_audio_set_pause(bool true_for_paused);

// -------------------------------------------------------------------------------------------------

CUTE_API void CUTE_CALL cf_music_play(CF_Audio* audio_source, float fade_in_time /*= 0*/);
CUTE_API void CUTE_CALL cf_music_stop(float fade_out_time /*= 0*/);
CUTE_API void CUTE_CALL cf_music_set_volume(float volume);
CUTE_API void CUTE_CALL cf_music_set_loop(bool true_to_loop);
CUTE_API void CUTE_CALL cf_music_pause();
CUTE_API void CUTE_CALL cf_music_resume();
CUTE_API void CUTE_CALL cf_music_switch_to(CF_Audio* audio_source, float fade_out_time /*= 0*/, float fade_in_time /*= 0*/);
CUTE_API void CUTE_CALL cf_music_crossfade(CF_Audio* audio_source, float cross_fade_time /*= 0*/);
CUTE_API uint64_t CUTE_CALL cf_music_get_sample_index();
CUTE_API CF_Result CUTE_CALL cf_music_set_sample_index(uint64_t sample_index);

// -------------------------------------------------------------------------------------------------

typedef struct CF_SoundParams
{
	bool paused;
	bool looped;
	float volume;
	float pan;
	float delay;
} CF_SoundParams;

typedef struct CF_Sound { uint64_t id; } CF_Sound;

CUTE_INLINE CF_SoundParams CUTE_CALL cf_sound_params_defaults()
{
	CF_SoundParams params;
	params.paused = false;
	params.looped = false;
	params.volume = 1.0f;
	params.pan = 0.5f;
	params.delay = 0;
	return params;
}

CUTE_API CF_Sound CUTE_CALL cf_play_sound(CF_Audio* audio_source, CF_SoundParams params /*= cf_sound_params_defaults()*/, CF_Result* err /*= NULL*/);

CUTE_API bool CUTE_CALL cf_sound_is_active(CF_Sound sound);
CUTE_API bool CUTE_CALL cf_sound_get_is_paused(CF_Sound sound);
CUTE_API bool CUTE_CALL cf_sound_get_is_looped(CF_Sound sound);
CUTE_API float CUTE_CALL cf_sound_get_volume(CF_Sound sound);
CUTE_API uint64_t CUTE_CALL cf_sound_get_sample_index(CF_Sound sound);
CUTE_API void CUTE_CALL cf_sound_set_is_paused(CF_Sound sound, bool true_for_paused);
CUTE_API void CUTE_CALL cf_sound_set_is_looped(CF_Sound sound, bool true_for_looped);
CUTE_API void CUTE_CALL cf_sound_set_volume(CF_Sound sound, float volume);
CUTE_API void CUTE_CALL cf_sound_set_sample_index(CF_Sound sound, uint64_t sample_index);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using Audio = CF_Audio;

struct SoundParams : public CF_SoundParams
{
	SoundParams() { *(CF_SoundParams*)this = cf_sound_params_defaults(); }
	SoundParams(CF_SoundParams sp) { *(CF_SoundParams*)this = sp; }
};

struct Sound : public CF_Sound
{
	Sound() { id = -1; }
	Sound(CF_Sound s) { *(CF_Sound*)this = s; }
};

CUTE_INLINE Audio* audio_load_ogg(const char* path = NULL) { return cf_audio_load_ogg(path); }
CUTE_INLINE Audio* audio_load_wav(const char* path = NULL) { return cf_audio_load_wav(path); }
CUTE_INLINE Audio* audio_load_ogg_from_memory(void* memory, int byte_count) { return cf_audio_load_ogg_from_memory(memory, byte_count); }
CUTE_INLINE Audio* audio_load_wav_from_memory(void* memory, int byte_count) { return cf_audio_load_wav_from_memory(memory, byte_count); }
CUTE_INLINE void audio_destroy(Audio* audio) { cf_audio_destroy(audio); }

// -------------------------------------------------------------------------------------------------

CUTE_INLINE void audio_set_pan(float pan) { cf_audio_set_pan(pan); }
CUTE_INLINE void audio_set_global_volume(float volume) { cf_audio_set_global_volume(volume); }
CUTE_INLINE void audio_set_sound_volume(float volume) { cf_audio_set_sound_volume(volume); }
CUTE_INLINE void audio_set_pause(bool true_for_paused) { cf_audio_set_pause(true_for_paused); }

// -------------------------------------------------------------------------------------------------

CUTE_INLINE void music_play(Audio* audio_source, float fade_in_time = 0) { cf_music_play(audio_source, fade_in_time); }
CUTE_INLINE void music_stop(float fade_out_time = 0) { cf_music_stop(fade_out_time = 0); }
CUTE_INLINE void music_set_volume(float volume) { cf_music_set_volume(volume); }
CUTE_INLINE void music_set_loop(bool true_to_loop) { cf_music_set_loop(true_to_loop); }
CUTE_INLINE void music_pause() { cf_music_pause(); }
CUTE_INLINE void music_resume() { cf_music_resume(); }
CUTE_INLINE void music_switch_to(Audio* audio_source, float fade_out_time = 0, float fade_in_time = 0) { cf_music_switch_to(audio_source, fade_out_time, fade_in_time); }
CUTE_INLINE void music_crossfade(Audio* audio_source, float cross_fade_time = 0) { cf_music_crossfade(audio_source, cross_fade_time); }
CUTE_INLINE void music_set_sample_index(uint64_t sample_index) { cf_music_set_sample_index(sample_index); }
CUTE_INLINE uint64_t music_get_sample_index() { return cf_music_get_sample_index(); }

// -------------------------------------------------------------------------------------------------

CUTE_INLINE Sound sound_play(Audio* audio_source, SoundParams params = SoundParams(), Result* err = NULL) { return cf_play_sound(audio_source, params, err); }

CUTE_INLINE bool sound_is_active(Sound sound) { return cf_sound_is_active(sound); }
CUTE_INLINE bool sound_get_is_paused(Sound sound) { return cf_sound_get_is_paused(sound); }
CUTE_INLINE bool sound_get_is_looped(Sound sound) { return cf_sound_get_is_looped(sound); }
CUTE_INLINE float sound_get_volume(Sound sound) { return cf_sound_get_volume(sound); }
CUTE_INLINE uint64_t sound_get_sample_index(Sound sound) { return cf_sound_get_sample_index(sound); }
CUTE_INLINE void sound_set_is_paused(Sound sound, bool true_for_paused) { cf_sound_set_is_paused(sound, true_for_paused); }
CUTE_INLINE void sound_set_is_looped(Sound sound, bool true_for_looped) { cf_sound_set_is_looped(sound, true_for_looped); }
CUTE_INLINE void sound_set_volume(Sound sound, float volume) { cf_sound_set_volume(sound, volume); }
CUTE_INLINE void sound_set_sample_index(Sound sound, int sample_index) { cf_sound_set_sample_index(sound, sample_index); }

}

#endif // CUTE_CPP

#endif // CUTE_AUDIO_H
