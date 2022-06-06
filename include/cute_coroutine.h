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

#ifndef CUTE_COROUTINE_H
#define CUTE_COROUTINE_H

#include "cute_defines.h"
#include "cute_error.h"

struct cf_coroutine_t;
typedef void (cf_coroutine_fn)(cf_coroutine_t* co);

CUTE_API cf_coroutine_t* CUTE_CALL cf_coroutine_make(cf_coroutine_fn* fn, int stack_size = 0, void* udata = NULL);
CUTE_API void CUTE_CALL cf_coroutine_destroy(cf_coroutine_t* co);

enum cf_coroutine_state_t
{
	CF_COROUTINE_STATE_DEAD,
	CF_COROUTINE_STATE_ACTIVE_AND_RUNNING,
	CF_COROUTINE_STATE_ACTIVE_BUT_RESUMED_ANOTHER,
	CF_COROUTINE_STATE_SUSPENDED,
};

CUTE_API cf_error_t CUTE_CALL cf_coroutine_resume(cf_coroutine_t* co, float dt = 0);
CUTE_API float CUTE_CALL cf_coroutine_yield(cf_coroutine_t* co, cf_error_t* err = NULL);
CUTE_API cf_error_t CUTE_CALL cf_coroutine_wait(cf_coroutine_t* co, float seconds);
CUTE_API cf_coroutine_state_t CUTE_CALL cf_coroutine_state(cf_coroutine_t* co);
CUTE_API void* CUTE_CALL cf_coroutine_get_udata(cf_coroutine_t* co);

CUTE_API cf_error_t CUTE_CALL cf_coroutine_push(cf_coroutine_t* co, const void* data, size_t size);
CUTE_API cf_error_t CUTE_CALL cf_coroutine_pop(cf_coroutine_t* co, void* data, size_t size);
CUTE_API size_t CUTE_CALL cf_coroutine_bytes_pushed(cf_coroutine_t* co);
CUTE_API size_t CUTE_CALL cf_coroutine_space_remaining(cf_coroutine_t* co);

CUTE_API cf_coroutine_t* CUTE_CALL cf_coroutine_currently_running();

#ifdef CUTE_CPP

namespace cute
{

}

#endif // CUTE_CPP

#endif // CUTE_COROUTINE_H
