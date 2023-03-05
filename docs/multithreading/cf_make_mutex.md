# cf_make_mutex | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading_readme.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

Returns an unlocked [CF_Mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex.md).

```cpp
CF_Mutex cf_make_mutex();
```

## Remarks

Destroy the mutex with [cf_destroy_mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_mutex.md) when done.

## Related Pages

[CF_Mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex.md)  
[cf_mutex_try_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex_try_lock.md)  
[cf_destroy_mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_mutex.md)  
[cf_mutex_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex_lock.md)  
[cf_mutex_unlock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex_unlock.md)  
