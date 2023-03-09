[](../header.md ':include')

# cf_sem_value

Category: [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Atomically fetches the semaphore's counter.

```cpp
CF_Result cf_sem_value(CF_Semaphore* semaphore);
```

Parameters | Description
--- | ---
semaphore | The semaphore.

## Return Value

Returns any errors upon failure.

## Related Pages

[CF_Semaphore](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_semaphore.md)  
[cf_make_sem](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_sem.md)  
[cf_destroy_sem](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_sem.md)  
[cf_sem_post](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_post.md)  
[cf_sem_try](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_try.md)  
[cf_sem_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_wait.md)  
