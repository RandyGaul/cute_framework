# coroutine_state

Returns the state of the coroutine.

## Syntax

```cpp
coroutine_state_t coroutine_state(coroutine_t* co);
```

## Function Parameters

Parameter Name | Description
--- | ---
co | The coroutine.

## Return Value

Returns the state of the coroutine. Here are the various states for the enum `coroutine_state_t`.

Enumeration Entry | Description
--- | ---
COROUTINE_STATE_DEAD | The coroutine has returned, or was destroyed, and can not be resumed.
COROUTINE_STATE_ACTIVE_AND_RUNNING | The coroutine is active and running.
COROUTINE_STATE_ACTIVE_BUT_RESUMED_ANOTHER | The coroutine is active, but has called [coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_resume.md) on another coroutine.
COROUTINE_STATE_SUSPENDED | The coroutine is paused (either paused from a called to [coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_yield.md), or has not started running yet), ready to be resumed by calling [coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_resume.md).

## Related Functions

[coroutine_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_make.md)  
[coroutine_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_destroy.md)  
[coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_yield.md)  
[coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_resume.md)  
