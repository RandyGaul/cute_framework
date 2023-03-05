# CF_TaskFn | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/README.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

A function pointer for a task in [CF_Threadpool](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool.md).

```cpp
typedef void (CF_TaskFn)(void* param);
```

Parameters | Description
--- | ---
param | Can be `NULL`. This is passed to the task, and comes from [cf_threadpool_add_task](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_add_task.md).

## Remarks

Threadpools are an advanced topic. You've been warned! John has a [good article on threadpools](https://nachtimwald.com/2019/04/12/thread-pool-in-c/).
A task is a single function that a thread in the threadpool will run. Usually they perform one chunk of work, and then
return. Often a task is defined as a bunch of processing that doesn't share any data external to the task.

## Related Pages

[cf_threadpool_kick](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_kick.md)  
[cf_make_threadpool](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_threadpool.md)  
[cf_destroy_threadpool](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_threadpool.md)  
[cf_threadpool_add_task](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_add_task.md)  
[cf_threadpool_kick_and_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_kick_and_wait.md)  
