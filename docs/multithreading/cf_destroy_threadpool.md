[](../header.md ':include')

# cf_destroy_threadpool

Category: [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Destroys a [CF_Threadpool](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool.md) created by [cf_make_threadpool](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_threadpool.md).

```cpp
void cf_destroy_threadpool(CF_Threadpool* pool);
```

Parameters | Description
--- | ---
pool | The pool.

## Related Pages

[CF_TaskFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_taskfn.md)  
[cf_make_threadpool](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_threadpool.md)  
[cf_threadpool_kick](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_kick.md)  
[cf_threadpool_add_task](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_add_task.md)  
[cf_threadpool_kick_and_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadpool_kick_and_wait.md)  
