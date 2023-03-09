# cf_thread_wait

Category: [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Waits and blocks/sleeps until the thread exits, and returns the thread's return result.

```cpp
CF_Result cf_thread_wait(CF_Thread* thread);
```

Parameters | Description
--- | ---
thread | The thread.

## Remarks

It is invalid to call this function on a detached thread (see [cf_thread_detach](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_detach.md)). It is invalid to
call this function on a thread more than once.

## Related Pages

[CF_Thread](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread.md)  
[CF_ThreadFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadfn.md)  
[cf_thread_create](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_create.md)  
[cf_thread_detach](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_detach.md)  
[cf_thread_get_id](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_get_id.md)  
[cf_thread_id](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_id.md)  
