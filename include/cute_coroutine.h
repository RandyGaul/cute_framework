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

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct cf_coroutine_t cf_coroutine_t;
typedef void (cf_coroutine_fn)(cf_coroutine_t* co);

CUTE_API cf_coroutine_t* CUTE_CALL cf_coroutine_make(cf_coroutine_fn* fn, int stack_size /*= 0*/, void* udata /*= NULL*/);
CUTE_API void CUTE_CALL cf_coroutine_destroy(cf_coroutine_t* co);

typedef enum cf_coroutine_state_t
{
	CF_COROUTINE_STATE_DEAD,
	CF_COROUTINE_STATE_ACTIVE_AND_RUNNING,
	CF_COROUTINE_STATE_ACTIVE_BUT_RESUMED_ANOTHER,
	CF_COROUTINE_STATE_SUSPENDED,
} cf_coroutine_state_t;

CUTE_API cf_error_t CUTE_CALL cf_coroutine_resume(cf_coroutine_t* co, float dt /*= 0*/);
CUTE_API float CUTE_CALL cf_coroutine_yield(cf_coroutine_t* co, cf_error_t* err /*= NULL*/);
CUTE_API cf_error_t CUTE_CALL cf_coroutine_wait(cf_coroutine_t* co, float seconds);
CUTE_API cf_coroutine_state_t CUTE_CALL cf_coroutine_state(cf_coroutine_t* co);
CUTE_API void* CUTE_CALL cf_coroutine_get_udata(cf_coroutine_t* co);

CUTE_API cf_error_t CUTE_CALL cf_coroutine_push(cf_coroutine_t* co, const void* data, size_t size);
CUTE_API cf_error_t CUTE_CALL cf_coroutine_pop(cf_coroutine_t* co, void* data, size_t size);
CUTE_API size_t CUTE_CALL cf_coroutine_bytes_pushed(cf_coroutine_t* co);
CUTE_API size_t CUTE_CALL cf_coroutine_space_remaining(cf_coroutine_t* co);

CUTE_API cf_coroutine_t* CUTE_CALL cf_coroutine_currently_running();

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using coroutine_t = cf_coroutine_t;
using coroutine_fn = cf_coroutine_fn;
using coroutine_state_t = cf_coroutine_state_t;

CUTE_INLINE coroutine_t* coroutine_make(coroutine_fn* fn, int stack_size = 0, void* udata = NULL) { return cf_coroutine_make(fn, stack_size, udata); }
CUTE_INLINE void coroutine_destroy(coroutine_t* co) { cf_coroutine_destroy(co); }
	 
CUTE_INLINE error_t coroutine_resume(coroutine_t* co, float dt = 0) { return cf_coroutine_resume(co, dt); }
CUTE_INLINE float coroutine_yield(coroutine_t* co, cf_error_t* err = NULL) { return cf_coroutine_yield(co, err); }
CUTE_INLINE error_t coroutine_wait(coroutine_t* co, float seconds) { return cf_coroutine_wait(co, seconds); }
CUTE_INLINE coroutine_state_t coroutine_state(coroutine_t* co) { return cf_coroutine_state(co); }
CUTE_INLINE void* coroutine_get_udata(coroutine_t* co) { return cf_coroutine_get_udata(co); }
	 
CUTE_INLINE error_t coroutine_push(coroutine_t* co, const void* data, size_t size) { return cf_coroutine_push(co, data, size); }
CUTE_INLINE error_t coroutine_pop(coroutine_t* co, void* data, size_t size) { return cf_coroutine_pop(co, data, size); }
CUTE_INLINE size_t coroutine_bytes_pushed(coroutine_t* co) { return cf_coroutine_bytes_pushed(co); }
CUTE_INLINE size_t coroutine_space_remaining(coroutine_t* co) { return cf_coroutine_space_remaining(co); }
	 
CUTE_INLINE coroutine_t* coroutine_currently_running() { return cf_coroutine_currently_running(); }

}

#endif // CUTE_CPP

#endif // CUTE_COROUTINE_H
