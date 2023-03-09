# cf_make_sem

Category: [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Returns an initialized semaphore.

```cpp
CF_Semaphore cf_make_sem(int initial_count);
```

Parameters | Description
--- | ---
initial_count | The initial value of the semaphore.

## Remarks

Semaphores are used to prevent race conditions between multiple threads that need to access
common resources. Usually you'll have N resources, and initialize the semaphore to N. This is
a rather advanced and low-level topic, beware. To learn more about semaphores I suggest reading
the online book "The Little Book of Semaphores".

## Related Pages

[CF_Semaphore](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_semaphore.md)  
[cf_sem_value](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_value.md)  
[cf_destroy_sem](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_sem.md)  
[cf_sem_post](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_post.md)  
[cf_sem_try](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_try.md)  
[cf_sem_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_wait.md)  
