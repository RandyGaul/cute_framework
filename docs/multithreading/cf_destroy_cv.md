[](../header.md ':include')

# cf_destroy_cv

Category: [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Destroys a [CF_ConditionVariable](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_conditionvariable.md) created with [cf_make_cv](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_cv.md).

```cpp
void cf_destroy_cv(CF_ConditionVariable* cv);
```

Parameters | Description
--- | ---
cv | The condition variable.

## Related Pages

[CF_ConditionVariable](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_conditionvariable.md)  
[cf_make_cv](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_cv.md)  
[cf_cv_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_cv_wait.md)  
[cf_cv_wake_all](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_cv_wake_all.md)  
[cf_cv_wake_one](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_cv_wake_one.md)  
