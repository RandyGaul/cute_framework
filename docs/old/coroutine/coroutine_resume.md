# coroutine_resume

Resumes a paused coroutine.

## Syntax

```cpp
error_t coroutine_resume(coroutine_t* co, float dt = 0);
```

## Function Parameters

Parameter Name | Description
--- | ---
co | The coroutine to resume.
dt | An optional time since the function was last resumed. This is useful for certain coroutines that run once per game tick.

## Return Value

Returns errors upon failure.

## Remarks

Coroutines are paused when initially constructed, or when they call [coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_yield.md). Calling `coroutine_resume` will unpause the function.

## Related Functions

[coroutine_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_make.md)  
[coroutine_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_destroy.md)  
[coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_yield.md)  
[coroutine_currently_running](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_currently_running.md)  
