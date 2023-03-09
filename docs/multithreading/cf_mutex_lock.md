[](../header.md ':include')

# cf_mutex_lock

Category: [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Locks a [CF_Mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex.md).

```cpp
CF_Result cf_mutex_lock(CF_Mutex* mutex);
```

Parameters | Description
--- | ---
mutex | The mutex.

## Return Value

Returns any errors as a [CF_Result](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_result.md).

## Remarks

Will cause the thread to wait until the lock is available if it's currently locked.

## Related Pages

[CF_Mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex.md)  
[cf_make_mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_mutex.md)  
[cf_destroy_mutex](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_mutex.md)  
[cf_mutex_try_lock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex_try_lock.md)  
[cf_mutex_unlock](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_mutex_unlock.md)  
