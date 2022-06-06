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

#ifndef CUTE_CONCURRENCY_H
#define CUTE_CONCURRENCY_H

#include "cute_defines.h"
#include "cute_error.h"

#include "cute/cute_sync.h"


using cf_mutex_t       = cute_mutex_t;
using cf_cv_t          = cute_cv_t;
using cf_atomic_int_t  = cute_atomic_int_t;
using cf_semaphore_t   = cute_semaphore_t;
using cf_thread_t      = cute_thread_t;
using cf_thread_id_t   = cute_thread_id_t;
using cf_thread_func_t = cute_thread_fn;
using cf_rw_lock_t     = cute_rw_lock_t;
using cf_cf_threadpool_t  = cute_threadpool_t;

CUTE_API cf_mutex_t CUTE_CALL cf_mutex_create();
CUTE_API void CUTE_CALL cf_mutex_destroy(cf_mutex_t* mutex);
CUTE_API cf_error_t CUTE_CALL cf_mutex_lock(cf_mutex_t* mutex);
CUTE_API cf_error_t CUTE_CALL cf_mutex_unlock(cf_mutex_t* mutex);
CUTE_API bool CUTE_CALL cf_mutex_trylock(cf_mutex_t* mutex);

CUTE_API cf_cv_t CUTE_CALL cf_cv_create();
CUTE_API void CUTE_CALL cf_cv_destroy(cf_cv_t* cv);
CUTE_API cf_error_t CUTE_CALL cf_cv_wake_all(cf_cv_t* cv);
CUTE_API cf_error_t CUTE_CALL cf_cv_wake_one(cf_cv_t* cv);
CUTE_API cf_error_t CUTE_CALL cf_cv_wait(cf_cv_t* cv, cf_mutex_t* mutex);

CUTE_API cf_semaphore_t CUTE_CALL cf_sem_create(int initial_count);
CUTE_API void CUTE_CALL cf_sem_destroy(cf_semaphore_t* semaphore);
CUTE_API cf_error_t CUTE_CALL cf_sem_post(cf_semaphore_t* semaphore);
CUTE_API cf_error_t CUTE_CALL cf_sem_try(cf_semaphore_t* semaphore);
CUTE_API cf_error_t CUTE_CALL cf_sem_wait(cf_semaphore_t* semaphore);
CUTE_API cf_error_t CUTE_CALL cf_sem_value(cf_semaphore_t* semaphore);

CUTE_API cf_thread_t* CUTE_CALL cf_thread_create(cf_thread_func_t func, const char* name, void* udata);
CUTE_API void CUTE_CALL cf_thread_detach(cf_thread_t* thread);
CUTE_API cf_thread_id_t CUTE_CALL cf_thread_get_id(cf_thread_t* thread);
CUTE_API cf_thread_id_t CUTE_CALL cf_thread_id();
CUTE_API cf_error_t CUTE_CALL cf_thread_wait(cf_thread_t* thread);

CUTE_API int CUTE_CALL cf_core_count();
CUTE_API int CUTE_CALL cf_cacheline_size();

CUTE_API cf_atomic_int_t CUTE_CALL cf_atomic_zero();
CUTE_API int CUTE_CALL cf_atomic_add(cf_atomic_int_t* atomic, int addend);
CUTE_API int CUTE_CALL cf_atomic_set(cf_atomic_int_t* atomic, int value);
CUTE_API int CUTE_CALL cf_atomic_get(cf_atomic_int_t* atomic);
CUTE_API cf_error_t CUTE_CALL cf_atomic_cas(cf_atomic_int_t* atomic, int expected, int value);
CUTE_API void* CUTE_CALL cf_atomic_ptr_set(void** atomic, void* value);
CUTE_API void* CUTE_CALL cf_atomic_ptr_get(void** atomic);
CUTE_API cf_error_t CUTE_CALL cf_atomic_ptr_cas(void** atomic, void* expected, void* value);

CUTE_API cf_rw_lock_t CUTE_CALL cf_rw_lock_create();
CUTE_API void CUTE_CALL cf_rw_lock_destroy(cf_rw_lock_t* rw);
CUTE_API void CUTE_CALL cf_read_lock(cf_rw_lock_t* rw);
CUTE_API void CUTE_CALL cf_read_unlock(cf_rw_lock_t* rw);
CUTE_API void CUTE_CALL cf_write_lock(cf_rw_lock_t* rw);
CUTE_API void CUTE_CALL cf_write_unlock(cf_rw_lock_t* rw);

typedef void (CUTE_CALL cf_task_fn)(void* param);

CUTE_API cf_cf_threadpool_t* CUTE_CALL cf_threadpool_create(int thread_count, void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL cf_threadpool_destroy(cf_cf_threadpool_t* pool);
CUTE_API void CUTE_CALL cf_threadpool_add_task(cf_cf_threadpool_t* pool, cf_task_fn* task, void* param);
CUTE_API void CUTE_CALL cf_threadpool_kick_and_wait(cf_cf_threadpool_t* pool);
CUTE_API void CUTE_CALL cf_threadpool_kick(cf_cf_threadpool_t* pool);

typedef void (CUTE_CALL cf_promise_fn)(cf_error_t status, void* param, void* promise_udata);

struct cf_promise_t
{
	CUTE_INLINE cf_promise_t () { }
	CUTE_INLINE cf_promise_t (cf_promise_fn* callback, void* promise_udata = NULL) : callback(callback), promise_udata(promise_udata) { }
	CUTE_INLINE void invoke(cf_error_t status, void* param) { callback(status, param, promise_udata); }

	cf_promise_fn* callback = NULL;
	void* promise_udata = NULL;
};

#ifdef CUTE_CPP

namespace cute
{

}

#endif // CUTE_CPP

#endif // CUTE_CONCURRENCY_H
