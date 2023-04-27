[](../header.md ':include')

# CF_CoroutineState

Category: [coroutine](/api_reference?id=coroutine)  
GitHub: [cute_coroutine.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_coroutine.h)  
---

The states of a coroutine.

## Values

Enum | Description
--- | ---
COROUTINE_STATE_DEAD | The coroutine has stopped running entirely.
COROUTINE_STATE_ACTIVE_AND_RUNNING | The coroutine is not dead, and currently running.
COROUTINE_STATE_ACTIVE_BUT_RESUMED_ANOTHER | The coroutine is not dead, but has called [cf_coroutine_resume](/coroutine/cf_coroutine_resume.md) on another coroutine.
COROUTINE_STATE_SUSPENDED | The coroutine is not dead, but is not active since it called [cf_coroutine_yield](/coroutine/cf_coroutine_yield.md).

## Related Pages

[cf_make_app](/app/cf_make_app.md)  
[cf_destroy_app](/app/cf_destroy_app.md)  
