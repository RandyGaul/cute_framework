[](../header.md ':include')

# cf_coroutine_push

Category: [coroutine](/api_reference?id=coroutine)  
GitHub: [cute_coroutine.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_coroutine.h)  
---

Pushes some bytes onto the coroutine's storage.

```cpp
CF_Result cf_coroutine_push(CF_Coroutine co, const void* data, size_t size);
```

Parameters | Description
--- | ---
co | The coroutine.
data | The data to push into the storage.
size | The size of `data` in bytes.
return    Returns info on any errors as [CF_Result](/utility/cf_result.md).

## Remarks

Each coroutine has an internal storage of 1k (1024) bytes. The purpose is to allow a formal communication/parameter passing
in/out of coroutines via FIFO ordering. These storage are totally optional, and here merely for convenience.

## Related Pages

[CF_Coroutine](/coroutine/cf_coroutine.md)  
[CF_CoroutineFn](/coroutine/cf_coroutinefn.md)  
[CF_CoroutineState](/coroutine/cf_coroutinestate.md)  
[cf_make_coroutine](/coroutine/cf_make_coroutine.md)  
[cf_destroy_coroutine](/coroutine/cf_destroy_coroutine.md)  
[cf_coroutine_state_to_string](/coroutine/cf_coroutine_state_to_string.md)  
[cf_coroutine_resume](/coroutine/cf_coroutine_resume.md)  
[cf_coroutine_yield](/coroutine/cf_coroutine_yield.md)  
[cf_coroutine_state](/coroutine/cf_coroutine_state.md)  
[cf_coroutine_get_udata](/coroutine/cf_coroutine_get_udata.md)  
[cf_coroutine_currently_running](/coroutine/cf_coroutine_currently_running.md)  
[cf_coroutine_pop](/coroutine/cf_coroutine_pop.md)  
[cf_coroutine_bytes_pushed](/coroutine/cf_coroutine_bytes_pushed.md)  
[cf_coroutine_space_remaining](/coroutine/cf_coroutine_space_remaining.md)  
