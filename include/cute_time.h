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

#ifndef CUTE_TIMER_H
#define CUTE_TIMER_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CUTE_API extern float CF_DELTA_TIME;
CUTE_API extern float CF_DELTA_TIME_FIXED;
CUTE_API extern float CF_DELTA_TIME_INTERPOLANT;
CUTE_API extern uint64_t CF_TICKS;
CUTE_API extern uint64_t CF_PREV_TICKS;
CUTE_API extern double CF_SECONDS;
CUTE_API extern double CF_PREV_SECONDS;
CUTE_API extern float CF_PAUSE_TIME_LEFT;

typedef void (CF_OnUpdateFn)();

CUTE_API void CUTE_CALL cf_set_fixed_timestep(int frames_per_second);
CUTE_API void CUTE_CALL cf_set_fixed_timestep_max_updates(int max_updates);
CUTE_API void CUTE_CALL cf_update_time(CF_OnUpdateFn* on_update);
CUTE_API void CUTE_CALL cf_pause_for(float seconds);
CUTE_API void CUTE_CALL cf_pause_for_ticks(uint64_t pause_ticks);

CUTE_API bool CUTE_CALL cf_on_interval(float interval, float offset);
CUTE_API bool CUTE_CALL cf_between_interval(float interval, float offset);
CUTE_API bool CUTE_CALL cf_on_timestamp(double timestamp);
CUTE_API bool CUTE_CALL cf_is_paused();

CUTE_API uint64_t CUTE_CALL cf_get_ticks();
CUTE_API uint64_t CUTE_CALL cf_get_tick_frequency();
CUTE_API void CUTE_CALL cf_sleep(int milliseconds);

typedef struct CF_Stopwatch { uint64_t start_time; } CF_Stopwatch;

CUTE_API CF_Stopwatch CUTE_CALL cf_make_stopwatch();
CUTE_API uint64_t CUTE_CALL cf_seconds(CF_Stopwatch stopwatch);
CUTE_API uint64_t CUTE_CALL cf_milliseconds(CF_Stopwatch stopwatch);
CUTE_API uint64_t CUTE_CALL cf_microseconds(CF_Stopwatch stopwatch);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

#define DELTA_TIME (CF_DELTA_TIME)
#define DELTA_TIME_FIXED (CF_DELTA_TIME_FIXED)
#define DELTA_TIME_INTERPOLANT (CF_DELTA_TIME_INTERPOLANT)
using OnUpdateFn = CF_OnUpdateFn;
using Stopwatch = CF_Stopwatch;

CUTE_INLINE void set_fixed_timestep(int frames_per_second = 60) { cf_set_fixed_timestep(frames_per_second); }
CUTE_INLINE void set_fixed_timestep_max_updates(int max_updates = 5) { cf_set_fixed_timestep_max_updates(max_updates); }
CUTE_INLINE void update_time(OnUpdateFn* on_update = NULL) { cf_update_time(on_update); }
CUTE_INLINE void pause_for(float seconds) { cf_pause_for(seconds); }
CUTE_INLINE void pause_for_ticks(uint64_t pause_ticks) { cf_pause_for_ticks(pause_ticks); }

CUTE_INLINE bool on_interval(float interval, float offset = 0) { return cf_on_interval(interval, offset); }
CUTE_INLINE bool between_interval(float interval, float offset = 0) { return cf_between_interval(interval, offset); }
CUTE_INLINE bool on_timestamp(double timestamp) { return cf_on_timestamp(timestamp); }
CUTE_INLINE bool is_paused() { return cf_is_paused(); }

CUTE_INLINE uint64_t get_ticks() { return cf_get_ticks(); }
CUTE_INLINE uint64_t get_tick_frequency() { return cf_get_tick_frequency(); }
CUTE_INLINE void sleep(int milliseconds) { cf_sleep(milliseconds); }

CUTE_INLINE CF_Stopwatch make_stopwatch() { return cf_make_stopwatch(); }
CUTE_INLINE uint64_t seconds(CF_Stopwatch stopwatch) { return cf_seconds(stopwatch); }
CUTE_INLINE uint64_t milliseconds(CF_Stopwatch stopwatch) { return cf_milliseconds(stopwatch); }
CUTE_INLINE uint64_t microseconds(CF_Stopwatch stopwatch) { return cf_microseconds(stopwatch); }

}

#endif // CUTE_CPP

#endif // CUTE_TIMER_H
