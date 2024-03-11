/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_MULTITHREADING_H
#define CF_MULTITHREADING_H

#include "cute_defines.h"
#include "cute_result.h"

#include "cute/cute_sync.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

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
 * @remarks  Atomics are an advanced topic. You've been warned! Beej has a [good article on atomics](https://beej.us/guide/bgc/html/split/chapter-atomics.html).
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
 * @remarks  A read/write lock can have a large number of simultaneous readers, but only one writer at a time. This can be
 *           used as an opimization where a resources can be safely read from many threads. Then, when the resource must be
 *           modified a writer can wait for all readers to leave, and then exclusively lock to perform a write update.
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
 * @brief    Returns an unlocked `CF_Mutex`.
 * @remarks  Destroy the mutex with `cf_destroy_mutex` when done.
 * @related  CF_Mutex cf_make_mutex cf_destroy_mutex cf_mutex_lock cf_mutex_unlock cf_mutex_try_lock
 */
CF_API CF_Mutex CF_CALL cf_make_mutex();

/**
 * @function cf_destroy_mutex
 * @category multithreading
 * @brief    Destroys a `CF_Mutex` created with `cf_make_mutex`.
 * @param    mutex      The mutex.
 * @related  CF_Mutex cf_make_mutex cf_destroy_mutex cf_mutex_lock cf_mutex_unlock cf_mutex_try_lock
 */
CF_API void CF_CALL cf_destroy_mutex(CF_Mutex* mutex);

/**
 * @function cf_mutex_lock
 * @category multithreading
 * @brief    Locks a `CF_Mutex`.
 * @param    mutex      The mutex.
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  Will cause the thread to wait until the lock is available if it's currently locked.
 * @related  CF_Mutex cf_make_mutex cf_destroy_mutex cf_mutex_lock cf_mutex_unlock cf_mutex_try_lock
 */
CF_API CF_Result CF_CALL cf_mutex_lock(CF_Mutex* mutex);

/**
 * @function cf_mutex_unlock
 * @category multithreading
 * @brief    Unlocks a `CF_Mutex`.
 * @param    mutex      The mutex.
 * @return   Returns any errors as a `CF_Result`.
 * @related  CF_Mutex cf_make_mutex cf_destroy_mutex cf_mutex_lock cf_mutex_unlock cf_mutex_try_lock
 */
CF_API CF_Result CF_CALL cf_mutex_unlock(CF_Mutex* mutex);

/**
 * @function cf_mutex_try_lock
 * @category multithreading
 * @brief    Attempts to lock a `CF_Mutex` without waiting.
 * @param    mutex      The mutex.
 * @return   Returns true if the lock was acquired, and false if the lock was already locked.
 * @related  CF_Mutex cf_make_mutex cf_destroy_mutex cf_mutex_lock cf_mutex_unlock cf_mutex_try_lock
 */
CF_API bool CF_CALL cf_mutex_try_lock(CF_Mutex* mutex);

/**
 * @function cf_make_cv
 * @category multithreading
 * @brief    Returns an initialized `CF_ConditionVariable`, used to sleep or wake threads.
 * @remarks  Destroy the mutex with `cf_destroy_cv` when done.
 * @related  CF_ConditionVariable cf_make_cv cf_destroy_cv cf_cv_wake_all cf_cv_wake_one cf_cv_wait
 */
CF_API CF_ConditionVariable CF_CALL cf_make_cv();

/**
 * @function cf_destroy_cv
 * @category multithreading
 * @brief    Destroys a `CF_ConditionVariable` created with `cf_make_cv`.
 * @param    cv         The condition variable.
 * @related  CF_ConditionVariable cf_make_cv cf_destroy_cv cf_cv_wake_all cf_cv_wake_one cf_cv_wait
 */
CF_API void CF_CALL cf_destroy_cv(CF_ConditionVariable* cv);

/**
 * @function cf_cv_wake_all
 * @category multithreading
 * @brief    Wakes all threads sleeping on the condition variable.
 * @param    cv         The condition variable.
 * @return   Returns any errors as a `CF_Result`.
 * @related  CF_ConditionVariable cf_make_cv cf_destroy_cv cf_cv_wake_all cf_cv_wake_one cf_cv_wait
 */
CF_API CF_Result CF_CALL cf_cv_wake_all(CF_ConditionVariable* cv);

/**
 * @function cf_cv_wake_one
 * @category multithreading
 * @brief    Wakes a single (implementation-dependent) thread sleeping on the condition variable.
 * @param    cv         The condition variable.
 * @return   Returns any errors as a `CF_Result`.
 * @related  CF_ConditionVariable cf_make_cv cf_destroy_cv cf_cv_wake_all cf_cv_wake_one cf_cv_wait
 */
CF_API CF_Result CF_CALL cf_cv_wake_one(CF_ConditionVariable* cv);

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
CF_API CF_Result CF_CALL cf_cv_wait(CF_ConditionVariable* cv, CF_Mutex* mutex);

/**
 * @function cf_make_sem
 * @category multithreading
 * @brief    Returns an initialized semaphore.
 * @param    initial_count  The initial value of the semaphore.
 * @remarks  Semaphores are used to prevent race conditions between multiple threads that need to access
 *           common resources. Usually you'll have N resources, and initialize the semaphore to N. This is
 *           a rather advanced and low-level topic, beware. To learn more about semaphores I suggest reading
 *           the online book "The Little Book of Semaphores".
 * @related  CF_Semaphore cf_make_sem cf_destroy_sem cf_sem_post cf_sem_try cf_sem_wait cf_sem_value
 */
CF_API CF_Semaphore CF_CALL cf_make_sem(int initial_count);

/**
 * @function cf_destroy_sem
 * @category multithreading
 * @brief    Destroys a `CF_Semaphore` made by `cf_make_sem`.
 * @param    semaphore  The semaphore.
 * @related  CF_Semaphore cf_make_sem cf_destroy_sem cf_sem_post cf_sem_try cf_sem_wait cf_sem_value
 */
CF_API void CF_CALL cf_destroy_sem(CF_Semaphore* semaphore);

/**
 * @function cf_sem_post
 * @category multithreading
 * @brief    Increments the semaphore's counter and wakes one thread if the counter becomes greater than zero.
 * @param    semaphore  The semaphore.
 * @remarks  As other threads call `cf_sem_try` or `cf_sem_wait` they decrement the semaphore's counter. Eventually
 *           the counter will become zero, causing additional threads to wait (sleep). When the resources become
 *           available again, this function is used to wake one up.
 * @related  CF_Semaphore cf_make_sem cf_destroy_sem cf_sem_post cf_sem_try cf_sem_wait cf_sem_value
 */
CF_API CF_Result CF_CALL cf_sem_post(CF_Semaphore* semaphore);

/**
 * @function cf_sem_try
 * @category multithreading
 * @brief    Attempts to decrement the semaphore's counter without sleeping, and returns success if decremented.
 * @param    semaphore  The semaphore.
 * @return   This function will not cause the thread to sleep if the semaphore's counter is zero. Instead, an error will
 *           be returned. Success is returned if the semaphore was successfully acquired and the counter was decremented.
 * @remarks  Since this function does not block/sleep, it allows the thread to continue running, even if the return
 *           value was an error. This lets a thread poll the semaphore instead of blocking/sleeping.
 * @related  CF_Semaphore cf_make_sem cf_destroy_sem cf_sem_post cf_sem_try cf_sem_wait cf_sem_value
 */
CF_API CF_Result CF_CALL cf_sem_try(CF_Semaphore* semaphore);

/**
 * @function cf_sem_wait
 * @category multithreading
 * @brief    Acquires the semaphore.
 * @param    semaphore  The semaphore.
 * @return   Returns any errors upon failure.
 * @remarks  The calling thread will sleep until the semaphore's counter is positive. When positive, the counter will be
 *           decremented once.
 * @related  CF_Semaphore cf_make_sem cf_destroy_sem cf_sem_post cf_sem_try cf_sem_wait cf_sem_value
 */
CF_API CF_Result CF_CALL cf_sem_wait(CF_Semaphore* semaphore);

/**
 * @function cf_sem_value
 * @category multithreading
 * @brief    Atomically fetches the semaphore's counter.
 * @param    semaphore  The semaphore.
 * @return   Returns any errors upon failure.
 * @related  CF_Semaphore cf_make_sem cf_destroy_sem cf_sem_post cf_sem_try cf_sem_wait cf_sem_value
 */
CF_API CF_Result CF_CALL cf_sem_value(CF_Semaphore* semaphore);

/**
 * @function cf_thread_create
 * @category multithreading
 * @brief    Creates a new thread and runs it's thread function (`CF_ThreadFn`).
 * @param    func    The function to run for the thread.
 * @param    name    The name of this thread. Must be unique.
 * @param    udata   Can be `NULL`. This gets handed back to you in your `func`.
 * @return   Returns an opaque pointer to `CF_Thread`.
 * @example  > Example syntax of a thread.
 *     int MyThreadFn(void *udata)
 *     {
 *         // Do work here...
 *         return 0;
 *     }
 * @remarks  Unless you call `cf_thread_detach` you should call `cf_thread_wait` from another thread to
 *           clean up resources and get the thread's return value back. It is considered a leak otherwise.
 * @related  CF_Thread CF_ThreadFn cf_thread_create cf_thread_detach cf_thread_get_id cf_thread_id cf_thread_wait
 */
CF_API CF_Thread* CF_CALL cf_thread_create(CF_ThreadFn func, const char* name, void* udata);

/**
 * @function cf_thread_detach
 * @category multithreading
 * @brief    Makes a special note your thread will never have `cf_thread_wait` called on it. Useful as a minor optimization
 *           for long-lived threads.
 * @param    thread  The thread.
 * @remarks  When a thread has `cf_thread_detach` called on it, it is no longer necessary to call `cf_thread_wait` on it.
 * @related  CF_Thread CF_ThreadFn cf_thread_create cf_thread_detach cf_thread_get_id cf_thread_id cf_thread_wait
 */
CF_API void CF_CALL cf_thread_detach(CF_Thread* thread);

/**
 * @function cf_thread_get_id
 * @category multithreading
 * @brief    Returns the unique id of a thread.
 * @param    thread  The thread.
 * @related  CF_Thread CF_ThreadFn cf_thread_create cf_thread_detach cf_thread_get_id cf_thread_id cf_thread_wait
 */
CF_API CF_ThreadId CF_CALL cf_thread_get_id(CF_Thread* thread);

/**
 * @function cf_thread_id
 * @category multithreading
 * @brief    Returns the unique id of the calling thread.
 * @param    thread  The thread.
 * @related  CF_Thread CF_ThreadFn cf_thread_create cf_thread_detach cf_thread_get_id cf_thread_id cf_thread_wait
 */
CF_API CF_ThreadId CF_CALL cf_thread_id();

/**
 * @function cf_thread_wait
 * @category multithreading
 * @brief    Waits and blocks/sleeps until the thread exits, and returns the thread's return result.
 * @param    thread  The thread.
 * @remarks  It is invalid to call this function on a detached thread (see `cf_thread_detach`). It is invalid to
 *           call this function on a thread more than once.
 * @related  CF_Thread CF_ThreadFn cf_thread_create cf_thread_detach cf_thread_get_id cf_thread_id cf_thread_wait
 */
CF_API CF_Result CF_CALL cf_thread_wait(CF_Thread* thread);

/**
 * @function cf_core_count
 * @category CPU
 * @brief    Returns the number of cores on the CPU. Can be affected my machine dependent technology, such as Intel's hyperthreading.
 * @related  cf_core_count
 */
CF_API int CF_CALL cf_core_count();

/**
 * @function cf_cacheline_size
 * @category CPU
 * @brief    Returns the number of bytes in a single cache line of the CPU L1 memory cache.
 * @related  cf_core_count
 */
CF_API int CF_CALL cf_cacheline_size();

/**
 * @function cf_atomic_zero
 * @category atomic
 * @brief    Returns an atomic integer of value zero.
 * @remarks  Atomics are an advanced topic. You've been warned!
 * @related  cf_atomic_zero cf_atomic_add cf_atomic_set cf_atomic_get cf_atomic_cas cf_atomic_ptr_set cf_atomic_ptr_get cf_atomic_ptr_cas
 */
CF_API CF_AtomicInt CF_CALL cf_atomic_zero();

/**
 * @function cf_atomic_add
 * @category atomic
 * @brief    Atomically adds `addend` to `atomic` and returns the old value from `atomic`.
 * @param    atomic     The integer to atomically manipulate.
 * @param    addend     A value to atomically add to `atomic`.
 * @remarks  Atomics are an advanced topic. You've been warned! Beej has a [good article on atomics](https://beej.us/guide/bgc/html/split/chapter-atomics.html).
 * @related  cf_atomic_zero cf_atomic_add cf_atomic_set cf_atomic_get cf_atomic_cas cf_atomic_ptr_set cf_atomic_ptr_get cf_atomic_ptr_cas
 */
CF_API int CF_CALL cf_atomic_add(CF_AtomicInt* atomic, int addend);

/**
 * @function cf_atomic_set
 * @category atomic
 * @brief    Atomically sets `atomic` to `value` and returns the old value from `atomic`.
 * @param    atomic     The integer to atomically manipulate.
 * @param    value      A value to atomically set to `atomic`.
 * @remarks  Atomics are an advanced topic. You've been warned! Beej has a [good article on atomics](https://beej.us/guide/bgc/html/split/chapter-atomics.html).
 * @related  cf_atomic_zero cf_atomic_add cf_atomic_set cf_atomic_get cf_atomic_cas cf_atomic_ptr_set cf_atomic_ptr_get cf_atomic_ptr_cas
 */
CF_API int CF_CALL cf_atomic_set(CF_AtomicInt* atomic, int value);

/**
 * @function cf_atomic_get
 * @category atomic
 * @brief    Atomically fetches the value at `atomic`.
 * @param    atomic     The integer to fetch from.
 * @remarks  Atomics are an advanced topic. You've been warned! Beej has a [good article on atomics](https://beej.us/guide/bgc/html/split/chapter-atomics.html).
 * @related  cf_atomic_zero cf_atomic_add cf_atomic_set cf_atomic_get cf_atomic_cas cf_atomic_ptr_set cf_atomic_ptr_get cf_atomic_ptr_cas
 */
CF_API int CF_CALL cf_atomic_get(CF_AtomicInt* atomic);

/**
 * @function cf_atomic_cas
 * @category atomic
 * @brief    Atomically sets `atomic` to `value` if `expected` equals `atomic`.
 * @param    atomic     The pointer to atomically manipulate.
 * @param    expected   Used to compare against `atomic`.
 * @param    value      A value to atomically set to `atomic`.
 * @return   Returns success if the value was set, error otherwise.
 * @remarks  Atomics are an advanced topic. You've been warned! Beej has a [good article on atomics](https://beej.us/guide/bgc/html/split/chapter-atomics.html).
 * @related  cf_atomic_zero cf_atomic_add cf_atomic_set cf_atomic_get cf_atomic_cas cf_atomic_ptr_set cf_atomic_ptr_get cf_atomic_ptr_cas
 */
CF_API CF_Result CF_CALL cf_atomic_cas(CF_AtomicInt* atomic, int expected, int value);

/**
 * @function cf_atomic_ptr_set
 * @category atomic
 * @brief    Atomically sets `atomic` to `value` and returns the old value from `atomic`.
 * @param    atomic     The pointer to atomically manipulate.
 * @param    value      A value to atomically set to `atomic`.
 * @remarks  Atomics are an advanced topic. You've been warned! Beej has a [good article on atomics](https://beej.us/guide/bgc/html/split/chapter-atomics.html).
 * @related  cf_atomic_zero cf_atomic_add cf_atomic_set cf_atomic_get cf_atomic_cas cf_atomic_ptr_set cf_atomic_ptr_get cf_atomic_ptr_cas
 */
CF_API void* CF_CALL cf_atomic_ptr_set(void** atomic, void* value);

/**
 * @function cf_atomic_ptr_get
 * @category atomic
 * @brief    Atomically fetches the value at `atomic`.
 * @param    atomic    The pointer to fetch from.
 * @remarks  Atomics are an advanced topic. You've been warned! Beej has a [good article on atomics](https://beej.us/guide/bgc/html/split/chapter-atomics.html).
 * @related  cf_atomic_zero cf_atomic_add cf_atomic_set cf_atomic_get cf_atomic_cas cf_atomic_ptr_set cf_atomic_ptr_get cf_atomic_ptr_cas
 */
CF_API void* CF_CALL cf_atomic_ptr_get(void** atomic);

/**
 * @function cf_atomic_ptr_cas
 * @category atomic
 * @brief    Atomically sets `atomic` to `value` if `expected` equals `atomic`.
 * @param    atomic     The pointer to atomically manipulate.
 * @param    expected   Used to compare against `atomic`.
 * @param    value      A value to atomically set to `atomic`.
 * @return   Returns success if the value was set, error otherwise.
 * @remarks  Atomics are an advanced topic. You've been warned! Beej has a [good article on atomics](https://beej.us/guide/bgc/html/split/chapter-atomics.html).
 * @related  cf_atomic_zero cf_atomic_add cf_atomic_set cf_atomic_get cf_atomic_cas cf_atomic_ptr_set cf_atomic_ptr_get cf_atomic_ptr_cas
 */
CF_API CF_Result CF_CALL cf_atomic_ptr_cas(void** atomic, void* expected, void* value);

/**
 * @function cf_make_rw_lock
 * @category multithreading
 * @brief    Returns an unlocked `CF_ReadWriteLock` lock.
 * @remarks  Call `cf_destroy_rw_lock` when done.
 * @related  CF_ReadWriteLock cf_make_rw_lock cf_destroy_rw_lock cf_read_lock cf_read_unlock cf_write_lock cf_write_unlock
 */
CF_API CF_ReadWriteLock CF_CALL cf_make_rw_lock();

/**
 * @function cf_destroy_rw_lock
 * @category multithreading
 * @brief    Destroys a `CF_ReadWriteLock` made from `cf_make_rw_lock`.
 * @param    rw         The read/write lock.
 * @related  CF_ReadWriteLock cf_make_rw_lock cf_destroy_rw_lock cf_read_lock cf_read_unlock cf_write_lock cf_write_unlock
 */
CF_API void CF_CALL cf_destroy_rw_lock(CF_ReadWriteLock* rw);

/**
 * @function cf_read_lock
 * @category multithreading
 * @brief    Locks for reading. Many simultaneous readers are allowed.
 * @param    rw         The read/write lock.
 * @related  CF_ReadWriteLock cf_make_rw_lock cf_destroy_rw_lock cf_read_lock cf_read_unlock cf_write_lock cf_write_unlock
 */
CF_API void CF_CALL cf_read_lock(CF_ReadWriteLock* rw);

/**
 * @function cf_read_unlock
 * @category multithreading
 * @brief    Undoes one call to `cf_read_lock`.
 * @param    rw         The read/write lock.
 * @related  CF_ReadWriteLock cf_make_rw_lock cf_destroy_rw_lock cf_read_lock cf_read_unlock cf_write_lock cf_write_unlock
 */
CF_API void CF_CALL cf_read_unlock(CF_ReadWriteLock* rw);

/**
 * @function cf_write_lock
 * @category multithreading
 * @brief    Locks for writing.
 * @param    rw         The read/write lock.
 * @remarks  When locked for writing, only one writer can be present with no readers. The writer will sleep/wait for all other
 *           readers and writers to unlock before acquiring exclusive access to the lock.
 * @related  CF_ReadWriteLock cf_make_rw_lock cf_destroy_rw_lock cf_read_lock cf_read_unlock cf_write_lock cf_write_unlock
 */
CF_API void CF_CALL cf_write_lock(CF_ReadWriteLock* rw);

/**
 * @function cf_write_unlock
 * @category multithreading
 * @brief    Unlocks for writing.
 * @param    rw         The read/write lock.
 * @remarks  When locked for writing, only one writer can be present with no readers. The writer will sleep/wait for all other
 *           readers and writers to unlock before acquiring exclusive access to the lock.
 * @related  CF_ReadWriteLock cf_make_rw_lock cf_destroy_rw_lock cf_read_lock cf_read_unlock cf_write_lock cf_write_unlock
 */
CF_API void CF_CALL cf_write_unlock(CF_ReadWriteLock* rw);

/**
 * @function CF_TaskFn
 * @category multithreading
 * @brief    A function pointer for a task in `CF_Threadpool`.
 * @param    param      Can be `NULL`. This is passed to the task, and comes from `cf_threadpool_add_task`.
 * @remarks  Threadpools are an advanced topic. You've been warned! John has a [good article on threadpools](https://nachtimwald.com/2019/04/12/thread-pool-in-c/).
 *           A task is a single function that a thread in the threadpool will run. Usually they perform one chunk of work, and then
 *           return. Often a task is defined as a bunch of processing that doesn't share any data external to the task.
 * @related  CF_TaskFn cf_make_threadpool cf_destroy_threadpool cf_threadpool_add_task cf_threadpool_kick_and_wait cf_threadpool_kick
 */
typedef void (CF_CALL CF_TaskFn)(void* param);

/**
 * @function cf_make_threadpool
 * @category multithreading
 * @brief    Returns an opaque `CF_Threadpool` pointer.
 * @param    thread_count  The number of threads to spawn within the internal pool.
 * @remarks  Call `cf_destroy_threadpool` when done. A threadpool manages a set of threads. Each thread sleeps until a task is placed
 *           into the threadpool (see: `CF_TaskFn`). Once the task is completed, the thread attempts to fetch another task. If no more
 *           tasks are available, the thread goes back to sleep. A common tactic is to take the number of cores in a given CPU and
 *           subtract one, then use this number for `thread_count`. We subtract one to account for the main thread.
 * @related  CF_TaskFn cf_make_threadpool cf_destroy_threadpool cf_threadpool_add_task cf_threadpool_kick_and_wait cf_threadpool_kick
 */
CF_API CF_Threadpool* CF_CALL cf_make_threadpool(int thread_count);

/**
 * @function cf_destroy_threadpool
 * @category multithreading
 * @brief    Destroys a `CF_Threadpool` created by `cf_make_threadpool`.
 * @param    pool       The pool.
 * @related  CF_TaskFn cf_make_threadpool cf_destroy_threadpool cf_threadpool_add_task cf_threadpool_kick_and_wait cf_threadpool_kick
 */
CF_API void CF_CALL cf_destroy_threadpool(CF_Threadpool* pool);

/**
 * @function cf_threadpool_add_task
 * @category multithreading
 * @brief    Adds a `CF_TaskFn` to the threadpool.
 * @param    pool       The pool.
 * @param    task       The task for a thread in the pool to perform.
 * @param    param      Can be `NULL`. This gets handed to the `CF_TaskFn` when it gets called.
 * @remarks  Once a task is added to the pool `cf_threadpool_kick_and_wait` or `cf_threadpool_kick` must be called wake threads. Once
 *           awake, threads will process the tasks. The order of start/finish for the tasks is not deterministic.
 * @related  CF_TaskFn cf_make_threadpool cf_destroy_threadpool cf_threadpool_add_task cf_threadpool_kick_and_wait cf_threadpool_kick
 */
CF_API void CF_CALL cf_threadpool_add_task(CF_Threadpool* pool, CF_TaskFn* task, void* param);

/**
 * @function cf_threadpool_kick_and_wait
 * @category multithreading
 * @brief    Tells the internal threads to wake and start processing tasks, and blocks until all tasks are done.
 * @param    pool       The pool.
 * @remarks  This function will block until all tasks are completed.
 * @related  CF_TaskFn cf_make_threadpool cf_destroy_threadpool cf_threadpool_add_task cf_threadpool_kick_and_wait cf_threadpool_kick
 */
CF_API void CF_CALL cf_threadpool_kick_and_wait(CF_Threadpool* pool);

/**
 * @function cf_threadpool_kick
 * @category multithreading
 * @brief    Tells the internal threads to wake and start processing tasks without blocking.
 * @param    pool       The pool.
 * @remarks  This function will _not_ block. It immediately returns after signaling the threads in the pool to wake.
 * @related  CF_TaskFn cf_make_threadpool cf_destroy_threadpool cf_threadpool_add_task cf_threadpool_kick_and_wait cf_threadpool_kick
 */
CF_API void CF_CALL cf_threadpool_kick(CF_Threadpool* pool);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

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

CF_INLINE Mutex make_mutex() { return cf_make_mutex(); }
CF_INLINE void destroy_mutex(Mutex* mutex) { cf_destroy_mutex(mutex); }
CF_INLINE Result mutex_lock(Mutex* mutex) { return cf_mutex_lock(mutex); }
CF_INLINE Result mutex_unlock(Mutex* mutex) { return cf_mutex_unlock(mutex); }
CF_INLINE bool Mutexrylock(Mutex* mutex) { return cf_mutex_try_lock(mutex); }

CF_INLINE ConditionVariable make_cv() { return cf_make_cv(); }
CF_INLINE void destroy_cv(ConditionVariable* cv) { cf_destroy_cv(cv); }
CF_INLINE Result cv_wake_all(ConditionVariable* cv) { return cf_cv_wake_all(cv); }
CF_INLINE Result cv_wake_one(ConditionVariable* cv) { return cf_cv_wake_one(cv); }
CF_INLINE Result cv_wait(ConditionVariable* cv, Mutex* mutex) { return cf_cv_wait(cv, mutex); }

CF_INLINE Semaphore make_sem(int initial_count) { return cf_make_sem(initial_count); }
CF_INLINE void destroy_sem(Semaphore* semaphore) { cf_destroy_sem(semaphore); }
CF_INLINE Result sem_post(Semaphore* semaphore) { return cf_sem_post(semaphore); }
CF_INLINE Result sem_try(Semaphore* semaphore) { return cf_sem_try(semaphore); }
CF_INLINE Result sem_wait(Semaphore* semaphore) { return cf_sem_wait(semaphore); }
CF_INLINE Result sem_value(Semaphore* semaphore) { return cf_sem_value(semaphore); }

CF_INLINE Thread* thread_create(ThreadFn func, const char* name, void* udata) { return cf_thread_create(func, name, udata); }
CF_INLINE void thread_detach(Thread* thread) { cf_thread_detach(thread); }
CF_INLINE ThreadId thread_get_id(Thread* thread) { return cf_thread_get_id(thread); }
CF_INLINE ThreadId thread_id() { return cf_thread_id(); }
CF_INLINE Result thread_wait(Thread* thread) { return cf_thread_wait(thread); }

CF_INLINE int core_count() { return cf_core_count(); }
CF_INLINE int cacheline_size() { return cf_cacheline_size(); }

CF_INLINE AtomicInt atomic_zero() { return cf_atomic_zero(); }
CF_INLINE int atomic_add(AtomicInt* atomic, int addend) { return cf_atomic_add(atomic, addend); }
CF_INLINE int atomic_set(AtomicInt* atomic, int value) { return cf_atomic_set(atomic, value); }
CF_INLINE int atomic_get(AtomicInt* atomic) { return cf_atomic_get(atomic); }
CF_INLINE Result atomic_cas(AtomicInt* atomic, int expected, int value) { return cf_atomic_cas(atomic, expected, value); }
CF_INLINE void* atomic_ptr_set(void** atomic, void* value) { return cf_atomic_ptr_set(atomic, value); }
CF_INLINE void* atomic_ptr_get(void** atomic) { return cf_atomic_ptr_get(atomic); }
CF_INLINE Result atomic_ptr_cas(void** atomic, void* expected, void* value) { return cf_atomic_ptr_cas(atomic, expected, value); }

CF_INLINE ReadWriteLock make_rw_lock() { return cf_make_rw_lock(); }
CF_INLINE void destroy_rw_lock(ReadWriteLock* rw) { cf_destroy_rw_lock(rw); }
CF_INLINE void read_lock(ReadWriteLock* rw) { cf_read_lock(rw); }
CF_INLINE void read_unlock(ReadWriteLock* rw) { cf_read_unlock(rw); }
CF_INLINE void write_lock(ReadWriteLock* rw) { cf_write_lock(rw); }
CF_INLINE void write_unlock(ReadWriteLock* rw) { cf_write_unlock(rw); }

CF_INLINE Threadpool* make_threadpool(int thread_count) { return cf_make_threadpool(thread_count); }
CF_INLINE void destroy_threadpool(Threadpool* pool) { return cf_destroy_threadpool(pool); }
CF_INLINE void threadpool_add_task(Threadpool* pool, TaskFn* task, void* param) { return cf_threadpool_add_task(pool, task, param); }
CF_INLINE void threadpool_kick_and_wait(Threadpool* pool) { return cf_threadpool_kick_and_wait(pool); }
CF_INLINE void threadpool_kick(Threadpool* pool) { return cf_threadpool_kick(pool); }

}

#endif // CF_CPP

#endif // CF_MULTITHREADING_H
