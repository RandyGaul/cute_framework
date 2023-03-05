# cf_thread_detach | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading_readme.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

Makes a special note your thread will never have [cf_thread_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_wait.md) called on it. Useful as a minor optimization
for long-lived threads.

```cpp
void cf_thread_detach(CF_Thread* thread);
```

Parameters | Description
--- | ---
thread | The thread.

## Remarks

When a thread has [cf_thread_detach](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_detach.md) called on it, it is no longer necessary to call [cf_thread_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_wait.md) on it.

## Related Pages

[CF_Thread](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread.md)  
[CF_ThreadFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadfn.md)  
[cf_thread_create](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_create.md)  
[cf_thread_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_wait.md)  
[cf_thread_get_id](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_get_id.md)  
[cf_thread_id](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_id.md)  
