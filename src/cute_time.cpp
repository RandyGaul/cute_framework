/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_time.h>
#include <cute_math.h>
#include <cute_c_runtime.h>

#include <internal/cute_app_internal.h>

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
int target_framerate = -1;

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

void cf_set_target_framerate(int frames_per_second)
{
	target_framerate = frames_per_second;
}

static void s_step(uint64_t delta)
{
	CF_PREV_TICKS = CF_TICKS;
	CF_TICKS += delta;
	CF_PREV_SECONDS = CF_SECONDS;
	CF_SECONDS = CF_TICKS * inv_freq;
}

void cf_set_update_udata(void* udata)
{
	app->update_udata = udata;
}

// Originally from: https://blog.bearcats.nl/accurate-sleep-function/
// Modified slightly to avoid pulling in std::chrono, and fix some bugs.
static void s_precise_sleep(double seconds)
{
	static double estimate = 5e-3;
	static double mean = 5e-3;
	static double m2 = 0;
	static int64_t count = 1;

	// Sleep for 1ms while there's some level of certainty it won't overshoot.
	// Then break from this loop and perform a spin-lock for the remaining time.
	while (seconds > estimate) {
		uint64_t start = cf_get_ticks();
		cf_sleep(1);
		uint64_t end = cf_get_ticks();
	
		double observed = (end - start) * inv_freq;
		seconds -= observed;
	
		++count;
		double delta = observed - mean;
		mean += delta / count;
		m2 += delta * (observed - mean);
		double stddev = sqrt(m2 / (count - 1));
		estimate = mean + stddev;
	
		if (count > 100000) {
			estimate = 5e-3;
			mean = 5e-3;
			m2 = 0;
			count = 0;
		}
	}
	
	if (seconds < 0) {
		// Overshoot.
		return;
	}

	// Spin lock.
	uint64_t start = cf_get_ticks();
	while (1) {
		uint64_t now = cf_get_ticks();
		uint64_t elapsed = now - start;
		double elapsed_seconds = elapsed * inv_freq;
		if (elapsed_seconds > seconds) {
			break;
		}
	}
}

static void s_fps_limit()
{
	if (target_framerate != -1) {
		double seconds = 1.0 / target_framerate;
		uint64_t now = cf_get_ticks();
		uint64_t delta = now - prev_ticks;
		s_precise_sleep(seconds - delta * inv_freq);
	}
}

void cf_update_time(CF_OnUpdateFn* on_update)
{
	if (ticks_per_timestep) {
		// Fixed timestep (opt-in only).

		// Sleep if the app is running too fast.
		s_fps_limit();

		// Accumulate unsimulated time.
		uint64_t now = cf_get_ticks();
		uint64_t old_prev_ticks = prev_ticks;
		uint64_t delta = now - prev_ticks;
		prev_ticks = now;
		unsimulated_ticks += delta;

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
						if (on_update) on_update(app->update_udata);
					} else {
						unsimulated_ticks += leftover;
					}
				}
			} else {
				s_step(ticks_per_timestep);
				if (on_update) on_update(app->update_udata);
			}
		}

		// Record the remaining time.
		// Intended for rendering code to interpolate between fixed timesteps.
		CF_DELTA_TIME_INTERPOLANT = (float)(((unsimulated_ticks * inv_freq) / CF_DELTA_TIME_FIXED));
	} else {
		// Variable timestep (default).

		// Sleep if the app is running too fast.
		s_fps_limit();

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
			if (on_update) on_update(app->update_udata);
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
	return CF_FMODF((float)CF_SECONDS - offset, interval * 2) >= interval;
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

double cf_stopwatch_seconds(CF_Stopwatch stopwatch)
{
	return (cf_get_ticks() - stopwatch.start_time) * inv_freq;
}

double cf_stopwatch_milliseconds(CF_Stopwatch stopwatch)
{
	return ((cf_get_ticks() - stopwatch.start_time) * inv_freq) * 1000.0;
}

double cf_stopwatch_microseconds(CF_Stopwatch stopwatch)
{
	return ((cf_get_ticks() - stopwatch.start_time) * inv_freq) * 1000000.0;
}
