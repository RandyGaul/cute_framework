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

#include <cute_log.h>

#include <stdarg.h>
#include <stdio.h>

namespace cute
{

static int s_log_level = CUTE_LOG_LEVEL_ERROR;
static log_fn* s_log_fn = NULL;
static void* s_log_udata = NULL;

void log_set_level(int level)
{
	s_log_level = level;
}

void log_set_function(log_fn* fn, void* udata)
{
	s_log_fn = fn;
	s_log_udata = udata;
}

void log(int level, const char* fmt, ...)
{
	if (s_log_level > level) return;

	va_list args;
	va_start(args, fmt);

	if (s_log_fn) {
		s_log_fn(s_log_udata, level, fmt, args);
	} else {
		vfprintf(stderr, fmt, args);
		fprintf(stderr, "\n\t");
	}

	va_end(args);
}

}
