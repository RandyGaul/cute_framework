# cf_sem_try | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading_readme.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

Attempts to decrement the semaphore's counter without sleeping, and returns success if decremented.

```cpp
CF_Result cf_sem_try(CF_Semaphore* semaphore);
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

[CF_Semaphore](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_semaphore.md)  
[cf_make_sem](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_sem.md)  
[cf_destroy_sem](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_sem.md)  
[cf_sem_post](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_post.md)  
[cf_sem_value](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_value.md)  
[cf_sem_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_wait.md)  
