[](../header.md ':include')

# cf_threadpool_kick

Category: [multithreading](/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Tells the internal threads to wake and start processing tasks without blocking.

```cpp
CF_API void CF_CALL cf_threadpool_kick(CF_Threadpool* pool);
```

Parameters | Description
--- | ---
pool | The pool.

## Remarks

This function will _not_ block. It immediately returns after signaling the threads in the pool to wake.

## Related Pages

[CF_TaskFn](/multithreading/cf_taskfn.md)  
[cf_make_threadpool](/multithreading/cf_make_threadpool.md)  
[cf_destroy_threadpool](/multithreading/cf_destroy_threadpool.md)  
[cf_threadpool_add_task](/multithreading/cf_threadpool_add_task.md)  
[cf_threadpool_kick_and_wait](/multithreading/cf_threadpool_kick_and_wait.md)  
