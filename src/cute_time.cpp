/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#include <cute_time.h>
#include <cute_math.h>
#include <cute_c_runtime.h>

using namespace Cute;

#include <SDL.h>

#include <stdio.h>
#include <inttypes.h>

// Initial API design by Noel Berry's blah_time.h
// https://github.com/NoelFB/blah/blob/master/include/blah_time.h

float CF_DELTA_TIME;
float CF_DELTA_TIME_FIXED;
float CF_DELTA_TIME_INTERPOLANT;
uint64_t CF_TICKS;
uint64_t CF_PREV_TICKS;
double CF_SECONDS;
double CF_PREV_SECONDS;
float CF_PAUSE_TIME_LEFT;

double inv_freq;
double freq;

uint64_t prev_ticks;
int64_t pause_ticks;

uint64_t ticks_per_second;
uint64_t ticks_per_timestep;
int max_updates = 5;
uint64_t unsimulated_ticks;

static void s_init()
{
	if (inv_freq == 0) {
		ticks_per_second = SDL_GetPerformanceFrequency();
		freq = (double)ticks_per_second;
		inv_freq = 1.0 / freq;

		// Pretend the first frame is 1/60th of a second.
		prev_ticks = SDL_GetPerformanceCounter() - (uint64_t)((1.0/60.0) * freq);
	}
}

void cf_set_fixed_timestep(int frames_per_second)
{
	s_init();
	CF_DELTA_TIME_FIXED = 1.0f / frames_per_second;
	ticks_per_timestep = (uint64_t)((1.0 / frames_per_second) * freq);
}

void cf_set_fixed_timestep_max_updates(int max_updates)
{
	::max_updates = max(1, max_updates);
}

static void s_step(uint64_t delta)
{
	CF_PREV_TICKS = CF_TICKS;
	CF_TICKS += delta;
	CF_PREV_SECONDS = CF_SECONDS;
	CF_SECONDS = CF_TICKS * inv_freq;
}

void cf_update_time(CF_OnUpdateFn* on_update)
{
	if (ticks_per_timestep) {
		// Fixed timestep (opt-in only).
		// Related reading: https://gafferongames.com/post/fix_your_timestep/

		// Accumulate unsimulated time.
		uint64_t now = cf_get_ticks();
		uint64_t old_prev_ticks = prev_ticks;
		uint64_t delta = now - prev_ticks;
		prev_ticks = now;
		unsimulated_ticks += delta;

		// Sleep if the app is running too fast.
		while (unsimulated_ticks < ticks_per_timestep) {
			int milliseconds = (int)((ticks_per_timestep - unsimulated_ticks) * (inv_freq * 1000));
			cf_sleep(milliseconds);

			// Record how much we slept by.
			now = cf_get_ticks();
			delta = now - prev_ticks;
			prev_ticks = now;
			unsimulated_ticks += delta;
		}

		// Record traditional delta time for this update.
		CF_DELTA_TIME = (float)((now - old_prev_ticks) * inv_freq);

		// Cap the number of steps we can take forward.
		// If the app is running slower than the target framerate (especially if just for a moment),
		// we can try and catch up. But, this must have an upper-bound to avoid the spiral of death.
		// The game will have lowered framerate if unsimulated_ticks gets capped here.
		unsimulated_ticks = min(unsimulated_ticks, ticks_per_timestep * max_updates);

		// Perform fixed-timestep updates in discrete chunks of time.
		// This will run input + gameplay loops, but not do any rendering.
		while (unsimulated_ticks >= ticks_per_timestep) {
			unsimulated_ticks -= ticks_per_timestep;
			if (pause_ticks > 0) {
				pause_ticks -= (int64_t)ticks_per_timestep;
				if (pause_ticks < 0) {
					uint64_t leftover = (uint64_t)(-pause_ticks);
					if (leftover + unsimulated_ticks > ticks_per_timestep) {
						unsimulated_ticks -= ticks_per_timestep - leftover;
						s_step(ticks_per_timestep);
						if (on_update) on_update();
					} else {
						unsimulated_ticks += leftover;
					}
				} else {
					continue;
				}
			} else {
				s_step(ticks_per_timestep);
				if (on_update) on_update();
			}
		}

		// Record the remaining time.
		// Intended for rendering code to interpolate between fixed timesteps.
		CF_DELTA_TIME_INTERPOLANT = (float)(((unsimulated_ticks * inv_freq) / CF_DELTA_TIME_FIXED));
	} else {
		// Variable timestep (default).
		uint64_t now = cf_get_ticks();
		uint64_t delta = now - prev_ticks;
		prev_ticks = now;
		CF_DELTA_TIME = (float)(delta * inv_freq);
		if (pause_ticks > 0) {
			pause_ticks -= (int64_t)delta;
			if (pause_ticks < 0) pause_ticks = 0;
			CF_PAUSE_TIME_LEFT = (float)(pause_ticks * inv_freq);
		} else {
			s_step(delta);
			if (on_update) on_update();
		}
	}
}

void cf_pause_for(float seconds)
{
	pause_ticks = max(pause_ticks, (uint64_t)(seconds * freq));
}

void cf_pause_for_ticks(uint64_t pause_ticks)
{
	::pause_ticks = max(::pause_ticks, pause_ticks);
}

bool cf_on_interval(float interval, float offset)
{
	int prev = (int)((CF_PREV_SECONDS - offset) / interval);
	int next = (int)((CF_SECONDS - offset) / interval);
	return prev < next;
}

bool cf_between_interval(float interval, float offset)
{
	return fmodf((float)CF_SECONDS - offset, interval * 2) >= interval;
}

bool cf_on_timestamp(double timestamp)
{
	float c = (float)CF_SECONDS - CF_DELTA_TIME;
	return CF_SECONDS >= timestamp && c < timestamp;
}

bool cf_is_paused()
{
	return pause_ticks > 0;
}

uint64_t cf_get_ticks()
{
	s_init();
	uint64_t now = SDL_GetPerformanceCounter();
	return now;
}

uint64_t cf_get_tick_frequency()
{
	return SDL_GetPerformanceFrequency();
}

void cf_sleep(int milliseconds)
{
	SDL_Delay((Uint32)milliseconds);
}

CF_Stopwatch cf_make_stopwatch()
{
	CF_Stopwatch result;
	result.start_time = cf_get_ticks();
	return result;
}

uint64_t cf_seconds(CF_Stopwatch stopwatch)
{
	return (uint64_t)((cf_get_ticks() - stopwatch.start_time) * inv_freq);
}

uint64_t cf_milliseconds(CF_Stopwatch stopwatch)
{
	return (uint64_t)(((cf_get_ticks() - stopwatch.start_time) * inv_freq) * 1000);
}

uint64_t cf_microseconds(CF_Stopwatch stopwatch)
{
	return (uint64_t)(((cf_get_ticks() - stopwatch.start_time) * inv_freq) * 1000000);
}
