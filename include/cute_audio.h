/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_AUDIO_H
#define CF_AUDIO_H

#include "cute_defines.h"
#include "cute_multithreading.h"
#include "cute_result.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Sound
 * @category audio
 * @brief    An opaque pointer representing a sound created by `cf_play_sound`.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
typedef struct CF_Sound { uint64_t id; } CF_Sound;
// @end

/**
 * @struct   CF_Audio
 * @category audio
 * @brief    An opaque pointer representing raw audio samples loaded as a resource.
 * @related  CF_Audio cf_audio_load_ogg cf_audio_load_ogg_from_memory cf_audio_load_wav cf_audio_load_wav_from_memory cf_audio_destroy cf_music_play cf_music_switch_to cf_music_crossfade cf_play_sound
 */
typedef struct CF_Audio { uint64_t id; } CF_Audio;
// @end

/**
 * @function cf_audio_load_ogg
 * @category audio
 * @brief    Loads a .ogg audio file.
 * @param    path         The virtual path to a .ogg file. See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @return   Returns a pointer to `CF_Audio`. Free it up with `cf_audio_destroy` when done.
 * @related  CF_Audio cf_audio_load_ogg cf_audio_load_ogg_from_memory cf_audio_load_wav cf_audio_load_wav_from_memory cf_audio_destroy
 */
CF_API CF_Audio CF_CALL cf_audio_load_ogg(const char* path);

/**
 * @function cf_audio_load_wav
 * @category audio
 * @brief    Loads a .wav audio file.
 * @param    path         The virtual path to a .wav file. See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @return   Returns a pointer to `CF_Audio`. Free it up with `cf_audio_destroy` when done.
 * @related  CF_Audio cf_audio_load_ogg cf_audio_load_ogg_from_memory cf_audio_load_wav cf_audio_load_wav_from_memory cf_audio_destroy
 */
CF_API CF_Audio CF_CALL cf_audio_load_wav(const char* path);

/**
 * @function cf_audio_load_ogg_from_memory
 * @category audio
 * @brief    Loads a .ogg audio file from memory.
 * @param    memory       A buffer containing the bytes of a .ogg file.
 * @param    byte_count   The number of bytes in `memory`.
 * @return   Returns a pointer to `CF_Audio`. Free it up with `cf_audio_destroy` when done.
 * @related  CF_Audio cf_audio_load_ogg cf_audio_load_ogg_from_memory cf_audio_load_wav cf_audio_load_wav_from_memory cf_audio_destroy
 */
CF_API CF_Audio CF_CALL cf_audio_load_ogg_from_memory(void* memory, int byte_count);

/**
 * @function cf_audio_load_wav_from_memory
 * @category audio
 * @brief    Loads a .wav audio file from memory.
 * @param    memory       A buffer containing the bytes of a .wav file.
 * @param    byte_count   The number of bytes in `memory`.
 * @return   Returns a pointer to `CF_Audio`. Free it up with `cf_audio_destroy` when done.
 * @related  CF_Audio cf_audio_load_ogg cf_audio_load_ogg_from_memory cf_audio_load_wav cf_audio_load_wav_from_memory cf_audio_destroy
 */
CF_API CF_Audio CF_CALL cf_audio_load_wav_from_memory(void* memory, int byte_count);

/**
 * @function cf_audio_destroy
 * @category audio
 * @brief    Frees all resources used by a `CF_Audio`.
 * @param    audio        A pointer to the `CF_Audio`.
 * @related  CF_Audio cf_audio_load_ogg cf_audio_load_ogg_from_memory cf_audio_load_wav cf_audio_load_wav_from_memory cf_audio_destroy
 */
CF_API void CF_CALL cf_audio_destroy(CF_Audio audio);

/**
 * @function cf_audio_cull_duplicates
 * @category audio
 * @brief    Turns on/off duplicate culling.
 * @remarks  This is turned on by default. If the exact same sound is played more than once on a given audio update it will simply
 *           amplify the volume of the sound. This can create an audio bug where the sound plays way louder than intended. This
 *           setting simply culls away any extra calls to `play_sound` for a particular sound.
 *           
 *           The audio update rate is determined by how quickly the audio mixing thread runs, meaning exactly how many sounds can
 *           get culled within a particular timespan is variable -- but usually you still want this option turned on.
 *           
 *           When on this applies to both sound FX and music.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume
 */
CF_API void CF_CALL cf_audio_cull_duplicates(bool true_to_cull_duplicates);

/**
 * @function cf_audio_sample_rate
 * @category audio
 * @brief    Returns the sample rate for a loaded audio resource.
 * @related  CF_Audio cf_audio_sample_rate cf_audio_sample_count cf_audio_channel_count
 */
CF_API int CF_CALL cf_audio_sample_rate(CF_Audio audio);

/**
 * @function cf_audio_sample_count
 * @category audio
 * @brief    Returns the sample count for a loaded audio resource.
 * @related  CF_Audio cf_audio_sample_rate cf_audio_sample_count cf_audio_channel_count
 */
CF_API int CF_CALL cf_audio_sample_count(CF_Audio audio);

/**
 * @function cf_audio_channel_count
 * @category audio
 * @brief    Returns the channel count for a loaded audio resource.
 * @related  CF_Audio cf_audio_sample_rate cf_audio_sample_count cf_audio_channel_count
 */
CF_API int CF_CALL cf_audio_channel_count(CF_Audio audio);

// -------------------------------------------------------------------------------------------------
// Global controls.

/**
 * @function cf_audio_set_pan
 * @category audio
 * @brief    Sets the global stereo pan for all audio.
 * @param    pan          0.5f means perfect balance for left/right speakers. 0.0f means only left speaker, 1.0f means only right speaker.
 * @related  cf_audio_set_pan cf_audio_set_global_volume cf_audio_set_sound_volume cf_audio_set_pause
 */
CF_API void CF_CALL cf_audio_set_pan(float pan);

/**
 * @function cf_audio_set_global_volume
 * @category audio
 * @brief    Sets the global volume for all audio.
 * @param    volume       A value from 0.0f to 1.0f, where 0.0f means no volume, and 1.0f means full volume.
 * @related  cf_audio_set_pan cf_audio_set_global_volume cf_audio_set_sound_volume cf_audio_set_pause
 */
CF_API void CF_CALL cf_audio_set_global_volume(float volume);

/**
 * @function cf_audio_set_sound_volume
 * @category audio
 * @brief    Sets the volume for all sound effects.
 * @param    volume       A value from 0.0f to 1.0f, where 0.0f means no volume, and 1.0f means full volume.
 * @remarks  Sounds come from `cf_play_sound`, as opposed to music coming from `cf_music_play`.
 * @related  cf_audio_set_pan cf_audio_set_global_volume cf_audio_set_sound_volume cf_audio_set_pause cf_play_sound
 */
CF_API void CF_CALL cf_audio_set_sound_volume(float volume);

/**
 * @function cf_audio_set_pause
 * @category audio
 * @brief    Pauses all audio.
 * @param    true_for_paused  True to pause all audio, false to unpause.
 * @related  cf_audio_set_pan cf_audio_set_global_volume cf_audio_set_sound_volume cf_audio_set_pause
 */
CF_API void CF_CALL cf_audio_set_pause(bool true_for_paused);

// -------------------------------------------------------------------------------------------------
// Music API.

/**
 * @function cf_music_play
 * @category audio
 * @brief    Plays audio as music.
 * @param    audio_source   A `CF_Audio` containing your music.
 * @param    fade_in_time   A number of seconds to fade the music in. Can be 0.0f to instantly play your music.
 * @related  cf_music_play cf_music_stop cf_music_set_volume cf_music_set_loop cf_music_pause cf_music_resume cf_music_switch_to cf_music_crossfade cf_music_get_sample_index cf_music_set_sample_index cf_music_set_pitch
 */
CF_API void CF_CALL cf_music_play(CF_Audio audio_source, float fade_in_time);

/**
 * @function cf_music_stop
 * @category audio
 * @brief    Stop the music currently playing.
 * @param    fade_out_time  A number of seconds to fade the music out. Can be 0.0f to instantly stop your music.
 * @related  cf_music_play cf_music_stop cf_music_set_volume cf_music_set_loop cf_music_pause cf_music_resume cf_music_switch_to cf_music_crossfade cf_music_get_sample_index cf_music_set_sample_index cf_music_set_pitch
 */
CF_API void CF_CALL cf_music_stop(float fade_out_time);

/**
 * @function cf_music_set_volume
 * @category audio
 * @brief    Set the volume for music.
 * @param    volume         A value from 0.0f to 1.0f, where 0.0f means no volume, and 1.0f means full volume.
 * @related  cf_music_play cf_music_stop cf_music_set_volume cf_music_set_loop cf_music_pause cf_music_resume cf_music_switch_to cf_music_crossfade cf_music_get_sample_index cf_music_set_sample_index cf_music_set_pitch
 */
CF_API void CF_CALL cf_music_set_volume(float volume);

/**
 * @function cf_music_set_loop
 * @category audio
 * @brief    Turns on or off music looping.
 * @param    true_to_loop   True to loop the music.
 * @related  cf_music_play cf_music_stop cf_music_set_volume cf_music_set_loop cf_music_pause cf_music_resume cf_music_switch_to cf_music_crossfade cf_music_get_sample_index cf_music_set_sample_index cf_music_set_pitch
 */
CF_API void CF_CALL cf_music_set_loop(bool true_to_loop);

/**
 * @function cf_music_set_pitch
 * @category audio
 * @brief    Sets the pitch for the music.
 * @param    pitch    The pitch to set, default to 1.0f.
 * @remarks  This is a playback speed multiplier, meaning negative numbers will play the music backwards,
 *           while positive numbers play forwards.
 * @related  cf_music_play cf_music_stop cf_music_set_volume cf_music_set_loop cf_music_pause cf_music_resume cf_music_switch_to cf_music_crossfade cf_music_get_sample_index cf_music_set_sample_index cf_music_set_pitch
 */
CF_API void CF_CALL cf_music_set_pitch(float pitch);

/**
 * @function cf_music_pause
 * @category audio
 * @brief    Pauses the music.
 * @related  cf_music_play cf_music_stop cf_music_set_volume cf_music_set_loop cf_music_pause cf_music_resume cf_music_switch_to cf_music_crossfade cf_music_get_sample_index cf_music_set_sample_index cf_music_set_pitch
 */
CF_API void CF_CALL cf_music_pause();

/**
 * @function cf_music_resume
 * @category audio
 * @brief    Resumes the music if the music was paused.
 * @related  cf_music_play cf_music_stop cf_music_set_volume cf_music_set_loop cf_music_pause cf_music_resume cf_music_switch_to cf_music_crossfade cf_music_get_sample_index cf_music_set_sample_index cf_music_set_pitch
 */
CF_API void CF_CALL cf_music_resume();

/**
 * @function cf_music_switch_to
 * @category audio
 * @brief    Switches from the currently playing music to another track.
 * @param    fade_out_time  A number of seconds to fade the currently playing track out. Can be 0.0f to instantly stop your music.
 * @param    fade_in_time   A number of seconds to fade the next track in. Can be 0.0f to instantly play your music.
 * @remarks  The currently playing track is faded out, then the second track is faded in.
 * @related  cf_music_play cf_music_stop cf_music_set_volume cf_music_set_loop cf_music_pause cf_music_resume cf_music_switch_to cf_music_crossfade cf_music_get_sample_index cf_music_set_sample_index cf_music_set_pitch
 */
CF_API void CF_CALL cf_music_switch_to(CF_Audio audio_source, float fade_out_time, float fade_in_time);

/**
 * @function cf_music_crossfade
 * @category audio
 * @brief    Crossfades the currently playing track out with the next track in.
 * @param    cross_fade_time  A number of seconds to crossfade to the next track. Can be 0.0f to instantly switch to the next track.
 * @related  cf_music_play cf_music_stop cf_music_set_volume cf_music_set_loop cf_music_pause cf_music_resume cf_music_switch_to cf_music_crossfade cf_music_get_sample_index cf_music_set_sample_index cf_music_set_pitch
 */
CF_API void CF_CALL cf_music_crossfade(CF_Audio audio_source, float cross_fade_time);

/**
 * @function cf_music_get_sample_index
 * @category audio
 * @brief    Returns the current sample index the music is playing at.
 * @remarks  This can be useful to sync a dynamic audio system that can turn on/off different instruments or sounds.
 * @related  cf_music_play cf_music_stop cf_music_set_volume cf_music_set_loop cf_music_pause cf_music_resume cf_music_switch_to cf_music_crossfade cf_music_get_sample_index cf_music_set_sample_index cf_music_set_pitch
 */
CF_API int CF_CALL cf_music_get_sample_index();

/**
 * @function cf_music_set_sample_index
 * @category audio
 * @brief    Sets the sample index to play at for the music.
 * @param    sample_index   Tells where to play music from within the `CF_Audio` for the currently playing music track.
 * @remarks  This can be useful to sync a dynamic audio system that can turn on/off different instruments or sounds.
 * @related  cf_music_play cf_music_stop cf_music_set_volume cf_music_set_loop cf_music_pause cf_music_resume cf_music_switch_to cf_music_crossfade cf_music_get_sample_index cf_music_set_sample_index cf_music_set_pitch
 */
CF_API CF_Result CF_CALL cf_music_set_sample_index(int sample_index);

// -------------------------------------------------------------------------------------------------
// Sound API.

/**
 * @struct   CF_SoundParams
 * @category audio
 * @brief    Parameters for the function `cf_play_sound`.
 * @remarks  You can use default settings from the `cf_sound_params_defaults` function.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop  cf_sound_set_pitch
 */
typedef struct CF_SoundParams
{
	/* @member Default: false. True to start the sound in a paused state. False to start immediately playing the sound. */
	bool paused;

	/* @member Default: false. True to loop the sound. */
	bool looped;

	/* @member Default: 0.5f. A volume control from 0.0f to 1.0f. 0.0f meaning silent, 1.0f meaning max volume. */
	float volume;

	/* @member Default: 0.5f. A stereo pan control from 0.0f to 1.0f. 0.0f means left-speaker, 1.0f means right speaker, 0.5f means equal both. */
	float pan;

	/* @member Default: 1.0f. Lower numbers lower the pitch and increase playback speed. Higher numbers increase the pitch and reduce playback speed. */
	float pitch;
} CF_SoundParams;
// @end

/**
 * @function cf_sound_params_defaults
 * @category audio
 * @brief    Returns a `CF_SoundParams` filled with default state, to use with `cf_play_sound`.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
CF_INLINE CF_SoundParams CF_CALL cf_sound_params_defaults()
{
	CF_SoundParams params;
	params.paused = false;
	params.looped = false;
	params.volume = 1.0f;
	params.pan = 0.5f;
	params.pitch = 1.0f;
	return params;
}

/**
 * @function cf_play_sound
 * @category audio
 * @brief    Plays a sound.
 * @param    audio_source   The `CF_Audio` samples for the sound to play.
 * @param    params         `CF_SoundParams` on how to play the sound. You can use default values by calling `cf_sound_params_defaults`.
 * @return   Returns a playing sound `CF_Sound`.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
CF_API CF_Sound CF_CALL cf_play_sound(CF_Audio audio_source, CF_SoundParams params);

/**
 * @function cf_sound_set_on_finish_callback
 * @category audio
 * @brief    Sets the callback for notifications of when a sound finishes playing.
 * @param    on_finished      Called whenever a `CF_Sound` finishes playing, including music.
 * @param    udata            An optional pointer handed back to you within the `on_finished` callback.
 * @param    single_threaded  Set to true to queue up callbacks and invoke them on the main thread. Otherwise this callback is called from the mixing thread directly.
 * @related  CF_Audio cf_audio_sample_rate cf_audio_sample_count cf_audio_channel_count
 */
CF_API void CF_CALL cf_sound_set_on_finish_callback(void (*on_finished)(CF_Sound, void*), void* udata, bool single_threaded);

/**
 * @function cf_sound_is_active
 * @category audio
 * @brief    Returns whether or not a sound is active.
 * @param    sound          The sound.
 * @return   Rreturns true if the sound is active, or false if it finished playing (and was not looped).
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
CF_API bool CF_CALL cf_sound_is_active(CF_Sound sound);

/**
 * @function cf_sound_get_is_paused
 * @category audio
 * @brief    Returns whether or not a sound is paused.
 * @param    sound          The sound.
 * @remarks  You can set a sound to paused with `cf_sound_set_is_paused`, or upon creation with `cf_play_sound`.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
CF_API bool CF_CALL cf_sound_get_is_paused(CF_Sound sound);

/**
 * @function cf_sound_get_is_looped
 * @category audio
 * @brief    Returns whether or not a sound is looped.
 * @param    sound          The sound.
 * @remarks  You can set a sound to looped with `cf_sound_set_is_looped`, or upon creation with `cf_play_sound`.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
CF_API bool CF_CALL cf_sound_get_is_looped(CF_Sound sound);

/**
 * @function cf_sound_get_volume
 * @category audio
 * @brief    Returns the volume of the sound.
 * @param    sound          The sound.
 * @remarks  You can set a sound volume with `cf_sound_set_volume`, or upon creation with `cf_play_sound`.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
CF_API float CF_CALL cf_sound_get_volume(CF_Sound sound);

/**
 * @function cf_sound_get_sample_index
 * @category audio
 * @brief    Returns the index of the currently playing sample for the sound.
 * @param    sound          The sound.
 * @remarks  You can set a sound's playing index with `cf_sound_set_sample_index`. This can be useful to sync a dynamic audio system that
 *           can turn on/off different instruments or sounds.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
CF_API int CF_CALL cf_sound_get_sample_index(CF_Sound sound);

/**
 * @function cf_sound_set_is_paused
 * @category audio
 * @brief    Sets the paused state for the sound.
 * @param    sound            The sound.
 * @param    true_for_paused  The pause state to set.
 * @remarks  You can get a sound's paused state with `cf_sound_get_is_paused`.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
CF_API void CF_CALL cf_sound_set_is_paused(CF_Sound sound, bool true_for_paused);

/**
 * @function cf_sound_set_is_looped
 * @category audio
 * @brief    Sets the looped state for the sound.
 * @param    sound            The sound.
 * @param    true_for_looped  The loop state to set.
 * @remarks  You can get a sound's looped state with `cf_sound_get_is_looped`.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
CF_API void CF_CALL cf_sound_set_is_looped(CF_Sound sound, bool true_for_looped);

/**
 * @function cf_sound_set_volume
 * @category audio
 * @brief    Sets the volume for the sound.
 * @param    sound      The sound.
 * @param    volume     A value from 0.0f to 1.0f. 0.0f meaning silent, 1.0f meaning max volume.
 * @remarks  You can get a sound's volume with `cf_sound_get_volume`.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
CF_API void CF_CALL cf_sound_set_volume(CF_Sound sound, float volume);

/**
 * @function cf_sound_set_pitch
 * @category audio
 * @brief    Sets pitch for the sound.
 * @remarks  Defaults to 1.0f.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
CF_API void CF_CALL cf_sound_set_pitch(CF_Sound sound, float pitch);

/**
 * @function cf_sound_set_sample_index
 * @category audio
 * @brief    Sets the sample index for the sound to control which sample to play next.
 * @param    sound         The sound.
 * @param    sample_index  The index of the sample to play the sound from.
 * @remarks  You can get a sound's playing index with `cf_sound_get_sample_index`. This can be useful to sync a dynamic audio system that
 *           can turn on/off different instruments or sounds.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
CF_API void CF_CALL cf_sound_set_sample_index(CF_Sound sound, int sample_index);

/**
 * @function cf_sound_stop
 * @category audio
 * @brief    Stops the sound instance so it no longer plays.
 * @related  CF_SoundParams CF_Sound cf_sound_params_defaults cf_play_sound cf_sound_is_active cf_sound_get_is_paused cf_sound_get_is_looped cf_sound_get_volume cf_sound_get_sample_index cf_sound_set_sample_index cf_sound_set_is_paused cf_sound_set_is_looped cf_sound_set_volume cf_sound_stop cf_sound_set_pitch
 */
CF_API void CF_CALL cf_sound_stop(CF_Sound sound);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

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
CF_INLINE Audio audio_load_ogg(const char* path) { return cf_audio_load_ogg(path); }
CF_INLINE Audio audio_load_wav(const char* path) { return cf_audio_load_wav(path); }
CF_INLINE Audio audio_load_ogg_from_memory(void* memory, int byte_count) { return cf_audio_load_ogg_from_memory(memory, byte_count); }
CF_INLINE Audio audio_load_wav_from_memory(void* memory, int byte_count) { return cf_audio_load_wav_from_memory(memory, byte_count); }
CF_INLINE void audio_destroy(Audio audio) { cf_audio_destroy(audio); }
CF_INLINE void audio_cull_duplicates(bool true_to_cull_duplicates = false) { cf_audio_cull_duplicates(true_to_cull_duplicates); }
CF_INLINE int audio_sample_rate(Audio audio) { return cf_audio_sample_rate(audio); }
CF_INLINE int audio_sample_count(Audio audio) { return cf_audio_sample_count(audio); }
CF_INLINE int audio_channel_count(Audio audio) { return cf_audio_channel_count(audio); }

// -------------------------------------------------------------------------------------------------

CF_INLINE void audio_set_pan(float pan) { cf_audio_set_pan(pan); }
CF_INLINE void audio_set_global_volume(float volume) { cf_audio_set_global_volume(volume); }
CF_INLINE void audio_set_sound_volume(float volume) { cf_audio_set_sound_volume(volume); }
CF_INLINE void audio_set_pause(bool true_for_paused) { cf_audio_set_pause(true_for_paused); }

// -------------------------------------------------------------------------------------------------

CF_INLINE void music_play(Audio audio_source, float fade_in_time = 0) { cf_music_play(audio_source, fade_in_time); }
CF_INLINE void music_stop(float fade_out_time = 0) { cf_music_stop(fade_out_time = 0); }
CF_INLINE void play_music(Audio audio_source, float fade_in_time = 0) { cf_music_play(audio_source, fade_in_time); }
CF_INLINE void stop_music(float fade_out_time = 0) { cf_music_stop(fade_out_time = 0); }
CF_INLINE void music_set_volume(float volume) { cf_music_set_volume(volume); }
CF_INLINE void music_set_loop(bool true_to_loop) { cf_music_set_loop(true_to_loop); }
CF_INLINE void music_set_pitch(float pitch = 1.0f) { cf_music_set_pitch(pitch); }
CF_INLINE void music_pause() { cf_music_pause(); }
CF_INLINE void music_resume() { cf_music_resume(); }
CF_INLINE void music_switch_to(Audio audio_source, float fade_out_time = 0, float fade_in_time = 0) { cf_music_switch_to(audio_source, fade_out_time, fade_in_time); }
CF_INLINE void music_crossfade(Audio audio_source, float cross_fade_time = 0) { cf_music_crossfade(audio_source, cross_fade_time); }
CF_INLINE void music_set_sample_index(int sample_index) { cf_music_set_sample_index(sample_index); }
CF_INLINE int music_get_sample_index() { return cf_music_get_sample_index(); }

// -------------------------------------------------------------------------------------------------

CF_INLINE Sound sound_play(Audio audio_source, SoundParams params = SoundParams()) { return cf_play_sound(audio_source, params); }
CF_INLINE Sound play_sound(Audio audio_source, SoundParams params = SoundParams()) { return cf_play_sound(audio_source, params); }

CF_INLINE bool sound_is_active(Sound sound) { return cf_sound_is_active(sound); }
CF_INLINE bool sound_get_is_paused(Sound sound) { return cf_sound_get_is_paused(sound); }
CF_INLINE bool sound_get_is_looped(Sound sound) { return cf_sound_get_is_looped(sound); }
CF_INLINE float sound_get_volume(Sound sound) { return cf_sound_get_volume(sound); }
CF_INLINE int sound_get_sample_index(Sound sound) { return cf_sound_get_sample_index(sound); }
CF_INLINE void sound_set_is_paused(Sound sound, bool true_for_paused) { cf_sound_set_is_paused(sound, true_for_paused); }
CF_INLINE void sound_set_is_looped(Sound sound, bool true_for_looped) { cf_sound_set_is_looped(sound, true_for_looped); }
CF_INLINE void sound_set_volume(Sound sound, float volume) { cf_sound_set_volume(sound, volume); }
CF_INLINE void sound_set_pitch(Sound sound, float pitch = 1.0f) { cf_sound_set_pitch(sound, pitch); }
CF_INLINE void sound_set_sample_index(Sound sound, int sample_index) { cf_sound_set_sample_index(sound, sample_index); }
CF_INLINE void sound_stop(Sound sound) { cf_sound_stop(sound); }

}

#endif // CF_CPP

#endif // CF_AUDIO_H
