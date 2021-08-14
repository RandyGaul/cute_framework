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

#include <cute_timer.h>

#include <SDL.h>

namespace cute
{

float calc_dt()
{
	static int first = 1;
	static double inv_freq;
	static uint64_t prev;

	uint64_t now = SDL_GetPerformanceCounter();

	if (first) {
		first = 0;
		prev = now;
		inv_freq = 1.0 / (double)SDL_GetPerformanceFrequency();
	}

	float dt = (float)((double)(now - prev) * inv_freq);
	prev = now;
	return dt;
}

timer_t timer_init()
{
	timer_t timer;
	timer.prev = SDL_GetPerformanceCounter();
	timer.inv_freq = 1.0 / (double)SDL_GetPerformanceFrequency();
	return timer;
}

float timer_dt(timer_t* timer)
{
	uint64_t now = SDL_GetPerformanceCounter();
	float dt = (float)((double)(now - timer->prev) * timer->inv_freq);
	timer->prev = now;
	return dt;
}

float timer_elapsed(timer_t* timer)
{
	uint64_t now = SDL_GetPerformanceCounter();
	float elapsed = (float)((double)(now - timer->prev) * timer->inv_freq);
	return elapsed;
}

}
