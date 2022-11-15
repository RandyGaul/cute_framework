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

#define STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>

#define CUTE_SOUND_IMPLEMENTATION
#define CUTE_SOUND_FORCE_SDL
#include <cute/cute_sound.h>

using namespace cute;

cf_audio_t* cf_audio_load_ogg(const char* path)
{
	auto src = cs_load_ogg(path, NULL);
	return (cf_audio_t*) src;
}

cf_audio_t* cf_audio_load_wav(const char* path)
{
	auto src = cs_load_wav(path, NULL);
	return (cf_audio_t*) src;
}

cf_audio_t* cf_audio_load_ogg_from_memory(void* memory, int byte_count)
{
	auto src = cs_read_mem_ogg(memory, (size_t)byte_count, NULL);
	return (cf_audio_t*)src;
}

cf_audio_t* cf_audio_load_wav_from_memory(void* memory, int byte_count)
{
	auto src = cs_read_mem_wav(memory, (size_t)byte_count, NULL);
	return (cf_audio_t*)src;
}

void cf_audio_destroy(cf_audio_t* audio)
{
	cs_free_audio_source((cs_audio_source_t*)audio);
}

// -------------------------------------------------------------------------------------------------

void cf_audio_set_pan(float pan)
{
	cs_set_global_pan(pan);
}

void cf_audio_set_global_volume(float volume)
{
	cs_set_global_volume(volume);
}

void cf_audio_set_sound_volume(float volume)
{
	cs_set_playing_sounds_volume(volume);
}

void cf_audio_set_pause(bool true_for_paused)
{
	cs_set_global_pause(true_for_paused);
}

// -------------------------------------------------------------------------------------------------

static inline cf_result_t s_result(cs_error_t err)
{
	if (err == CUTE_SOUND_ERROR_NONE) return result_success();
	else {
		result_t result;
		result.code = RESULT_ERROR;
		result.details = cs_error_as_string(err);
		return result;
	}
}

void cf_music_play(cf_audio_t* audio_source, float fade_in_time)
{
	cs_music_play((cs_audio_source_t*)audio_source, fade_in_time);
}

void cf_music_stop(float fade_out_time)
{
	cs_music_stop(fade_out_time);
}

void cf_music_set_volume(float volume)
{
	cs_music_set_volume(volume);
}

void cf_music_set_loop(bool true_to_loop)
{
	cs_music_set_loop(true_to_loop);
}

void cf_music_pause()
{
	cs_music_pause();
}

void cf_music_resume()
{
	cs_music_resume();
}

void cf_music_switch_to(cf_audio_t* audio_source, float fade_out_time, float fade_in_time)
{
	return cs_music_switch_to((cs_audio_source_t*)audio_source, fade_out_time, fade_in_time);
}

void cf_music_crossfade(cf_audio_t* audio_source, float cross_fade_time)
{
	return cs_music_crossfade((cs_audio_source_t*)audio_source, cross_fade_time);
}

uint64_t cf_music_get_sample_index()
{
	return cs_music_get_sample_index();
}

cf_result_t cf_music_set_sample_index(uint64_t sample_index)
{
	return s_result(cs_music_set_sample_index(sample_index));
}

// -------------------------------------------------------------------------------------------------

cf_sound_t cf_play_sound(cf_audio_t* audio_source, cf_sound_params_t params /*= cf_sound_params_defaults()*/, cf_result_t* err)
{
	cs_sound_params_t csparams;
	csparams.paused = params.paused;
	csparams.looped = params.looped;
	csparams.volume = params.volume;
	csparams.pan = params.pan;
	csparams.delay = params.delay;
	cf_sound_t result;
	cs_playing_sound_t csresult = cs_play_sound((cs_audio_source_t*)audio_source, csparams);
	result.id = csresult.id;
	return result;
}

bool cf_sound_is_active(cf_sound_t sound)
{
	cs_playing_sound_t cssound = { sound.id };
	return cs_sound_is_active(cssound);
}

bool cf_sound_get_is_paused(cf_sound_t sound)
{
	cs_playing_sound_t cssound = { sound.id };
	return cs_sound_get_is_paused(cssound);
}

bool cf_sound_get_is_looped(cf_sound_t sound)
{
	cs_playing_sound_t cssound = { sound.id };
	return cs_sound_get_is_looped(cssound);
}

float cf_sound_get_volume(cf_sound_t sound)
{
	cs_playing_sound_t cssound = { sound.id };
	return cs_sound_get_volume(cssound);
}

uint64_t cf_sound_get_sample_index(cf_sound_t sound)
{
	cs_playing_sound_t cssound = { sound.id };
	return cs_sound_get_sample_index(cssound);
}

void cf_sound_set_is_paused(cf_sound_t sound, bool true_for_paused)
{
	cs_playing_sound_t cssound = { sound.id };
	cs_sound_set_is_paused(cssound, true_for_paused);
}

void cf_sound_set_is_looped(cf_sound_t sound, bool true_for_looped)
{
	cs_playing_sound_t cssound = { sound.id };
	cs_sound_set_is_looped(cssound, true_for_looped);
}

void cf_sound_set_volume(cf_sound_t sound, float volume)
{
	cs_playing_sound_t cssound = { sound.id };
	cs_sound_set_volume(cssound, volume);
}

void cf_sound_set_sample_index(cf_sound_t sound, uint64_t sample_index)
{
	cs_playing_sound_t cssound = { sound.id };
	cs_sound_set_sample_index(cssound, sample_index);
}

#undef STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>
