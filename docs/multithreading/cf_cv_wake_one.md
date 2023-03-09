[](../header.md ':include')

# cf_cv_wake_one

Category: [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Wakes a single (implementation-dependent) thread sleeping on the condition variable.

```cpp
CF_Result cf_cv_wake_one(CF_ConditionVariable* cv);
```

Parameters | Description
--- | ---
cv | The condition variable.

## Return Value

Returns any errors as a [CF_Result](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_result.md).

## Related Pages

[CF_ConditionVariable](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_conditionvariable.md)  
[cf_make_cv](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_cv.md)  
[cf_destroy_cv](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_cv.md)  
[cf_cv_wake_all](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_cv_wake_all.md)  
[cf_cv_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_cv_wait.md)  
