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

namespace cute
{

mutex_t mutex_create()
{
	return cute_mutex_create();
}

void mutex_destroy(mutex_t* mutex)
{
	cute_mutex_destroy(mutex);
}

error_t mutex_lock(mutex_t* mutex)
{
	return error_make(cute_lock(mutex), NULL);
}

error_t mutex_unlock(mutex_t* mutex)
{
	return error_make(cute_unlock(mutex), NULL);
}

bool mutex_trylock(mutex_t* mutex)
{
	return !!cute_trylock(mutex);
}

cv_t cv_create()
{
	return cute_cv_create();
}

void cv_destroy(cv_t* cv)
{
	cute_cv_destroy(cv);
}

error_t cv_wake_all(cv_t* cv)
{
	return error_make(cute_cv_wake_all(cv), NULL);
}

error_t cv_wake_one(cv_t* cv)
{
	return error_make(cute_cv_wake_one(cv), NULL);
}

error_t cv_wait(cv_t* cv, mutex_t* mutex)
{
	return error_make(cute_cv_wait(cv, mutex), NULL);
}

semaphore_t sem_create(int initial_count)
{
	return cute_semaphore_create(initial_count);
}

void sem_destroy(semaphore_t* semaphore)
{
	cute_semaphore_destroy(semaphore);
}

error_t sem_post(semaphore_t* semaphore)
{
	return error_make(cute_semaphore_post(semaphore), NULL);
}

error_t sem_try(semaphore_t* semaphore)
{
	return error_make(cute_semaphore_try(semaphore), NULL);
}

error_t sem_wait(semaphore_t* semaphore)
{
	return error_make(cute_semaphore_wait(semaphore), NULL);
}

error_t sem_value(semaphore_t* semaphore)
{
	return error_make(cute_semaphore_value(semaphore), NULL);
}

thread_t* thread_create(thread_func_t func, const char* name, void* udata)
{
	return cute_thread_create(func, name, udata);
}

void thread_detach(thread_t* thread)
{
	cute_thread_detach(thread);
}

thread_id_t thread_get_id(thread_t* thread)
{
	return cute_thread_get_id(thread);
}

thread_id_t thread_id()
{
	return cute_thread_id();
}

error_t thread_wait(thread_t* thread)
{
	return error_make(cute_thread_wait(thread), NULL);
}

int core_count()
{
	return cute_core_count();
}

int cacheline_size()
{
	return cute_cacheline_size();
}

atomic_int_t atomic_zero()
{
	atomic_int_t result;
	result.i = 0;
	return result;
}

int atomic_add(atomic_int_t* atomic, int addend)
{
	return cute_atomic_add(atomic, addend);
}

int atomic_set(atomic_int_t* atomic, int value)
{
	return cute_atomic_set(atomic, value);
}

int atomic_get(atomic_int_t* atomic)
{
	return cute_atomic_get(atomic);
}

error_t atomic_cas(atomic_int_t* atomic, int expected, int value)
{
	return error_make(cute_atomic_cas(atomic, expected, value), NULL);
}

void* atomic_ptr_set(void** atomic, void* value)
{
	return cute_atomic_ptr_set(atomic, value);
}

void* atomic_ptr_get(void** atomic)
{
	return cute_atomic_ptr_get(atomic);
}

error_t atomic_ptr_cas(void** atomic, void* expected, void* value)
{
	return error_make(cute_atomic_ptr_cas(atomic, expected, value), NULL);
}

rw_lock_t rw_lock_create()
{
	return cute_rw_lock_create();
}

void rw_lock_destroy(rw_lock_t* rw)
{
	if (rw) cute_rw_lock_destroy(rw);
}

void read_lock(rw_lock_t* rw)
{
	cute_read_lock(rw);
}

void read_unlock(rw_lock_t* rw)
{
	cute_read_unlock(rw);
}

void write_lock(rw_lock_t* rw)
{
	cute_write_lock(rw);
}

void write_unlock(rw_lock_t* rw)
{
	cute_write_unlock(rw);
}

threadpool_t* threadpool_create(int thread_count, void* user_allocator_context)
{
	return cute_threadpool_create(thread_count, user_allocator_context);
}

void threadpool_add_task(threadpool_t* pool, task_fn* task, void* param)
{
	cute_threadpool_add_task(pool, task, param);
}

void threadpool_kick_and_wait(threadpool_t* pool)
{
	cute_threadpool_kick_and_wait(pool);
}

void threadpool_kick(threadpool_t* pool)
{
	cute_threadpool_kick(pool);
}

void threadpool_destroy(threadpool_t* pool)
{
	cute_threadpool_destroy(pool);
}

}
