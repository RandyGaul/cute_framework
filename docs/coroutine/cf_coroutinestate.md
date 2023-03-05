# CF_CoroutineState | [coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine_readme.md) | [cute_coroutine.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_coroutine.h)

The states of a coroutine.

## Values

Enum | Description
--- | ---
COROUTINE_STATE_DEAD | The coroutine has stopped running entirely.
COROUTINE_STATE_ACTIVE_AND_RUNNING | The coroutine is not dead, and currently running.
COROUTINE_STATE_ACTIVE_BUT_RESUMED_ANOTHER | The coroutine is not dead, but has called [cf_coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_resume.md) on another coroutine.
COROUTINE_STATE_SUSPENDED | The coroutine is not dead, but is not active since it called [cf_coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_yield.md).

## Related Pages

[cf_make_app](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_make_app.md)  
[cf_destroy_app](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_destroy_app.md)  
