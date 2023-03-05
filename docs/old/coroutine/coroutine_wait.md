# coroutine_wait

Pauses the coroutine for a number of seconds.

## Syntax

```cpp
error_t coroutine_wait(coroutine_t* co, float seconds);
```

## Function Parameters

Parameter Name | Description
--- | ---
co | The coroutine to pause.
seconds | The number of seconds to pause.

## Return Value

Returns errors upon failure.

## Remarks

Any calls to [coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_resume.md) will be blocked from actually resuming until `seconds` have elapsed. The `dt` value passed to `coroutine_resume` will increment an internal timer, and once the timer goes over `seconds` the coroutine will be successfully resumed.

## Related Functions

[coroutine_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_make.md)  
[coroutine_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_destroy.md)  
[coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_yield.md)  
[coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_resume.md)  
[coroutine_currently_running](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_currently_running.md)  
