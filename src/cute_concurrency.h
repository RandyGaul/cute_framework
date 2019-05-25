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

#include <SDL2/SDL_thread.h>
#define CUTE_SYNC_SDL
#include <cute/cute_sync.h>

namespace cute
{

using mutex_t       = cute_mutex_t;
using cv_t          = cute_cv_t;
using sem_t         = cute_sem_t;
using thread_t      = cute_thread_t;
using thread_id_t   = cute_thread_id_t;
using thread_func_t = cute_thread_func_t;
using rw_lock_t     = cute_rw_lock_t;
using threadpool_t  = cute_threadpool_t;

extern CUTE_API mutex_t* CUTE_CALL mutex_create();
extern CUTE_API void CUTE_CALL mutex_destroy(mutex_t* mutex);
extern CUTE_API error_t CUTE_CALL mutex_lock(mutex_t* mutex);
extern CUTE_API error_t CUTE_CALL mutex_unlock(mutex_t* mutex);
extern CUTE_API bool CUTE_CALL mutex_trylock(mutex_t* mutex);

extern CUTE_API cv_t* CUTE_CALL cv_create();
extern CUTE_API void CUTE_CALL cv_destroy(cv_t* cv);
extern CUTE_API error_t CUTE_CALL cv_wake_all(cv_t* cv);
extern CUTE_API error_t CUTE_CALL cv_wake_one(cv_t* cv);
extern CUTE_API error_t CUTE_CALL cv_wait(cv_t* cv, mutex_t* mutex);

extern CUTE_API sem_t* CUTE_CALL sem_create(unsigned initial_count);
extern CUTE_API void CUTE_CALL sem_destroy(sem_t* semaphore);
extern CUTE_API error_t CUTE_CALL sem_post(sem_t* semaphore);
extern CUTE_API error_t CUTE_CALL sem_try(sem_t* semaphore);
extern CUTE_API error_t CUTE_CALL sem_wait(sem_t* semaphore);
extern CUTE_API error_t CUTE_CALL sem_value(sem_t* semaphore);

extern CUTE_API thread_t* CUTE_CALL thread_create(thread_func_t func, const char* name, void* udata);
extern CUTE_API void CUTE_CALL thread_detach(thread_t* thread);
extern CUTE_API thread_id_t CUTE_CALL thread_get_id(thread_t* thread);
extern CUTE_API thread_id_t CUTE_CALL thread_id();
extern CUTE_API error_t CUTE_CALL thread_wait(thread_t* thread);

extern CUTE_API int CUTE_CALL core_count();
extern CUTE_API int CUTE_CALL cacheline_size();

extern CUTE_API int CUTE_CALL atomic_add(int* address, int addend);
extern CUTE_API int CUTE_CALL atomic_set(int* address, int value);
extern CUTE_API int CUTE_CALL atomic_get(int* address);
extern CUTE_API error_t CUTE_CALL atomic_cas(int* address, int compare, int value);
extern CUTE_API void* CUTE_CALL atomic_ptr_set(void** address, void* value);
extern CUTE_API void* CUTE_CALL atomic_ptr_get(void** address);
extern CUTE_API error_t CUTE_CALL atomic_ptr_cas(void** address, void* compare, void* value);

extern CUTE_API rw_lock_t* CUTE_CALL rw_lock_create(void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL rw_lock_destroy(rw_lock_t* rw, void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL read_lock(rw_lock_t* rw);
extern CUTE_API void CUTE_CALL read_unlock(rw_lock_t* rw);
extern CUTE_API void CUTE_CALL write_lock(rw_lock_t* rw);
extern CUTE_API void CUTE_CALL write_unlock(rw_lock_t* rw);

typedef void (CUTE_CALL task_fn)(void* param);

extern CUTE_API threadpool_t* CUTE_CALL threadpool_create(int thread_count, void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL threadpool_destroy(threadpool_t* pool);
extern CUTE_API void CUTE_CALL threadpool_add_task(threadpool_t* pool, task_fn* task, void* param);
extern CUTE_API void CUTE_CALL threadpool_kick_and_wait(threadpool_t* pool);
extern CUTE_API void CUTE_CALL threadpool_kick(threadpool_t* pool);

typedef void (CUTE_CALL promise_fn)(error_t status, void* param, void* promise_udata);

struct promise_t
{
	CUTE_INLINE promise_t () { }
	CUTE_INLINE promise_t (promise_fn* callback, void* promise_udata) : callback(callback), promise_udata(promise_udata) { }
	CUTE_INLINE void invoke(error_t status, void* param) { callback(status, param, promise_udata); }

	promise_fn* callback = NULL;
	void* promise_udata = NULL;
};

}

#endif // CUTE_CONCURRENCY_H
