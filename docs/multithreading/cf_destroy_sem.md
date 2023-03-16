[](../header.md ':include')

# cf_destroy_sem

Category: [multithreading](/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Destroys a [CF_Semaphore](/multithreading/cf_semaphore.md) made by [cf_make_sem](/multithreading/cf_make_sem.md).

```cpp
void cf_destroy_sem(CF_Semaphore* semaphore);
```

Parameters | Description
--- | ---
semaphore | The semaphore.

## Related Pages

[CF_Semaphore](/multithreading/cf_semaphore.md)  
[cf_make_sem](/multithreading/cf_make_sem.md)  
[cf_sem_value](/multithreading/cf_sem_value.md)  
[cf_sem_post](/multithreading/cf_sem_post.md)  
[cf_sem_try](/multithreading/cf_sem_try.md)  
[cf_sem_wait](/multithreading/cf_sem_wait.md)  
