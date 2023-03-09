[](../header.md ':include')

# cf_make_cv

Category: [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Returns an initialized [CF_ConditionVariable](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_conditionvariable.md), used to sleep or wake threads.

```cpp
CF_ConditionVariable cf_make_cv();
```

## Remarks

Destroy the mutex with [cf_destroy_cv](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_cv.md) when done.

## Related Pages

[CF_ConditionVariable](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_conditionvariable.md)  
[cf_cv_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_cv_wait.md)  
[cf_destroy_cv](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_cv.md)  
[cf_cv_wake_all](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_cv_wake_all.md)  
[cf_cv_wake_one](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_cv_wake_one.md)  
