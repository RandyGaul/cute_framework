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

#ifndef CUTE_ERROR_H
#define CUTE_ERROR_H

#include "cute_defines.h"


#define CUTE_ERROR_SUCCESS (0)
#define CUTE_ERROR_FAILURE (-1)

struct cf_error_t
{
	int code;
	const char *details;

	#ifdef CUTE_CPP
	CUTE_INLINE bool is_error() const;
	#endif // CUTE_CPP

};

CUTE_INLINE bool cf_is_error(const cf_error_t* error) { return error->code == CUTE_ERROR_FAILURE; }

CUTE_INLINE cf_error_t cf_error_make(int code, const char *details) { cf_error_t error; error.code = code; error.details = details; return error; }
CUTE_INLINE cf_error_t cf_error_failure(const char *details) { cf_error_t error; error.code = CUTE_ERROR_FAILURE; error.details = details; return error; }
CUTE_INLINE cf_error_t cf_error_success() { cf_error_t error; error.code = CUTE_ERROR_SUCCESS; error.details = NULL; return error; }

#define CUTE_RETURN_IF_ERROR(x) do { cf_error_t err = (x); if (cf_is_error(&err)) return err; } while (0)

#ifdef CUTE_CPP

CUTE_INLINE bool cf_error_t::is_error() const { return cf_is_error(this); }

namespace cute
{
using error_t = cf_error_t;

}

#endif // CUTE_CPP

#endif // CUTE_ERROR_H
