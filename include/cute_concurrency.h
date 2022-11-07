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
#include "cute_result.h"

#include "cute/cute_sync.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef cute_mutex_t cf_mutex_t;
typedef cute_cv_t cf_cv_t;
typedef cute_atomic_int_t cf_atomic_int_t;
typedef cute_semaphore_t cf_semaphore_t;
typedef cute_thread_t cf_thread_t;
typedef cute_thread_id_t cf_thread_id_t;
typedef cute_thread_fn cf_thread_func_t;
typedef cute_rw_lock_t cf_rw_lock_t;
typedef cute_threadpool_t cf_threadpool_t;

CUTE_API cf_mutex_t CUTE_CALL cf_make_mutex();
CUTE_API void CUTE_CALL cf_destroy_mutex(cf_mutex_t* mutex);
CUTE_API cf_result_t CUTE_CALL cf_mutex_lock(cf_mutex_t* mutex);
CUTE_API cf_result_t CUTE_CALL cf_mutex_unlock(cf_mutex_t* mutex);
CUTE_API bool CUTE_CALL cf_mutex_trylock(cf_mutex_t* mutex);

CUTE_API cf_cv_t CUTE_CALL cf_make_cv();
CUTE_API void CUTE_CALL cf_destroy_cv(cf_cv_t* cv);
CUTE_API cf_result_t CUTE_CALL cf_cv_wake_all(cf_cv_t* cv);
CUTE_API cf_result_t CUTE_CALL cf_cv_wake_one(cf_cv_t* cv);
CUTE_API cf_result_t CUTE_CALL cf_cv_wait(cf_cv_t* cv, cf_mutex_t* mutex);

CUTE_API cf_semaphore_t CUTE_CALL cf_make_sem(int initial_count);
CUTE_API void CUTE_CALL cf_destroy_sem(cf_semaphore_t* semaphore);
CUTE_API cf_result_t CUTE_CALL cf_sem_post(cf_semaphore_t* semaphore);
CUTE_API cf_result_t CUTE_CALL cf_sem_try(cf_semaphore_t* semaphore);
CUTE_API cf_result_t CUTE_CALL cf_sem_wait(cf_semaphore_t* semaphore);
CUTE_API cf_result_t CUTE_CALL cf_sem_value(cf_semaphore_t* semaphore);

CUTE_API cf_thread_t* CUTE_CALL cf_thread_create(cf_thread_func_t func, const char* name, void* udata);
CUTE_API void CUTE_CALL cf_thread_detach(cf_thread_t* thread);
CUTE_API cf_thread_id_t CUTE_CALL cf_thread_get_id(cf_thread_t* thread);
CUTE_API cf_thread_id_t CUTE_CALL cf_thread_id();
CUTE_API cf_result_t CUTE_CALL cf_thread_wait(cf_thread_t* thread);

CUTE_API int CUTE_CALL cf_core_count();
CUTE_API int CUTE_CALL cf_cacheline_size();

CUTE_API cf_atomic_int_t CUTE_CALL cf_atomic_zero();
CUTE_API int CUTE_CALL cf_atomic_add(cf_atomic_int_t* atomic, int addend);
CUTE_API int CUTE_CALL cf_atomic_set(cf_atomic_int_t* atomic, int value);
CUTE_API int CUTE_CALL cf_atomic_get(cf_atomic_int_t* atomic);
CUTE_API cf_result_t CUTE_CALL cf_atomic_cas(cf_atomic_int_t* atomic, int expected, int value);
CUTE_API void* CUTE_CALL cf_atomic_ptr_set(void** atomic, void* value);
CUTE_API void* CUTE_CALL cf_atomic_ptr_get(void** atomic);
CUTE_API cf_result_t CUTE_CALL cf_atomic_ptr_cas(void** atomic, void* expected, void* value);

CUTE_API cf_rw_lock_t CUTE_CALL cf_make_rw_lock();
CUTE_API void CUTE_CALL cf_destroy_rw_lock(cf_rw_lock_t* rw);
CUTE_API void CUTE_CALL cf_read_lock(cf_rw_lock_t* rw);
CUTE_API void CUTE_CALL cf_read_unlock(cf_rw_lock_t* rw);
CUTE_API void CUTE_CALL cf_write_lock(cf_rw_lock_t* rw);
CUTE_API void CUTE_CALL cf_write_unlock(cf_rw_lock_t* rw);

typedef void (CUTE_CALL cf_task_fn)(void* param);

CUTE_API cf_threadpool_t* CUTE_CALL cf_make_threadpool(int thread_count);
CUTE_API void CUTE_CALL cf_destroy_threadpool(cf_threadpool_t* pool);
CUTE_API void CUTE_CALL cf_threadpool_add_task(cf_threadpool_t* pool, cf_task_fn* task, void* param);
CUTE_API void CUTE_CALL cf_threadpool_kick_and_wait(cf_threadpool_t* pool);
CUTE_API void CUTE_CALL cf_threadpool_kick(cf_threadpool_t* pool);

typedef void (CUTE_CALL cf_promise_fn)(cf_result_t status, void* param, void* promise_udata);

typedef struct cf_promise_t
{
	#ifdef CUTE_CPP
	CUTE_INLINE cf_promise_t() : callback(NULL), promise_udata(NULL) {}
	CUTE_INLINE cf_promise_t(cf_promise_fn* callback, void* promise_udata = NULL) : callback(callback), promise_udata(promise_udata) {}
	CUTE_INLINE void invoke(cf_result_t status, void* param) { callback(status, param, promise_udata); }
	#endif // CUTE_CPP

	cf_promise_fn* callback; /*= NULL;*/
	void* promise_udata; /*= NULL;*/
} cf_promise_t;

CUTE_INLINE void cf_promise_invoke(cf_promise_t* promise, cf_result_t status, void* param) { promise->callback(status, param, promise->promise_udata); }

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using mutex_t = cf_mutex_t;
using cv_t = cf_cv_t;
using atomic_int_t = cf_atomic_int_t;
using semaphore_t = cf_semaphore_t;
using thread_t = cf_thread_t;
using thread_id_t = cf_thread_id_t;
using thread_func_t = cf_thread_func_t;
using rw_lock_t = cf_rw_lock_t;
using threadpool_t = cf_threadpool_t;
using promise_t = cf_promise_t;

using task_fn = cf_task_fn;
using promise_fn = cf_promise_fn;

CUTE_INLINE mutex_t make_mutex() { return cf_make_mutex(); }
CUTE_INLINE void destroy_mutex(mutex_t* mutex) { cf_destroy_mutex(mutex); }
CUTE_INLINE result_t mutex_lock(mutex_t* mutex) { return cf_mutex_lock(mutex); }
CUTE_INLINE result_t mutex_unlock(mutex_t* mutex) { return cf_mutex_unlock(mutex); }
CUTE_INLINE bool mutex_trylock(mutex_t* mutex) { return cf_mutex_trylock(mutex); }

CUTE_INLINE cv_t make_cv() { return cf_make_cv(); }
CUTE_INLINE void destroy_cv(cv_t* cv) { cf_destroy_cv(cv); }
CUTE_INLINE result_t cv_wake_all(cv_t* cv) { return cf_cv_wake_all(cv); }
CUTE_INLINE result_t cv_wake_one(cv_t* cv) { return cf_cv_wake_one(cv); }
CUTE_INLINE result_t cv_wait(cv_t* cv, mutex_t* mutex) { return cf_cv_wait(cv, mutex); }

CUTE_INLINE semaphore_t make_sem(int initial_count) { return cf_make_sem(initial_count); }
CUTE_INLINE void destroy_sem(semaphore_t* semaphore) { cf_destroy_sem(semaphore); }
CUTE_INLINE result_t sem_post(semaphore_t* semaphore) { return cf_sem_post(semaphore); }
CUTE_INLINE result_t sem_try(semaphore_t* semaphore) { return cf_sem_try(semaphore); }
CUTE_INLINE result_t sem_wait(semaphore_t* semaphore) { return cf_sem_wait(semaphore); }
CUTE_INLINE result_t sem_value(semaphore_t* semaphore) { return cf_sem_value(semaphore); }

CUTE_INLINE thread_t* thread_create(thread_func_t func, const char* name, void* udata) { return cf_thread_create(func, name, udata); }
CUTE_INLINE void thread_detach(thread_t* thread) { cf_thread_detach(thread); }
CUTE_INLINE thread_id_t thread_get_id(thread_t* thread) { return cf_thread_get_id(thread); }
CUTE_INLINE thread_id_t thread_id() { return cf_thread_id(); }
CUTE_INLINE result_t thread_wait(thread_t* thread) { return cf_thread_wait(thread); }

CUTE_INLINE int core_count() { return cf_core_count(); }
CUTE_INLINE int cacheline_size() { return cf_cacheline_size(); }

CUTE_INLINE atomic_int_t atomic_zero() { return cf_atomic_zero(); }
CUTE_INLINE int atomic_add(atomic_int_t* atomic, int addend) { return cf_atomic_add(atomic, addend); }
CUTE_INLINE int atomic_set(atomic_int_t* atomic, int value) { return cf_atomic_set(atomic, value); }
CUTE_INLINE int atomic_get(atomic_int_t* atomic) { return cf_atomic_get(atomic); }
CUTE_INLINE result_t atomic_cas(atomic_int_t* atomic, int expected, int value) { return cf_atomic_cas(atomic, expected, value); }
CUTE_INLINE void* atomic_ptr_set(void** atomic, void* value) { return cf_atomic_ptr_set(atomic, value); }
CUTE_INLINE void* atomic_ptr_get(void** atomic) { return cf_atomic_ptr_get(atomic); }
CUTE_INLINE result_t atomic_ptr_cas(void** atomic, void* expected, void* value) { return cf_atomic_ptr_cas(atomic, expected, value); }

CUTE_INLINE rw_lock_t make_rw_lock() { return cf_make_rw_lock(); }
CUTE_INLINE void destroy_rw_lock(rw_lock_t* rw) { cf_destroy_rw_lock(rw); }
CUTE_INLINE void read_lock(rw_lock_t* rw) { cf_read_lock(rw); }
CUTE_INLINE void read_unlock(rw_lock_t* rw) { cf_read_unlock(rw); }
CUTE_INLINE void write_lock(rw_lock_t* rw) { cf_write_lock(rw); }
CUTE_INLINE void write_unlock(rw_lock_t* rw) { cf_write_unlock(rw); }

CUTE_INLINE threadpool_t* make_threadpool(int thread_count) { return cf_make_threadpool(thread_count); }
CUTE_INLINE void destroy_threadpool(threadpool_t* pool) { return cf_destroy_threadpool(pool); }
CUTE_INLINE void threadpool_add_task(threadpool_t* pool, task_fn* task, void* param) { return cf_threadpool_add_task(pool, task, param); }
CUTE_INLINE void threadpool_kick_and_wait(threadpool_t* pool) { return cf_threadpool_kick_and_wait(pool); }
CUTE_INLINE void threadpool_kick(threadpool_t* pool) { return cf_threadpool_kick(pool); }

}

#endif // CUTE_CPP

#endif // CUTE_CONCURRENCY_H
