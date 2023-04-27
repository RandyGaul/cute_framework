# coroutine_yield

Pauses the currently running coroutine.

## Syntax

```cpp
float coroutine_yield(coroutine_t* co);
```

## Function Parameters

Parameter Name | Description
--- | ---
co | The coroutine to pause.

## Return Value

Returns the time value passed through the previously called [coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_resume.md).

## Related Functions

[coroutine_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_make.md)  
[coroutine_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_destroy.md)  
[coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_resume.md)  
[coroutine_wait](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_wait.md)  
[coroutine_currently_running](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_currently_running.md)  
