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

#include <cute_defines.h>
#include <cute_error.h>

#include <cute/cute_sync.h>

namespace cute
{

using mutex_t       = cute_mutex_t;
using cv_t          = cute_cv_t;
using atomic_int_t  = cute_atomic_int_t;
using sem_t         = cute_semaphore_t;
using thread_t      = cute_thread_t;
using thread_id_t   = cute_thread_id_t;
using thread_func_t = cute_thread_fn;
using rw_lock_t     = cute_rw_lock_t;
using threadpool_t  = cute_threadpool_t;

CUTE_API mutex_t CUTE_CALL mutex_create();
CUTE_API void CUTE_CALL mutex_destroy(mutex_t* mutex);
CUTE_API error_t CUTE_CALL mutex_lock(mutex_t* mutex);
CUTE_API error_t CUTE_CALL mutex_unlock(mutex_t* mutex);
CUTE_API bool CUTE_CALL mutex_trylock(mutex_t* mutex);

CUTE_API cv_t CUTE_CALL cv_create();
CUTE_API void CUTE_CALL cv_destroy(cv_t* cv);
CUTE_API error_t CUTE_CALL cv_wake_all(cv_t* cv);
CUTE_API error_t CUTE_CALL cv_wake_one(cv_t* cv);
CUTE_API error_t CUTE_CALL cv_wait(cv_t* cv, mutex_t* mutex);

CUTE_API sem_t CUTE_CALL sem_create(int initial_count);
CUTE_API void CUTE_CALL sem_destroy(sem_t* semaphore);
CUTE_API error_t CUTE_CALL sem_post(sem_t* semaphore);
CUTE_API error_t CUTE_CALL sem_try(sem_t* semaphore);
CUTE_API error_t CUTE_CALL sem_wait(sem_t* semaphore);
CUTE_API error_t CUTE_CALL sem_value(sem_t* semaphore);

CUTE_API thread_t* CUTE_CALL thread_create(thread_func_t func, const char* name, void* udata);
CUTE_API void CUTE_CALL thread_detach(thread_t* thread);
CUTE_API thread_id_t CUTE_CALL thread_get_id(thread_t* thread);
CUTE_API thread_id_t CUTE_CALL thread_id();
CUTE_API error_t CUTE_CALL thread_wait(thread_t* thread);

CUTE_API int CUTE_CALL core_count();
CUTE_API int CUTE_CALL cacheline_size();

CUTE_API atomic_int_t CUTE_CALL atomic_zero();
CUTE_API int CUTE_CALL atomic_add(atomic_int_t* atomic, int addend);
CUTE_API int CUTE_CALL atomic_set(atomic_int_t* atomic, int value);
CUTE_API int CUTE_CALL atomic_get(atomic_int_t* atomic);
CUTE_API error_t CUTE_CALL atomic_cas(atomic_int_t* atomic, int expected, int value);
CUTE_API void* CUTE_CALL atomic_ptr_set(void** atomic, void* value);
CUTE_API void* CUTE_CALL atomic_ptr_get(void** atomic);
CUTE_API error_t CUTE_CALL atomic_ptr_cas(void** atomic, void* expected, void* value);

CUTE_API rw_lock_t CUTE_CALL rw_lock_create();
CUTE_API void CUTE_CALL rw_lock_destroy(rw_lock_t* rw);
CUTE_API void CUTE_CALL read_lock(rw_lock_t* rw);
CUTE_API void CUTE_CALL read_unlock(rw_lock_t* rw);
CUTE_API void CUTE_CALL write_lock(rw_lock_t* rw);
CUTE_API void CUTE_CALL write_unlock(rw_lock_t* rw);

typedef void (CUTE_CALL task_fn)(void* param);

CUTE_API threadpool_t* CUTE_CALL threadpool_create(int thread_count, void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL threadpool_destroy(threadpool_t* pool);
CUTE_API void CUTE_CALL threadpool_add_task(threadpool_t* pool, task_fn* task, void* param);
CUTE_API void CUTE_CALL threadpool_kick_and_wait(threadpool_t* pool);
CUTE_API void CUTE_CALL threadpool_kick(threadpool_t* pool);

typedef void (CUTE_CALL promise_fn)(error_t status, void* param, void* promise_udata);

struct promise_t
{
	CUTE_INLINE promise_t () { }
	CUTE_INLINE promise_t (promise_fn* callback, void* promise_udata = NULL) : callback(callback), promise_udata(promise_udata) { }
	CUTE_INLINE void invoke(error_t status, void* param) { callback(status, param, promise_udata); }

	promise_fn* callback = NULL;
	void* promise_udata = NULL;
};

}

#endif // CUTE_CONCURRENCY_H
