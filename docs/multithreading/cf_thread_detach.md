[](../header.md ':include')

# cf_thread_detach

Category: [multithreading](/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Makes a special note your thread will never have [cf_thread_wait](/multithreading/cf_thread_wait.md) called on it. Useful as a minor optimization
for long-lived threads.

```cpp
void cf_thread_detach(CF_Thread* thread);
```

Parameters | Description
--- | ---
thread | The thread.

## Remarks

When a thread has [cf_thread_detach](/multithreading/cf_thread_detach.md) called on it, it is no longer necessary to call [cf_thread_wait](/multithreading/cf_thread_wait.md) on it.

## Related Pages

[CF_Thread](/multithreading/cf_thread.md)  
[CF_ThreadFn](/multithreading/cf_threadfn.md)  
[cf_thread_create](/multithreading/cf_thread_create.md)  
[cf_thread_wait](/multithreading/cf_thread_wait.md)  
[cf_thread_get_id](/multithreading/cf_thread_get_id.md)  
[cf_thread_id](/multithreading/cf_thread_id.md)  
