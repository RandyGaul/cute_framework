# cf_thread_create | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/README.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

Creates a new thread and runs it's thread function ([CF_ThreadFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadfn.md)).

```cpp
CF_Thread* cf_thread_create(CF_ThreadFn func, const char* name, void* udata);
```

Parameters | Description
--- | ---
func | The function to run for the thread.
name | The name of this thread. Must be unique.
udata | Can be `NULL`. This gets handed back to you in your `func`.

## Return Value

Returns an opaque pointer to [CF_Thread](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread.md).

## Code Example

> Example syntax of a thread.

```cpp
int MyThreadFn(void udata)
{
    // Do work here...
    return 0;
}
```

## Remarks

Unless you call [cf_thread_detach](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_detach.md) you should call [cf_thread_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_wait.md) from another thread to
clean up resources and get the thread's return value back. It is considered a leak otherwise.

## Related Pages

[CF_Thread](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread.md)  
[CF_ThreadFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_threadfn.md)  
[cf_thread_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_wait.md)  
[cf_thread_detach](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_detach.md)  
[cf_thread_get_id](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_get_id.md)  
[cf_thread_id](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_thread_id.md)  
