[](../header.md ':include')

# cf_coroutine_resume

Category: [coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=coroutine)  
GitHub: [cute_coroutine.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_coroutine.h)  
---

Resumes the coroutine.

```cpp
CF_Result cf_coroutine_resume(CF_Coroutine* co);
```

Parameters | Description
--- | ---
co | The coroutine.
return    Returns info on any errors as [CF_Result](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_result.md).

## Remarks

Coroutines are functions that can be paused with [cf_coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_yield.md) and resumed with [cf_coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_resume.md). See [CF_Coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine.md)
for more details.

## Related Pages

[CF_Coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine.md)  
[CF_CoroutineFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutinefn.md)  
[CF_CoroutineState](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutinestate.md)  
[cf_make_coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_make_coroutine.md)  
[cf_destroy_coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_destroy_coroutine.md)  
[cf_coroutine_state_to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_state_to_string.md)  
[cf_coroutine_currently_running](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_currently_running.md)  
[cf_coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_yield.md)  
[cf_coroutine_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_state.md)  
[cf_coroutine_get_udata](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_get_udata.md)  
[cf_coroutine_push](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_push.md)  
[cf_coroutine_pop](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_pop.md)  
[cf_coroutine_bytes_pushed](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_bytes_pushed.md)  
[cf_coroutine_space_remaining](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_space_remaining.md)  
