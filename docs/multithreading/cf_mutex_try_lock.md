# cf_mutex_try_lock | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/README.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

Attempts to lock a [CF_Mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex.md) without waiting.

```cpp
bool cf_mutex_try_lock(CF_Mutex* mutex);
```

Parameters | Description
--- | ---
mutex | The mutex.

## Return Value

Returns true if the lock was acquired, and false if the lock was already locked.

## Related Pages

[CF_Mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex.md)  
[cf_make_mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_mutex.md)  
[cf_destroy_mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_mutex.md)  
[cf_mutex_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex_lock.md)  
[cf_mutex_unlock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex_unlock.md)  
