[](../header.md ':include')

# cf_make_sem

Category: [multithreading](/api_reference?id=multithreading)  
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

[CF_Semaphore](/multithreading/cf_semaphore.md)  
[cf_sem_value](/multithreading/cf_sem_value.md)  
[cf_destroy_sem](/multithreading/cf_destroy_sem.md)  
[cf_sem_post](/multithreading/cf_sem_post.md)  
[cf_sem_try](/multithreading/cf_sem_try.md)  
[cf_sem_wait](/multithreading/cf_sem_wait.md)  
