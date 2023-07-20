[](../header.md ':include')

# cf_destroy_coroutine

Category: [coroutine](/api_reference?id=coroutine)  
GitHub: [cute_coroutine.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_coroutine.h)  
---

Destroys a coroutine created by [cf_make_coroutine](/coroutine/cf_make_coroutine.md).

```cpp
void cf_destroy_coroutine(CF_Coroutine co);
```

Parameters | Description
--- | ---
co | The coroutine.

## Remarks

All objects on the coroutine's stack will get automically cleaned up.

## Related Pages

[CF_Coroutine](/coroutine/cf_coroutine.md)  
[CF_CoroutineFn](/coroutine/cf_coroutinefn.md)  
[CF_CoroutineState](/coroutine/cf_coroutinestate.md)  
[cf_make_coroutine](/coroutine/cf_make_coroutine.md)  
[cf_coroutine_currently_running](/coroutine/cf_coroutine_currently_running.md)  
[cf_coroutine_state_to_string](/coroutine/cf_coroutine_state_to_string.md)  
[cf_coroutine_resume](/coroutine/cf_coroutine_resume.md)  
[cf_coroutine_yield](/coroutine/cf_coroutine_yield.md)  
[cf_coroutine_state](/coroutine/cf_coroutine_state.md)  
[cf_coroutine_get_udata](/coroutine/cf_coroutine_get_udata.md)  
[cf_coroutine_push](/coroutine/cf_coroutine_push.md)  
[cf_coroutine_pop](/coroutine/cf_coroutine_pop.md)  
[cf_coroutine_bytes_pushed](/coroutine/cf_coroutine_bytes_pushed.md)  
[cf_coroutine_space_remaining](/coroutine/cf_coroutine_space_remaining.md)  
