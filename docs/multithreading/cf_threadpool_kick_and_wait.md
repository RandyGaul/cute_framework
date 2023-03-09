[](../header.md ':include')

# cf_threadpool_kick_and_wait

Category: [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Tells the internal threads to wake and start processing tasks, and blocks until all tasks are done.

```cpp
void cf_threadpool_kick_and_wait(CF_Threadpool* pool);
```

Parameters | Description
--- | ---
pool | The pool.

## Remarks

This function will block until all tasks are completed.

## Related Pages

[CF_TaskFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_taskfn.md)  
[cf_make_threadpool](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_threadpool.md)  
[cf_destroy_threadpool](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_threadpool.md)  
[cf_threadpool_add_task](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_add_task.md)  
[cf_threadpool_kick](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_kick.md)  
