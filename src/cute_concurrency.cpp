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

#include <SDL2/SDL.h>

#define CUTE_SYNC_IMPLEMENTATION
#define CUTE_SYNC_SDL
#define CUTE_THREAD_ALLOC CUTE_ALLOC
#define CUTE_THREAD_FREE CUTE_FREE
#include <cute/cute_sync.h>

namespace cute
{

mutex_t* mutex_create()
{
	return cute_mutex_create();
}

void mutex_destroy(mutex_t* mutex)
{
	cute_mutex_destroy(mutex);
}

int mutex_lock(mutex_t* mutex)
{
	return !cute_lock(mutex);
}

int mutex_unlock(mutex_t* mutex)
{
	return !cute_unlock(mutex);
}

int mutex_trylock(mutex_t* mutex)
{
	return !cute_trylock(mutex);
}

cv_t* cv_create()
{
	return cute_cv_create();
}

void cv_destroy(cv_t* cv)
{
	cute_cv_destroy(cv);
}

int cv_wake_all(cv_t* cv)
{
	return !cute_cv_wake_all(cv);
}

int cv_wake_one(cv_t* cv)
{
	return !cute_cv_wake_one(cv);
}

int cv_wait(cv_t* cv, mutex_t* mutex)
{
	return !cute_cv_wait(cv, mutex);
}

sem_t* sem_create(unsigned initial_count)
{
	return cute_sem_create(initial_count);
}

void sem_destroy(sem_t* semaphore)
{
	cute_sem_destroy(semaphore);
}

int sem_post(sem_t* semaphore)
{
	return !cute_sem_post(semaphore);
}

int sem_try(sem_t* semaphore)
{
	return !cute_sem_try(semaphore);
}

int sem_wait(sem_t* semaphore)
{
	return !cute_sem_wait(semaphore);
}

int sem_value(sem_t* semaphore)
{
	return !cute_sem_value(semaphore);
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

int thread_wait(thread_t* thread)
{
	return cute_thread_wait(thread);
}

int core_count()
{
	return cute_core_count();
}

int cacheline_size()
{
	return cute_cacheline_size();
}

int atomic_add(int* address, int addend)
{
	return cute_atomic_add(address, addend);
}

int atomic_set(int* address, int value)
{
	return cute_atomic_set(address, value);
}

int atomic_get(int* address)
{
	return cute_atomic_get(address);
}

int atomic_cas(int* address, int compare, int value)
{
	return cute_atomic_cas(address, compare, value);
}

void* atomic_ptr_set(void** address, void* value)
{
	return cute_atomic_ptr_set(address, value);
}

void* atomic_ptr_get(void** address)
{
	return cute_atomic_ptr_get(address);
}

int atomic_ptr_cas(void** address, void* compare, void* value)
{
	return cute_atomic_ptr_cas(address, compare, value);
}

rw_lock_t* rw_lock_create(void* user_allocator_context)
{
	rw_lock_t* rw = (rw_lock_t*)CUTE_ALLOC(sizeof(rw_lock_t), user_allocator_context);
	if (rw) cute_rw_lock_create(rw);
	return rw;
}

void rw_lock_destroy(rw_lock_t* rw, void* user_allocator_context)
{
	if (rw) cute_rw_lock_destroy(rw);
	CUTE_FREE(rw, user_allocator_context);
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
