# cf_cv_wait | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/README.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

Causes the calling thread to wait on the condition variable.

```cpp
CF_Result cf_cv_wait(CF_ConditionVariable* cv, CF_Mutex* mutex);
```

Parameters | Description
--- | ---
cv | The condition variable.
mutex | The mutex used to access the condition variable.

## Return Value

Returns any errors as a [CF_Result](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_result.md).

## Remarks

The thread will not wake until [cf_cv_wake_all](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_cv_wake_all.md) or [cf_cv_wake_one](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_cv_wake_one.md) is called.

## Related Pages

[CF_ConditionVariable](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_conditionvariable.md)  
[cf_make_cv](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_make_cv.md)  
[cf_destroy_cv](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_destroy_cv.md)  
[cf_cv_wake_all](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_cv_wake_all.md)  
[cf_cv_wake_one](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_cv_wake_one.md)  
