# cf_coroutine_currently_running | [coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/README.md) | [cute_coroutine.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_coroutine.h)

Returns the opaque pointer to the coroutine currently running.

```cpp
CF_Coroutine* cf_coroutine_currently_running();
```

## Remarks

Each coroutine has ther `co` pointer handed to them as the only parameter in [CF_CoroutineFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutinefn.md), so you likely
already have access to the coroutine pointer. However, this function is made available here for convenience.
For example, your coroutines may call into other functions -- instead of passing around a `co` pointer everywhere,
your helper functions can simply fetch the [CF_Coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine.md) pointer themselves on an as-needed basis by calling
this function.

## Related Pages

[CF_Coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine.md)  
[CF_CoroutineFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutinefn.md)  
[CF_CoroutineState](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutinestate.md)  
[cf_make_coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_make_coroutine.md)  
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
