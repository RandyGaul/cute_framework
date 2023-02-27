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

// TODO - Rename this file to cute_multithreading.h

/**
 * @struct   CF_Mutex
 * @category multithreading
 * @brief    An opaque handle representing a mutex.
 * @related  CF_Mutex cf_make_mutex cf_destroy_mutex cf_mutex_lock cf_mutex_unlock cf_mutex_try_lock
 */
typedef cute_mutex_t CF_Mutex;
// @end

/**
 * @struct   CF_ConditionVariable
 * @category multithreading
 * @brief    An opaque handle representing a condition variable.
 * @related  CF_ConditionVariable cf_make_cv cf_destroy_cv cf_cv_wake_all cf_cv_wake_one cf_cv_wait
 */
typedef cute_cv_t CF_ConditionVariable;
// @end

/**
 * @struct   CF_AtomicInt
 * @category atomic
 * @brief    An opaque handle representing an atomic integer.
 * @related  CF_AtomicInt cf_atomic_zero cf_atomic_add cf_atomic_set cf_atomic_get cf_atomic_cas cf_atomic_ptr_set cf_atomic_ptr_get cf_atomic_ptr_cas
 */
typedef cute_atomic_int_t CF_AtomicInt;
// @end

/**
 * @struct   CF_Semaphore
 * @category multithreading
 * @brief    An opaque handle representing a semaphore.
 * @related  CF_Semaphore cf_make_sem cf_destroy_sem cf_sem_post cf_sem_try cf_sem_wait cf_sem_value
 */
typedef cute_semaphore_t CF_Semaphore;
// @end

/**
 * @struct   CF_Thread
 * @category multithreading
 * @brief    An opaque handle representing a thread.
 * @related  CF_Thread cf_thread_create cf_thread_detach cf_thread_get_id cf_thread_id cf_thread_wait
 */
typedef cute_thread_t CF_Thread;
// @end

/**
 * @struct   CF_ThreadId
 * @category multithreading
 * @brief    An identifier of a thread.
 * @related  CF_Thread cf_thread_create cf_thread_detach cf_thread_get_id cf_thread_id cf_thread_wait
 */
typedef cute_thread_id_t CF_ThreadId;
// @end

/**
 * @struct   CF_ThreadFn
 * @category multithreading
 * @brief    A thread.
 * @example  > Example syntax of a thread.
 *     int MyThreadFn(void *udata)
 *     {
 *         // Do work here...
 *         return 0;
 *     }
 * @related  CF_Thread cf_thread_create cf_thread_detach cf_thread_get_id cf_thread_id cf_thread_wait
 */
typedef cute_thread_fn CF_ThreadFn;
// @end

/**
 * @struct   CF_ReadWriteLock
 * @category multithreading
 * @brief    An opaque handle representing a read-write lock.
 * @related  CF_ReadWriteLock cf_make_rw_lock cf_destroy_rw_lock cf_read_lock cf_read_unlock cf_write_lock cf_write_unlock
 */
typedef cute_rw_lock_t CF_ReadWriteLock;
// @end

/**
 * @struct   CF_Threadpool
 * @category multithreading
 * @brief    An opaque handle representing a threadpool.
 * @related  CF_Threadpool CF_TaskFn cf_make_threadpool cf_destroy_threadpool cf_threadpool_add_task cf_threadpool_kick_and_wait cf_threadpool_kick
 */
typedef cute_threadpool_t CF_Threadpool;
// @end

/**
 * @function cf_make_mutex
 * @category multithreading
 * @brief    Returns an initialized `CF_Mutex`.
 * @remarks  Destroy the mutex with `cf_destroy_mutex` when done.
 * @related  CF_Mutex cf_make_mutex cf_destroy_mutex cf_mutex_lock cf_mutex_unlock cf_mutex_try_lock
 */
CUTE_API CF_Mutex CUTE_CALL cf_make_mutex();

/**
 * @function cf_destroy_mutex
 * @category multithreading
 * @brief    Destroys a `CF_Mutex` created with `cf_make_mutex`.
 * @param    mutex      The mutex.
 * @related  CF_Mutex cf_make_mutex cf_destroy_mutex cf_mutex_lock cf_mutex_unlock cf_mutex_try_lock
 */
CUTE_API void CUTE_CALL cf_destroy_mutex(CF_Mutex* mutex);

/**
 * @function cf_mutex_lock
 * @category multithreading
 * @brief    Locks a `CF_Mutex`.
 * @param    mutex      The mutex.
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  Will cause the thread to wait until the lock is available if it's currently locked.
 * @related  CF_Mutex cf_make_mutex cf_destroy_mutex cf_mutex_lock cf_mutex_unlock cf_mutex_try_lock
 */
CUTE_API CF_Result CUTE_CALL cf_mutex_lock(CF_Mutex* mutex);

/**
 * @function cf_mutex_unlock
 * @category multithreading
 * @brief    Unlocks a `CF_Mutex`.
 * @param    mutex      The mutex.
 * @return   Returns any errors as a `CF_Result`.
 * @related  CF_Mutex cf_make_mutex cf_destroy_mutex cf_mutex_lock cf_mutex_unlock cf_mutex_try_lock
 */
CUTE_API CF_Result CUTE_CALL cf_mutex_unlock(CF_Mutex* mutex);

/**
 * @function cf_mutex_try_lock
 * @category multithreading
 * @brief    Attempts to lock a `CF_Mutex` without waiting.
 * @param    mutex      The mutex.
 * @return   Returns true if the lock was acquired, and false if the lock was already locked.
 * @related  CF_Mutex cf_make_mutex cf_destroy_mutex cf_mutex_lock cf_mutex_unlock cf_mutex_try_lock
 */
CUTE_API bool CUTE_CALL cf_mutex_try_lock(CF_Mutex* mutex);

/**
 * @function cf_make_cv
 * @category multithreading
 * @brief    Returns an initialized `CF_ConditionVariable`.
 * @remarks  Destroy the mutex with `cf_destroy_cv` when done.
 * @related  CF_ConditionVariable cf_make_cv cf_destroy_cv cf_cv_wake_all cf_cv_wake_one cf_cv_wait
 */
CUTE_API CF_ConditionVariable CUTE_CALL cf_make_cv();

/**
 * @function cf_destroy_cv
 * @category multithreading
 * @brief    Destroys a `CF_ConditionVariable` created with `cf_make_cv`.
 * @param    cv         The condition variable.
 * @related  CF_ConditionVariable cf_make_cv cf_destroy_cv cf_cv_wake_all cf_cv_wake_one cf_cv_wait
 */
CUTE_API void CUTE_CALL cf_destroy_cv(CF_ConditionVariable* cv);

/**
 * @function cf_cv_wake_all
 * @category multithreading
 * @brief    Wakes all threads sleeping on the condition variable.
 * @param    cv         The condition variable.
 * @return   Returns any errors as a `CF_Result`.
 * @related  CF_ConditionVariable cf_make_cv cf_destroy_cv cf_cv_wake_all cf_cv_wake_one cf_cv_wait
 */
CUTE_API CF_Result CUTE_CALL cf_cv_wake_all(CF_ConditionVariable* cv);

/**
 * @function cf_cv_wake_one
 * @category multithreading
 * @brief    Wakes a single (implementation-dependent) thread sleeping on the condition variable.
 * @param    cv         The condition variable.
 * @return   Returns any errors as a `CF_Result`.
 * @related  CF_ConditionVariable cf_make_cv cf_destroy_cv cf_cv_wake_all cf_cv_wake_one cf_cv_wait
 */
CUTE_API CF_Result CUTE_CALL cf_cv_wake_one(CF_ConditionVariable* cv);

/**
 * @function cf_cv_wait
 * @category multithreading
 * @brief    Causes the calling thread to wait on the condition variable.
 * @param    cv         The condition variable.
 * @param    mutex      The mutex used to access the condition variable.
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  The thread will not wake until `cf_cv_wake_all` or `cf_cv_wake_one` is called.
 * @related  CF_ConditionVariable cf_make_cv cf_destroy_cv cf_cv_wake_all cf_cv_wake_one cf_cv_wait
 */
CUTE_API CF_Result CUTE_CALL cf_cv_wait(CF_ConditionVariable* cv, CF_Mutex* mutex);

CUTE_API CF_Semaphore CUTE_CALL cf_make_sem(int initial_count);
CUTE_API void CUTE_CALL cf_destroy_sem(CF_Semaphore* semaphore);
CUTE_API CF_Result CUTE_CALL cf_sem_post(CF_Semaphore* semaphore);
CUTE_API CF_Result CUTE_CALL cf_sem_try(CF_Semaphore* semaphore);
CUTE_API CF_Result CUTE_CALL cf_sem_wait(CF_Semaphore* semaphore);
CUTE_API CF_Result CUTE_CALL cf_sem_value(CF_Semaphore* semaphore);

CUTE_API CF_Thread* CUTE_CALL cf_thread_create(CF_ThreadFn func, const char* name, void* udata);
CUTE_API void CUTE_CALL cf_thread_detach(CF_Thread* thread);
CUTE_API CF_ThreadId CUTE_CALL cf_thread_get_id(CF_Thread* thread);
CUTE_API CF_ThreadId CUTE_CALL cf_thread_id();
CUTE_API CF_Result CUTE_CALL cf_thread_wait(CF_Thread* thread);

CUTE_API int CUTE_CALL cf_core_count();
CUTE_API int CUTE_CALL cf_cacheline_size();

CUTE_API CF_AtomicInt CUTE_CALL cf_atomic_zero();
CUTE_API int CUTE_CALL cf_atomic_add(CF_AtomicInt* atomic, int addend);
CUTE_API int CUTE_CALL cf_atomic_set(CF_AtomicInt* atomic, int value);
CUTE_API int CUTE_CALL cf_atomic_get(CF_AtomicInt* atomic);
CUTE_API CF_Result CUTE_CALL cf_atomic_cas(CF_AtomicInt* atomic, int expected, int value);
CUTE_API void* CUTE_CALL cf_atomic_ptr_set(void** atomic, void* value);
CUTE_API void* CUTE_CALL cf_atomic_ptr_get(void** atomic);
CUTE_API CF_Result CUTE_CALL cf_atomic_ptr_cas(void** atomic, void* expected, void* value);

CUTE_API CF_ReadWriteLock CUTE_CALL cf_make_rw_lock();
CUTE_API void CUTE_CALL cf_destroy_rw_lock(CF_ReadWriteLock* rw);
CUTE_API void CUTE_CALL cf_read_lock(CF_ReadWriteLock* rw);
CUTE_API void CUTE_CALL cf_read_unlock(CF_ReadWriteLock* rw);
CUTE_API void CUTE_CALL cf_write_lock(CF_ReadWriteLock* rw);
CUTE_API void CUTE_CALL cf_write_unlock(CF_ReadWriteLock* rw);

typedef void (CUTE_CALL CF_TaskFn)(void* param);

CUTE_API CF_Threadpool* CUTE_CALL cf_make_threadpool(int thread_count);
CUTE_API void CUTE_CALL cf_destroy_threadpool(CF_Threadpool* pool);
CUTE_API void CUTE_CALL cf_threadpool_add_task(CF_Threadpool* pool, CF_TaskFn* task, void* param);
CUTE_API void CUTE_CALL cf_threadpool_kick_and_wait(CF_Threadpool* pool);
CUTE_API void CUTE_CALL cf_threadpool_kick(CF_Threadpool* pool);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using Mutex = CF_Mutex;
using ConditionVariable = CF_ConditionVariable;
using AtomicInt = CF_AtomicInt;
using Semaphore = CF_Semaphore;
using Thread = CF_Thread;
using ThreadId = CF_ThreadId;
using ThreadFn = CF_ThreadFn;
using ReadWriteLock = CF_ReadWriteLock;
using Threadpool = CF_Threadpool;
using TaskFn = CF_TaskFn;

CUTE_INLINE Mutex make_mutex() { return cf_make_mutex(); }
CUTE_INLINE void destroy_mutex(Mutex* mutex) { cf_destroy_mutex(mutex); }
CUTE_INLINE Result mutex_lock(Mutex* mutex) { return cf_mutex_lock(mutex); }
CUTE_INLINE Result mutex_unlock(Mutex* mutex) { return cf_mutex_unlock(mutex); }
CUTE_INLINE bool Mutexrylock(Mutex* mutex) { return cf_mutex_try_lock(mutex); }

CUTE_INLINE ConditionVariable make_cv() { return cf_make_cv(); }
CUTE_INLINE void destroy_cv(ConditionVariable* cv) { cf_destroy_cv(cv); }
CUTE_INLINE Result cv_wake_all(ConditionVariable* cv) { return cf_cv_wake_all(cv); }
CUTE_INLINE Result cv_wake_one(ConditionVariable* cv) { return cf_cv_wake_one(cv); }
CUTE_INLINE Result cv_wait(ConditionVariable* cv, Mutex* mutex) { return cf_cv_wait(cv, mutex); }

CUTE_INLINE Semaphore make_sem(int initial_count) { return cf_make_sem(initial_count); }
CUTE_INLINE void destroy_sem(Semaphore* semaphore) { cf_destroy_sem(semaphore); }
CUTE_INLINE Result sem_post(Semaphore* semaphore) { return cf_sem_post(semaphore); }
CUTE_INLINE Result sem_try(Semaphore* semaphore) { return cf_sem_try(semaphore); }
CUTE_INLINE Result sem_wait(Semaphore* semaphore) { return cf_sem_wait(semaphore); }
CUTE_INLINE Result sem_value(Semaphore* semaphore) { return cf_sem_value(semaphore); }

CUTE_INLINE Thread* thread_create(ThreadFn func, const char* name, void* udata) { return cf_thread_create(func, name, udata); }
CUTE_INLINE void thread_detach(Thread* thread) { cf_thread_detach(thread); }
CUTE_INLINE ThreadId thread_get_id(Thread* thread) { return cf_thread_get_id(thread); }
CUTE_INLINE ThreadId thread_id() { return cf_thread_id(); }
CUTE_INLINE Result thread_wait(Thread* thread) { return cf_thread_wait(thread); }

CUTE_INLINE int core_count() { return cf_core_count(); }
CUTE_INLINE int cacheline_size() { return cf_cacheline_size(); }

CUTE_INLINE AtomicInt atomic_zero() { return cf_atomic_zero(); }
CUTE_INLINE int atomic_add(AtomicInt* atomic, int addend) { return cf_atomic_add(atomic, addend); }
CUTE_INLINE int atomic_set(AtomicInt* atomic, int value) { return cf_atomic_set(atomic, value); }
CUTE_INLINE int atomic_get(AtomicInt* atomic) { return cf_atomic_get(atomic); }
CUTE_INLINE Result atomic_cas(AtomicInt* atomic, int expected, int value) { return cf_atomic_cas(atomic, expected, value); }
CUTE_INLINE void* atomic_ptr_set(void** atomic, void* value) { return cf_atomic_ptr_set(atomic, value); }
CUTE_INLINE void* atomic_ptr_get(void** atomic) { return cf_atomic_ptr_get(atomic); }
CUTE_INLINE Result atomic_ptr_cas(void** atomic, void* expected, void* value) { return cf_atomic_ptr_cas(atomic, expected, value); }

CUTE_INLINE ReadWriteLock make_rw_lock() { return cf_make_rw_lock(); }
CUTE_INLINE void destroy_rw_lock(ReadWriteLock* rw) { cf_destroy_rw_lock(rw); }
CUTE_INLINE void read_lock(ReadWriteLock* rw) { cf_read_lock(rw); }
CUTE_INLINE void read_unlock(ReadWriteLock* rw) { cf_read_unlock(rw); }
CUTE_INLINE void write_lock(ReadWriteLock* rw) { cf_write_lock(rw); }
CUTE_INLINE void write_unlock(ReadWriteLock* rw) { cf_write_unlock(rw); }

CUTE_INLINE Threadpool* make_threadpool(int thread_count) { return cf_make_threadpool(thread_count); }
CUTE_INLINE void destroy_threadpool(Threadpool* pool) { return cf_destroy_threadpool(pool); }
CUTE_INLINE void threadpool_add_task(Threadpool* pool, TaskFn* task, void* param) { return cf_threadpool_add_task(pool, task, param); }
CUTE_INLINE void threadpool_kick_and_wait(Threadpool* pool) { return cf_threadpool_kick_and_wait(pool); }
CUTE_INLINE void threadpool_kick(Threadpool* pool) { return cf_threadpool_kick(pool); }

}

#endif // CUTE_CPP

#endif // CUTE_CONCURRENCY_H
