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

#include <cute_concurrency.h>
#include <cute_alloc.h>

#include <SDL.h>

#define CUTE_SYNC_IMPLEMENTATION
#ifdef CUTE_WINDOWS
#   define CUTE_SYNC_WINDOWS
#else
#   define CUTE_SYNC_SDL
#endif
#define CUTE_THREAD_ALLOC CUTE_ALLOC
#define CUTE_THREAD_FREE CUTE_FREE
#include <cute/cute_sync.h>

cf_mutex_t cf_make_mutex()
{
	return cute_mutex_create();
}

void cf_destroy_mutex(cf_mutex_t* mutex)
{
	cute_mutex_destroy(mutex);
}

cf_result_t cf_mutex_lock(cf_mutex_t* mutex)
{
	return cf_result_make(cute_lock(mutex), NULL);
}

cf_result_t cf_mutex_unlock(cf_mutex_t* mutex)
{
	return cf_result_make(cute_unlock(mutex), NULL);
}

bool cf_mutex_trylock(cf_mutex_t* mutex)
{
	return !!cute_trylock(mutex);
}

cf_cv_t cf_make_cv()
{
	return cute_cv_create();
}

void cf_destroy_cv(cf_cv_t* cv)
{
	cute_cv_destroy(cv);
}

cf_result_t cf_cv_wake_all(cf_cv_t* cv)
{
	return cf_result_make(cute_cv_wake_all(cv), NULL);
}

cf_result_t cf_cv_wake_one(cf_cv_t* cv)
{
	return cf_result_make(cute_cv_wake_one(cv), NULL);
}

cf_result_t cf_cv_wait(cf_cv_t* cv, cf_mutex_t* mutex)
{
	return cf_result_make(cute_cv_wait(cv, mutex), NULL);
}

cf_semaphore_t cf_make_sem(int initial_count)
{
	return cute_semaphore_create(initial_count);
}

void cf_destroy_sem(cf_semaphore_t* semaphore)
{
	cute_semaphore_destroy(semaphore);
}

cf_result_t cf_sem_post(cf_semaphore_t* semaphore)
{
	return cf_result_make(cute_semaphore_post(semaphore), NULL);
}

cf_result_t cf_sem_try(cf_semaphore_t* semaphore)
{
	return cf_result_make(cute_semaphore_try(semaphore), NULL);
}

cf_result_t cf_sem_wait(cf_semaphore_t* semaphore)
{
	return cf_result_make(cute_semaphore_wait(semaphore), NULL);
}

cf_result_t cf_sem_value(cf_semaphore_t* semaphore)
{
	return cf_result_make(cute_semaphore_value(semaphore), NULL);
}

cf_thread_t* cf_thread_create(cf_thread_func_t func, const char* name, void* udata)
{
	return cute_thread_create(func, name, udata);
}

void cf_thread_detach(cf_thread_t* thread)
{
	cute_thread_detach(thread);
}

cf_thread_id_t cf_thread_get_id(cf_thread_t* thread)
{
	return cute_thread_get_id(thread);
}

cf_thread_id_t cf_thread_id()
{
	return cute_thread_id();
}

cf_result_t cf_thread_wait(cf_thread_t* thread)
{
	return cf_result_make(cute_thread_wait(thread), NULL);
}

int cf_core_count()
{
	return cute_core_count();
}

int cf_cacheline_size()
{
	return cute_cacheline_size();
}

cf_atomic_int_t cf_atomic_zero()
{
	cf_atomic_int_t result;
	result.i = 0;
	return result;
}

int cf_atomic_add(cf_atomic_int_t* atomic, int addend)
{
	return cute_atomic_add(atomic, addend);
}

int cf_atomic_set(cf_atomic_int_t* atomic, int value)
{
	return cute_atomic_set(atomic, value);
}

int cf_atomic_get(cf_atomic_int_t* atomic)
{
	return cute_atomic_get(atomic);
}

cf_result_t cf_atomic_cas(cf_atomic_int_t* atomic, int expected, int value)
{
	return cf_result_make(cute_atomic_cas(atomic, expected, value), NULL);
}

void* cf_atomic_ptr_set(void** atomic, void* value)
{
	return cute_atomic_ptr_set(atomic, value);
}

void* cf_atomic_ptr_get(void** atomic)
{
	return cute_atomic_ptr_get(atomic);
}

cf_result_t cf_atomic_ptr_cas(void** atomic, void* expected, void* value)
{
	return cf_result_make(cute_atomic_ptr_cas(atomic, expected, value), NULL);
}

cf_rw_lock_t cf_make_rw_lock()
{
	return cute_rw_lock_create();
}

void cf_destroy_rw_lock(cf_rw_lock_t* rw)
{
	if (rw) cute_rw_lock_destroy(rw);
}

void cf_read_lock(cf_rw_lock_t* rw)
{
	cute_read_lock(rw);
}

void cf_read_unlock(cf_rw_lock_t* rw)
{
	cute_read_unlock(rw);
}

void cf_write_lock(cf_rw_lock_t* rw)
{
	cute_write_lock(rw);
}

void cf_write_unlock(cf_rw_lock_t* rw)
{
	cute_write_unlock(rw);
}

cf_threadpool_t* cf_make_threadpool(int thread_count)
{
	return cute_threadpool_create(thread_count, NULL);
}

void cf_threadpool_add_task(cf_threadpool_t* pool, cf_task_fn* task, void* param)
{
	cute_threadpool_add_task(pool, task, param);
}

void cf_threadpool_kick_and_wait(cf_threadpool_t* pool)
{
	cute_threadpool_kick_and_wait(pool);
}

void cf_threadpool_kick(cf_threadpool_t* pool)
{
	cute_threadpool_kick(pool);
}

void cf_destroy_threadpool(cf_threadpool_t* pool)
{
	cute_threadpool_destroy(pool);
}
