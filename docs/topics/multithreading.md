# Multithreading

Multithreading is a rather advanced topic. Many games need zero, or near-zero, multithreading. Multithreading is purely an optimization topic, only necessary to try and run multiple cores on a single CPU in parallel. Some good candidate areas for multithreading include collision detection, loading resources, the network stack, block updating large chunks of objects/entities, particle systems, etc.

Multithreading is all about understanding _synchronization primitives_, the various types available for synchronizing data from one _thread of execution_ to another. CF has a variety of primitives available.

We will briefly go over each one and link to some recommended readings if you want to learn more.

See also: [Atomics](atomics.md).

## Thread

A thread is the thing that runs your code. By default the application starts up in the _main thread_. The application can spin up many different threads as-necessary. Threads are best used to perform isolated tasks that can be done completely independent from any other task. You may spawn another thread with [`cf_thread_create`](https://randygaul.github.io/cute_framework/#/multithreading/cf_thread_create). It will call your thread function. When your thread function returns, the thread can then be cleaned up with [`cf_thread_wait`](https://randygaul.github.io/cute_framework/#/multithreading/cf_thread_wait).

```cpp
int my_thread_fn(void udata)
{
	CF_UNUSED(udata);
	printf("The test thread ran.\n");
	return 0;
}

void spawn_thread()
{
	CF_Thread* thread = cf_thread_create(my_thread_fn, "My Test Thread", NULL);
	printf("Thread spawned.\n");
	cf_thread_wait(thread);
}
```

When `spawn_thread` is called it's unclear which of the following will print:

```
Thread spawned.
The test thread ran.
```

Or:

```
The test thread ran.
Thread spawned.
```

Depending on the operating system's thread scheduling logic, it's entirely possible for the order of print statements to flip from one application run to another. To control the order we would have to use a _synchronization primitive_, such as an [atomics](atomics.md).

If the underlying system has multiple CPU cores, then separate threads can be scheduled by the operating system to actually run in parallel. However, even if the system has a single core (meaning it is single-threaded) you can usually still create threads perfectly fine -- they just won't give any performance boost, and instead usually cause a slight performance cost.

A great use case for threads in games is asynchronous resource loading. For further reading Beej has an excellent article on [Threads and Mutexes](https://beej.us/guide/bgc/html/split/multithreading.html).

## Condition Variable

A [`CF_ConditionVariable`](https://randygaul.github.io/cute_framework/#/multithreading/cf_conditionvariable) is used to put threads to sleep. The condition variable can wake a single thread, or wake all the threads. Usually you must pair the condition variable with some extra state to make it really useful, such as an integer (e.g. to implement a semaphore, more on this later), or a boolean. For further reading you can try [this article by IBM](https://www.ibm.com/docs/en/aix/7.2?topic=programming-using-condition-variables) on condition variables.

Usually condition variables are not used on their own, and higher level primitives are instead used, such as thread pools or semaphores (more on these later).

## Mutex

The mutex, aka lock ([`CF_Mutex`](http://localhost:3000/#/multithreading/cf_mutex), stands for "mutual exclusion". It's used as a basic _synchronization primitive_ for two threads to communicate with one another. The idea is the mutex can be acquired by only a single thread at any given time. If any other thread attempts to aquire the mutex (lock it), the thread will sleep and wait until the lock can be acquired.

Sometimes locks are used on their own, but generally locks are considered difficult to use, tedious, and a bit error-prone. It's recommended to instead use a thread pool if possible. But, if you really know what you're doing sometimes a simple mutex is preferable. For further reading Beej has an excellent article on [Threads and Mutexes](https://beej.us/guide/bgc/html/split/multithreading.html).

## Read/Write Lock

Similar to mutex, a read write lock is a bit higher level ([`CF_ReadWriteLock`](http://localhost:3000/#/multithreading/cf_readwritelock)). It supports many simultaneous readers, but only a single writer at a time (the write excludes other readers as well). Usually the read write lock is used to implement other tools or data structures.

For example, a multi-threaded hash table can be constructed with a read write lock. Most of the time the hash table can operate in read-only mode. We can use a read write lock for the entire table. Many simultaneous readers can fetch or lookup keys freely from the table. If the table needs to insert an element we can then write lock the table and perform the insertion. This will wait for all readers to exitb before doing any writing. To take this a step further, if the hash table uses chained collision resolution another read write lock can be used for each collision chain. As chains are updated, only an individual chain needs to be write-locked at a given time. If the table becomes saturated it can expand it's memory size with a single global write-lock, of which would write-lock all of the chains before expansion. This would completely wait for all readers to exit before expansion.

## Semaphore

A semaphore is a curious _synchronization primitive_ used sort of like a mutex, but instead of mutual exclusion it's used to restrict access to N resources. The "resource" is abstract, and represented by an integer initialized to N. Acquiring the semaphore decrements the integer. If it is greater than zero the resource is acquired, otherwise the thread is put to sleep until the semaphore becomes greater than zero again.

A common use of semaphores is to implement a thread pool (see below). CF actually implements a thread pool with the help of a semaphore. There are of course other uses cases, but they aren't all that common. For further reading see [The Little Book of Semaphores](https://greenteapress.com/wp/semaphores/).

## Thread Pool

The thread pool is a great tool for games. For a CPU with N cores the thread pool is initialized with N-1 threads (N-1 to account for the main thread as well). The threads are initially asleep. The thread pool can then be loaded up with _tasks_ (each represented by a function pointer and `void*` pair [`CF_TaskFn`])(https://randygaul.github.io/cute_framework/#/multithreading/cf_taskfn)).

After loading up the threadpool with tasks they can be kicked off. Once kicked, threads will wake and grab tasks from the pool and perform them until all tasks are complete, and the threads go back to sleep. The pool can be kicked off in two styles: _blocking_ and _non-blocking_.

[`cf_threadpool_kick_and_wait`](../multithreading/cf_threadpool_kick_and_wait.md) will kick off all tasks and return only once all the tasks are completed. In this way it is a _blocking_ function, as it blocks the thread's execution until it finishes. If you'd like to continue on while the tasks are performed, use [`cf_threadpool_kick`](https://randygaul.github.io/cute_framework/#/multithreading/cf_threadpool_kick), as it's a _non-blocking_ function, meaning the function will immediately return after kicking, without waiting for any tasks to complete.

Great uses cases for threadpools in games include perform collision checks, as well as block-updating large chunks of independent entities/objects/systems.
