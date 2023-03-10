[](../header.md ':include')

# cf_coroutine_resume

Category: [coroutine](/api_reference?id=coroutine)  
GitHub: [cute_coroutine.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_coroutine.h)  
---

Resumes the coroutine.

```cpp
CF_API CF_Result CF_CALL cf_coroutine_resume(CF_Coroutine* co);
```

Parameters | Description
--- | ---
co | The coroutine.
return    Returns info on any errors as [CF_Result](/utility/cf_result.md).

## Remarks

Coroutines are functions that can be paused with [cf_coroutine_yield](/coroutine/cf_coroutine_yield.md) and resumed with [cf_coroutine_resume](/coroutine/cf_coroutine_resume.md). See [CF_Coroutine](/coroutine/cf_coroutine.md)
for more details.

## Related Pages

[CF_Coroutine](/coroutine/cf_coroutine.md)  
[CF_CoroutineFn](/coroutine/cf_coroutinefn.md)  
[CF_CoroutineState](/coroutine/cf_coroutinestate.md)  
[cf_make_coroutine](/coroutine/cf_make_coroutine.md)  
[cf_destroy_coroutine](/coroutine/cf_destroy_coroutine.md)  
[cf_coroutine_state_to_string](/coroutine/cf_coroutine_state_to_string.md)  
[cf_coroutine_currently_running](/coroutine/cf_coroutine_currently_running.md)  
[cf_coroutine_yield](/coroutine/cf_coroutine_yield.md)  
[cf_coroutine_state](/coroutine/cf_coroutine_state.md)  
[cf_coroutine_get_udata](/coroutine/cf_coroutine_get_udata.md)  
[cf_coroutine_push](/coroutine/cf_coroutine_push.md)  
[cf_coroutine_pop](/coroutine/cf_coroutine_pop.md)  
[cf_coroutine_bytes_pushed](/coroutine/cf_coroutine_bytes_pushed.md)  
[cf_coroutine_space_remaining](/coroutine/cf_coroutine_space_remaining.md)  
