# cf_make_threadpool | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading_readme.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

Returns an opaque [CF_Threadpool](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool.md) pointer.

```cpp
CF_Threadpool* cf_make_threadpool(int thread_count);
```

Parameters | Description
--- | ---
thread_count | The number of threads to spawn within the internal pool.

## Remarks

Call [cf_destroy_threadpool](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_threadpool.md) when done. A threadpool manages a set of threads. Each thread sleeps until a task is placed
into the threadpool (see: [CF_TaskFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_taskfn.md)). Once the task is completed, the thread attempts to fetch another task. If no more
tasks are available, the thread goes back to sleep. A common tactic is to take the number of cores in a given CPU and
subtract one, then use this number for `thread_count`. We subtract one to account for the main thread.

## Related Pages

[CF_TaskFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_taskfn.md)  
[cf_threadpool_kick](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_kick.md)  
[cf_destroy_threadpool](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_threadpool.md)  
[cf_threadpool_add_task](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_add_task.md)  
[cf_threadpool_kick_and_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_kick_and_wait.md)  
