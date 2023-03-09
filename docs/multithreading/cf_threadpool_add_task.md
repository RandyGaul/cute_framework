# cf_threadpool_add_task

Category: [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Adds a [CF_TaskFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_taskfn.md) to the threadpool.

```cpp
void cf_threadpool_add_task(CF_Threadpool* pool, CF_TaskFn* task, void* param);
```

Parameters | Description
--- | ---
pool | The pool.
task | The task for a thread in the pool to perform.
param | Can be `NULL`. This gets handed to the [CF_TaskFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_taskfn.md) when it gets called.

## Remarks

Once a task is added to the pool [cf_threadpool_kick_and_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_kick_and_wait.md) or [cf_threadpool_kick](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_kick.md) must be called wake threads. Once
awake, threads will process the tasks. The order of start/finish for the tasks is not deterministic.

## Related Pages

[CF_TaskFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_taskfn.md)  
[cf_make_threadpool](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_threadpool.md)  
[cf_destroy_threadpool](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_threadpool.md)  
[cf_threadpool_kick](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_kick.md)  
[cf_threadpool_kick_and_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_kick_and_wait.md)  
