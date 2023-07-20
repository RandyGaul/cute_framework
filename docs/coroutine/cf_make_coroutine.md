[](../header.md ':include')

# cf_make_coroutine

Category: [coroutine](/api_reference?id=coroutine)  
GitHub: [cute_coroutine.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_coroutine.h)  
---

Sets up the entry point for a coroutine to start.

```cpp
CF_Coroutine cf_make_coroutine(CF_CoroutineFn* fn, int stack_size, void* udata);
```

Parameters | Description
--- | ---
fn | The entry point (function) the coroutine runs.
stack_size | The size of the coroutine's stack to call functions and make local variables.
udata | Can be `NULL`. Gets handed back to you when [cf_coroutine_get_udata](/coroutine/cf_coroutine_get_udata.md) is called.

## Remarks

The coroutine starts in a `COROUTINE_STATE_SUSPENDED`, and won't run until [cf_coroutine_resume](/coroutine/cf_coroutine_resume.md) is first called. Free up the
coroutine with [cf_destroy_coroutine](/coroutine/cf_destroy_coroutine.md) when done. See [CF_Coroutine](/coroutine/cf_coroutine.md) for some more details.

## Related Pages

[CF_Coroutine](/coroutine/cf_coroutine.md)  
[CF_CoroutineFn](/coroutine/cf_coroutinefn.md)  
[CF_CoroutineState](/coroutine/cf_coroutinestate.md)  
[cf_coroutine_currently_running](/coroutine/cf_coroutine_currently_running.md)  
[cf_destroy_coroutine](/coroutine/cf_destroy_coroutine.md)  
[cf_coroutine_state_to_string](/coroutine/cf_coroutine_state_to_string.md)  
[cf_coroutine_resume](/coroutine/cf_coroutine_resume.md)  
[cf_coroutine_yield](/coroutine/cf_coroutine_yield.md)  
[cf_coroutine_state](/coroutine/cf_coroutine_state.md)  
[cf_coroutine_get_udata](/coroutine/cf_coroutine_get_udata.md)  
[cf_coroutine_push](/coroutine/cf_coroutine_push.md)  
[cf_coroutine_pop](/coroutine/cf_coroutine_pop.md)  
[cf_coroutine_bytes_pushed](/coroutine/cf_coroutine_bytes_pushed.md)  
[cf_coroutine_space_remaining](/coroutine/cf_coroutine_space_remaining.md)  
