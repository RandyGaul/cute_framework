/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_audio.h>
#include <cute_file_system.h>
#include <cute_alloc.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_app_internal.h>

#define STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>

#ifdef CF_EMSCRIPTEN
#	ifndef CUTE_SOUND_SCALAR_MODE
#		define CUTE_SOUND_SCALAR_MODE
#	endif // CUTE_SOUND_SCALAR_MODE
#endif // CF_EMSCRIPTEN

#define CUTE_SOUND_IMPLEMENTATION
#define CUTE_SOUND_FORCE_SDL
#include <cute/cute_sound.h>

CF_Audio cf_audio_load_ogg(const char* path)
{
	size_t size;
	void* data = cf_fs_read_entire_file_to_memory(path, &size);
	if (data) {
		CF_Audio src = cf_audio_load_ogg_from_memory(data, (int)size);
		CF_FREE(data);
		return src;
	} else {
		return { NULL };
	}
}

CF_Audio cf_audio_load_wav(const char* path)
{
	size_t size;
	void* data = cf_fs_read_entire_file_to_memory(path, &size);
	if (data) {
		auto src = cf_audio_load_wav_from_memory(data, (int)size);
		CF_FREE(data);
		return (CF_Audio)src;
	} else {
		return { NULL };
	}
}

CF_Audio cf_audio_load_ogg_from_memory(void* memory, int byte_count)
{
	cs_audio_source_t* src = cs_read_mem_ogg(memory, (size_t)byte_count, NULL);
	CF_Audio result = { (uint64_t)src };
	return result;
}

CF_Audio cf_audio_load_wav_from_memory(void* memory, int byte_count)
{
	cs_audio_source_t* src = cs_read_mem_wav(memory, (size_t)byte_count, NULL);
	CF_Audio result = { (uint64_t)src };
	return result;
}

void cf_audio_destroy(CF_Audio audio)
{
	cs_free_audio_source((cs_audio_source_t*)audio.id);
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

static inline CF_Result s_result(cs_error_t err)
{
	if (err == CUTE_SOUND_ERROR_NONE) return cf_result_success();
	else {
		CF_Result result;
		result.code = CF_RESULT_ERROR;
		result.details = cs_error_as_string(err);
		return result;
	}
}

void cf_music_play(CF_Audio audio_source, float fade_in_time)
{
	cs_music_play((cs_audio_source_t*)audio_source.id, fade_in_time);
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

void cf_music_switch_to(CF_Audio audio_source, float fade_out_time, float fade_in_time)
{
	return cs_music_switch_to((cs_audio_source_t*)audio_source.id, fade_out_time, fade_in_time);
}

void cf_music_crossfade(CF_Audio audio_source, float cross_fade_time)
{
	return cs_music_crossfade((cs_audio_source_t*)audio_source.id, cross_fade_time);
}

int cf_music_get_sample_index()
{
	return cs_music_get_sample_index();
}

CF_Result cf_music_set_sample_index(int sample_index)
{
	return s_result(cs_music_set_sample_index(sample_index));
}

void cf_music_set_pitch(float pitch)
{
	cs_music_set_pitch(pitch);
}

// -------------------------------------------------------------------------------------------------

CF_Sound cf_play_sound(CF_Audio audio_source, CF_SoundParams params)
{
	cs_sound_params_t csparams;
	csparams.paused = params.paused;
	csparams.looped = params.looped;
	csparams.volume = params.volume;
	csparams.pan = params.pan;
	csparams.pitch = params.pitch;
	CF_Sound result;
	cs_playing_sound_t csresult = cs_play_sound((cs_audio_source_t*)audio_source.id, csparams);
	result.id = csresult.id;
	return result;
}

void s_on_finish(CF_Sound snd, void* udata)
{
	if (app->on_sound_finish_single_threaded) {
		app->on_sound_finish_queue.add(snd);
	} else {
		app->on_sound_finish(snd, udata);
	}
}

void cf_sound_set_on_finish_callback(void (*on_finish)(CF_Sound, void*), void* udata, bool single_threaded)
{
	app->on_sound_finish_single_threaded = single_threaded;
	app->on_sound_finish = on_finish;
	app->on_sound_finish_udata = udata;
	cs_on_sound_finished_callback((void (*)(cs_playing_sound_t, void*))s_on_finish, udata);
}

bool cf_sound_is_active(CF_Sound sound)
{
	cs_playing_sound_t cssound = { sound.id };
	return cs_sound_is_active(cssound);
}

bool cf_sound_get_is_paused(CF_Sound sound)
{
	cs_playing_sound_t cssound = { sound.id };
	return cs_sound_get_is_paused(cssound);
}

bool cf_sound_get_is_looped(CF_Sound sound)
{
	cs_playing_sound_t cssound = { sound.id };
	return cs_sound_get_is_looped(cssound);
}

float cf_sound_get_volume(CF_Sound sound)
{
	cs_playing_sound_t cssound = { sound.id };
	return cs_sound_get_volume(cssound);
}

int cf_sound_get_sample_index(CF_Sound sound)
{
	cs_playing_sound_t cssound = { sound.id };
	return cs_sound_get_sample_index(cssound);
}

void cf_sound_set_is_paused(CF_Sound sound, bool true_for_paused)
{
	cs_playing_sound_t cssound = { sound.id };
	cs_sound_set_is_paused(cssound, true_for_paused);
}

void cf_sound_set_is_looped(CF_Sound sound, bool true_for_looped)
{
	cs_playing_sound_t cssound = { sound.id };
	cs_sound_set_is_looped(cssound, true_for_looped);
}

void cf_sound_set_volume(CF_Sound sound, float volume)
{
	cs_playing_sound_t cssound = { sound.id };
	cs_sound_set_volume(cssound, volume);
}

void cf_sound_set_sample_index(CF_Sound sound, int sample_index)
{
	cs_playing_sound_t cssound = { sound.id };
	cs_sound_set_sample_index(cssound, sample_index);
}

void cf_sound_stop(CF_Sound sound)
{
	cs_playing_sound_t cssound = { sound.id };
	cs_sound_stop(cssound);
}

void cf_sound_set_pitch(CF_Sound sound, float pitch)
{
	cs_playing_sound_t cssound = { sound.id };
	cs_sound_set_pitch(cssound, pitch);
}

void cf_audio_cull_duplicates(bool true_to_cull_duplicates)
{
	cs_cull_duplicates(true_to_cull_duplicates);
}

int cf_audio_sample_rate(CF_Audio audio)
{
	cs_audio_source_t* src = (cs_audio_source_t*)audio.id;
	return src->sample_rate;
}

int cf_audio_sample_count(CF_Audio audio)
{
	cs_audio_source_t* src = (cs_audio_source_t*)audio.id;
	return src->sample_count;
}

int cf_audio_channel_count(CF_Audio audio)
{
	cs_audio_source_t* src = (cs_audio_source_t*)audio.id;
	return src->channel_count;
}

#undef STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>
