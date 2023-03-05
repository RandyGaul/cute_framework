# cf_make_coroutine | [coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine_readme.md) | [cute_coroutine.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_coroutine.h)

Sets up the entry point for a coroutine to start.

```cpp
CF_Coroutine* cf_make_coroutine(CF_CoroutineFn* fn, int stack_size, void* udata);
```

Parameters | Description
--- | ---
fn | The entry point (function) the coroutine runs.
stack_size | The size of the coroutine's stack to call functions and make local variables.
udata | Can be `NULL`. Gets handed back to you when [cf_coroutine_get_udata](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_get_udata.md) is called.

## Remarks

The coroutine starts in a `COROUTINE_STATE_SUSPENDED`, and won't run until [cf_coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_resume.md) is first called. Free up the
coroutine with [cf_destroy_coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_destroy_coroutine.md) when done. See [CF_Coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine.md) for some more details.

## Related Pages

[CF_Coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine.md)  
[CF_CoroutineFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutinefn.md)  
[CF_CoroutineState](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutinestate.md)  
[cf_coroutine_currently_running](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_currently_running.md)  
[cf_destroy_coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_destroy_coroutine.md)  
[cf_coroutine_state_to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_state_to_string.md)  
[cf_coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_resume.md)  
[cf_coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_yield.md)  
[cf_coroutine_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_state.md)  
[cf_coroutine_get_udata](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_get_udata.md)  
[cf_coroutine_push](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_push.md)  
[cf_coroutine_pop](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_pop.md)  
[cf_coroutine_bytes_pushed](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_bytes_pushed.md)  
[cf_coroutine_space_remaining](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_space_remaining.md)  
