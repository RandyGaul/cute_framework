# cf_sem_post | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading_readme.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

Increments the semaphore's counter and wakes one thread if the counter becomes greater than zero.

```cpp
CF_Result cf_sem_post(CF_Semaphore* semaphore);
```

Parameters | Description
--- | ---
semaphore | The semaphore.

## Remarks

As other threads call [cf_sem_try](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_try.md) or [cf_sem_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_wait.md) they decrement the semaphore's counter. Eventually
the counter will become zero, causing additional threads to wait (sleep). When the resources become
available again, this function is used to wake one up.

## Related Pages

[CF_Semaphore](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_semaphore.md)  
[cf_make_sem](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_sem.md)  
[cf_destroy_sem](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_sem.md)  
[cf_sem_value](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_value.md)  
[cf_sem_try](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_try.md)  
[cf_sem_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_wait.md)  
