/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#include <cute_multithreading.h>
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

CF_Mutex cf_make_mutex()
{
	return cute_mutex_create();
}

void cf_destroy_mutex(CF_Mutex* mutex)
{
	cute_mutex_destroy(mutex);
}

CF_Result cf_mutex_lock(CF_Mutex* mutex)
{
	CF_Result result;
	result.code = cute_lock(mutex);
	result.details = NULL;
	return result;
}

CF_Result cf_mutex_unlock(CF_Mutex* mutex)
{
	CF_Result result;
	result.code = cute_unlock(mutex);
	result.details = NULL;
	return result;
}

bool cf_mutex_try_lock(CF_Mutex* mutex)
{
	return !!cute_trylock(mutex);
}

CF_ConditionVariable cf_make_cv()
{
	return cute_cv_create();
}

void cf_destroy_cv(CF_ConditionVariable* cv)
{
	cute_cv_destroy(cv);
}

CF_Result cf_cv_wake_all(CF_ConditionVariable* cv)
{
	CF_Result result;
	result.code = cute_cv_wake_all(cv);
	result.details = NULL;
	return result;
}

CF_Result cf_cv_wake_one(CF_ConditionVariable* cv)
{
	CF_Result result;
	result.code = cute_cv_wake_one(cv);
	result.details = NULL;
	return result;
}

CF_Result cf_cv_wait(CF_ConditionVariable* cv, CF_Mutex* mutex)
{
	CF_Result result;
	result.code = cute_cv_wait(cv, mutex);
	result.details = NULL;
	return result;
}

CF_Semaphore cf_make_sem(int initial_count)
{
	return cute_semaphore_create(initial_count);
}

void cf_destroy_sem(CF_Semaphore* semaphore)
{
	cute_semaphore_destroy(semaphore);
}

CF_Result cf_sem_post(CF_Semaphore* semaphore)
{
	CF_Result result;
	result.code = cute_semaphore_post(semaphore);
	result.details = NULL;
	return result;
}

CF_Result cf_sem_try(CF_Semaphore* semaphore)
{
	CF_Result result;
	result.code = cute_semaphore_try(semaphore);
	result.details = NULL;
	return result;
}

CF_Result cf_sem_wait(CF_Semaphore* semaphore)
{
	CF_Result result;
	result.code = cute_semaphore_wait(semaphore);
	result.details = NULL;
	return result;
}

CF_Result cf_sem_value(CF_Semaphore* semaphore)
{
	CF_Result result;
	result.code = cute_semaphore_value(semaphore);
	result.details = NULL;
	return result;
}

CF_Thread* cf_thread_create(CF_ThreadFn func, const char* name, void* udata)
{
	return cute_thread_create(func, name, udata);
}

void cf_thread_detach(CF_Thread* thread)
{
	cute_thread_detach(thread);
}

CF_ThreadId cf_thread_get_id(CF_Thread* thread)
{
	return cute_thread_get_id(thread);
}

CF_ThreadId cf_thread_id()
{
	return cute_thread_id();
}

CF_Result cf_thread_wait(CF_Thread* thread)
{
	CF_Result result;
	result.code = cute_thread_wait(thread);
	result.details = NULL;
	return result;
}

int cf_core_count()
{
	return cute_core_count();
}

int cf_cacheline_size()
{
	return cute_cacheline_size();
}

CF_AtomicInt cf_atomic_zero()
{
	CF_AtomicInt result;
	result.i = 0;
	return result;
}

int cf_atomic_add(CF_AtomicInt* atomic, int addend)
{
	return cute_atomic_add(atomic, addend);
}

int cf_atomic_set(CF_AtomicInt* atomic, int value)
{
	return cute_atomic_set(atomic, value);
}

int cf_atomic_get(CF_AtomicInt* atomic)
{
	return cute_atomic_get(atomic);
}

CF_Result cf_atomic_cas(CF_AtomicInt* atomic, int expected, int value)
{
	CF_Result result;
	result.code = cute_atomic_cas(atomic, expected, value);
	result.details = NULL;
	return result;
}

void* cf_atomic_ptr_set(void** atomic, void* value)
{
	return cute_atomic_ptr_set(atomic, value);
}

void* cf_atomic_ptr_get(void** atomic)
{
	return cute_atomic_ptr_get(atomic);
}

CF_Result cf_atomic_ptr_cas(void** atomic, void* expected, void* value)
{
	CF_Result result;
	result.code = cute_atomic_ptr_cas(atomic, expected, value);
	result.details = NULL;
	return result;
}

CF_ReadWriteLock cf_make_rw_lock()
{
	return cute_rw_lock_create();
}

void cf_destroy_rw_lock(CF_ReadWriteLock* rw)
{
	if (rw) cute_rw_lock_destroy(rw);
}

void cf_read_lock(CF_ReadWriteLock* rw)
{
	cute_read_lock(rw);
}

void cf_read_unlock(CF_ReadWriteLock* rw)
{
	cute_read_unlock(rw);
}

void cf_write_lock(CF_ReadWriteLock* rw)
{
	cute_write_lock(rw);
}

void cf_write_unlock(CF_ReadWriteLock* rw)
{
	cute_write_unlock(rw);
}

CF_Threadpool* cf_make_threadpool(int thread_count)
{
	return cute_threadpool_create(thread_count, NULL);
}

void cf_threadpool_add_task(CF_Threadpool* pool, CF_TaskFn* task, void* param)
{
	cute_threadpool_add_task(pool, task, param);
}

void cf_threadpool_kick_and_wait(CF_Threadpool* pool)
{
	cute_threadpool_kick_and_wait(pool);
}

void cf_threadpool_kick(CF_Threadpool* pool)
{
	cute_threadpool_kick(pool);
}

void cf_destroy_threadpool(CF_Threadpool* pool)
{
	cute_threadpool_destroy(pool);
}
