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
#include "cute_result.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct CF_Coroutine CF_Coroutine;
typedef void (CF_CoroutineFn)(CF_Coroutine* co);

CUTE_API CF_Coroutine* CUTE_CALL cf_make_coroutine(CF_CoroutineFn* fn, int stack_size /*= 0*/, void* udata /*= NULL*/);
CUTE_API void CUTE_CALL cf_destroy_coroutine(CF_Coroutine* co);

#define CF_COROUTINE_STATE_DEFS \
	CF_ENUM(COROUTINE_STATE_DEAD, 0) \
	CF_ENUM(COROUTINE_STATE_ACTIVE_AND_RUNNING, 1) \
	CF_ENUM(COROUTINE_STATE_ACTIVE_BUT_RESUMED_ANOTHER, 2) \
	CF_ENUM(COROUTINE_STATE_SUSPENDED, 3) \

typedef enum CF_CoroutineState
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_COROUTINE_STATE_DEFS
	#undef CF_ENUM
} CF_CoroutineState;

CUTE_API CF_Result CUTE_CALL cf_coroutine_resume(CF_Coroutine* co, float dt /*= 0*/);
CUTE_API float CUTE_CALL cf_coroutine_yield(CF_Coroutine* co, CF_Result* err /*= NULL*/);
CUTE_API CF_Result CUTE_CALL cf_coroutine_wait(CF_Coroutine* co, float seconds);
CUTE_API CF_CoroutineState CUTE_CALL cf_coroutine_state(CF_Coroutine* co);
CUTE_API void* CUTE_CALL cf_coroutine_get_udata(CF_Coroutine* co);

CUTE_API CF_Result CUTE_CALL cf_coroutine_push(CF_Coroutine* co, const void* data, size_t size);
CUTE_API CF_Result CUTE_CALL cf_coroutine_pop(CF_Coroutine* co, void* data, size_t size);
CUTE_API size_t CUTE_CALL cf_coroutine_bytes_pushed(CF_Coroutine* co);
CUTE_API size_t CUTE_CALL cf_coroutine_space_remaining(CF_Coroutine* co);

CUTE_API CF_Coroutine* CUTE_CALL cf_coroutine_currently_running();

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using Coroutine = CF_Coroutine;
using CoroutineFn = CF_CoroutineFn;

enum CoroutineState : int
{
	#define CF_ENUM(K, V) K = V,
	CF_COROUTINE_STATE_DEFS
	#undef CF_ENUM
};

CUTE_INLINE Coroutine* make_coroutine(CoroutineFn* fn, int stack_size = 0, void* udata = NULL) { return cf_make_coroutine(fn, stack_size, udata); }
CUTE_INLINE void destroy_coroutine(Coroutine* co) { cf_destroy_coroutine(co); }
	 
CUTE_INLINE Result coroutine_resume(Coroutine* co, float dt = 0) { return cf_coroutine_resume(co, dt); }
CUTE_INLINE float coroutine_yield(Coroutine* co, CF_Result* err = NULL) { return cf_coroutine_yield(co, err); }
CUTE_INLINE Result coroutine_wait(Coroutine* co, float seconds) { return cf_coroutine_wait(co, seconds); }
CUTE_INLINE CoroutineState coroutine_state(Coroutine* co) { return (CoroutineState)cf_coroutine_state(co); }
CUTE_INLINE void* coroutine_get_udata(Coroutine* co) { return cf_coroutine_get_udata(co); }
	 
CUTE_INLINE Result coroutine_push(Coroutine* co, const void* data, size_t size) { return cf_coroutine_push(co, data, size); }
CUTE_INLINE Result coroutine_pop(Coroutine* co, void* data, size_t size) { return cf_coroutine_pop(co, data, size); }
CUTE_INLINE size_t coroutine_bytes_pushed(Coroutine* co) { return cf_coroutine_bytes_pushed(co); }
CUTE_INLINE size_t coroutine_space_remaining(Coroutine* co) { return cf_coroutine_space_remaining(co); }
	 
CUTE_INLINE Coroutine* coroutine_currently_running() { return cf_coroutine_currently_running(); }

}

#endif // CUTE_CPP

#endif // CUTE_COROUTINE_H
