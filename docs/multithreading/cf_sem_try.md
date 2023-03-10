[](../header.md ':include')

# cf_sem_try

Category: [multithreading](/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Attempts to decrement the semaphore's counter without sleeping, and returns success if decremented.

```cpp
CF_API CF_Result CF_CALL cf_sem_try(CF_Semaphore* semaphore);
```

Parameters | Description
--- | ---
semaphore | The semaphore.

## Return Value

This function will not cause the thread to sleep if the semaphore's counter is zero. Instead, an error will
be returned. Success is returned if the semaphore was successfully acquired and the counter was decremented.

## Remarks

Since this function does not block/sleep, it allows the thread to continue running, even if the return
value was an error. This lets a thread poll the semaphore instead of blocking/sleeping.

## Related Pages

[CF_Semaphore](/multithreading/cf_semaphore.md)  
[cf_make_sem](/multithreading/cf_make_sem.md)  
[cf_destroy_sem](/multithreading/cf_destroy_sem.md)  
[cf_sem_post](/multithreading/cf_sem_post.md)  
[cf_sem_value](/multithreading/cf_sem_value.md)  
[cf_sem_wait](/multithreading/cf_sem_wait.md)  
