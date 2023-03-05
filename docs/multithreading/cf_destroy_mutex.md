# cf_destroy_mutex | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/README.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

Destroys a [CF_Mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex.md) created with [cf_make_mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_mutex.md).

```cpp
void cf_destroy_mutex(CF_Mutex* mutex);
```

Parameters | Description
--- | ---
mutex | The mutex.

## Related Pages

[CF_Mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex.md)  
[cf_make_mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_mutex.md)  
[cf_mutex_try_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex_try_lock.md)  
[cf_mutex_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex_lock.md)  
[cf_mutex_unlock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex_unlock.md)  
