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

#include <cute_defines.h>

namespace cute
{

/**
 * Calculates the time, in seconds, since the last time this function was called.
 * No special care is taken to handle multi-threading (this function uses static memory).
 * Returns 0 on the first call.
 * 
 * For more fine-grained measuring of time, try using `timer_t`.
 */
CUTE_API float CUTE_CALL calc_dt();

struct timer_t
{
	double inv_freq;
	uint64_t prev;
};

/**
 * Initializes a new `timer_t` on the stack.
 */
CUTE_API timer_t CUTE_CALL timer_init();

/**
 * Returns the time elapsed since the last call to `timer_dt` was made.
 */
CUTE_API float CUTE_CALL timer_dt(timer_t* timer);

/**
 * Returns the time elapsed since the last call to `timer_dt` was made. Use this function
 * to repeatedly measure the time since the last `timer_dt` call.
 */
CUTE_API float CUTE_CALL timer_elapsed(timer_t* timer);

}

#endif // CUTE_TIMER_H
