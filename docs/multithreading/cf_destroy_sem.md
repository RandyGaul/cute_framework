[](../header.md ':include')

# cf_destroy_sem

Category: [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Destroys a [CF_Semaphore](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_semaphore.md) made by [cf_make_sem](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_sem.md).

```cpp
void cf_destroy_sem(CF_Semaphore* semaphore);
```

Parameters | Description
--- | ---
semaphore | The semaphore.

## Related Pages

[CF_Semaphore](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_semaphore.md)  
[cf_make_sem](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_sem.md)  
[cf_sem_value](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_value.md)  
[cf_sem_post](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_post.md)  
[cf_sem_try](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_try.md)  
[cf_sem_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_sem_wait.md)  
