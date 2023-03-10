[](../header.md ':include')

# cf_make_mutex

Category: [multithreading](/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Returns an unlocked [CF_Mutex](/multithreading/cf_mutex.md).

```cpp
CF_API CF_Mutex CF_CALL cf_make_mutex();
```

## Remarks

Destroy the mutex with [cf_destroy_mutex](/multithreading/cf_destroy_mutex.md) when done.

## Related Pages

[CF_Mutex](/multithreading/cf_mutex.md)  
[cf_mutex_try_lock](/multithreading/cf_mutex_try_lock.md)  
[cf_destroy_mutex](/multithreading/cf_destroy_mutex.md)  
[cf_mutex_lock](/multithreading/cf_mutex_lock.md)  
[cf_mutex_unlock](/multithreading/cf_mutex_unlock.md)  
