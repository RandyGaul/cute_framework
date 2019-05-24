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

#include <cute_defines.h>
#include <cute_concurrency.h>
#include <cute_error.h>

namespace cute
{

struct audio_t;

extern CUTE_API audio_t* CUTE_CALL audio_load_ogg(const char* path, void* user_allocator_context = NULL);
extern CUTE_API audio_t* CUTE_CALL audio_load_wav(const char* path, void* user_allocator_context = NULL);
extern CUTE_API audio_t* CUTE_CALL audio_load_ogg_from_memory(void* memory, int byte_count, void* user_allocator_context = NULL);
extern CUTE_API audio_t* CUTE_CALL audio_load_wav_from_memory(void* memory, int byte_count, void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL audio_stream_ogg(app_t* app, const char* path, promise_t promise, void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL audio_stream_wav(app_t* app, const char* path, promise_t promise, void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL audio_stream_ogg_from_memory(app_t* app, void* memory, int byte_count, promise_t promise, void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL audio_stream_wav_from_memory(app_t* app, void* memory, int byte_count, promise_t promise, void* user_allocator_context = NULL);
extern CUTE_API error_t CUTE_CALL audio_destroy(audio_t* audio);
extern CUTE_API int CUTE_CALL audio_ref_count(audio_t* audio_source);

extern CUTE_API error_t CUTE_CALL music_play(app_t* app, audio_t* audio_source, float fade_in_time = 0, float delay = 0);
extern CUTE_API void CUTE_CALL music_stop(app_t* app, float fade_out_time = 0);
extern CUTE_API void CUTE_CALL music_pause(app_t* app);
extern CUTE_API void CUTE_CALL music_resume(app_t* app);
extern CUTE_API error_t CUTE_CALL music_switch_to(app_t* app, audio_t* audio_source, float fade_out_time = 0, float fade_in_time = 0);
extern CUTE_API error_t CUTE_CALL music_crossfade_to(app_t* app, audio_t* audio_source, float cross_fade_time = 0);
extern CUTE_API void CUTE_CALL music_set_loop(app_t* app, int loop);
extern CUTE_API void CUTE_CALL music_set_volume(app_t* app, float volume);
extern CUTE_API void CUTE_CALL music_set_pan(app_t* app, float pan);
extern CUTE_API void CUTE_CALL music_pitch(app_t* app, float pitch);

struct sound_def_t
{
	int paused = 0;
	int looped = 0;
	float volume = 1.0f;
	float pan = 0.5f;
	float pitch = 1.0f;
	float delay = 0;
};

extern CUTE_API void CUTE_CALL sound_play(app_t* app, audio_t* audio_source, sound_def_t def);

}

#endif // CUTE_AUDIO_H
