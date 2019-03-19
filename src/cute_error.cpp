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

#include <cute_error.h>

namespace cute
{

static const char* s_error;
static error_handler_fn* s_handler;
static void* s_udata;

const char* error_get()
{
	return s_error;
}

void error_set(const char* error_string)
{
	s_error = error_string;
	if (s_handler) {
		s_handler(error_string, s_udata);
	}
}

void error_handler_set(error_handler_fn* handler, void* udata)
{
	s_handler = handler;
	s_udata = udata;
}

}
