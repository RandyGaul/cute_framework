# cf_coroutine_space_remaining | [coroutine](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine_readme.md) | [cute_coroutine.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_coroutine.h)

Returns the number of bytes currently available for use in the coroutine's storage.

```cpp
size_t cf_coroutine_space_remaining(CF_Coroutine* co);
```

Parameters | Description
--- | ---
co | The coroutine.

## Remarks

Each coroutine has an internal storage of 1k (1024) bytes. The purpose is to allow a formal communication/parameter passing
in/out of coroutines via FIFO ordering. These storage are totally optional, and here merely for convenience.

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
[cf_coroutine_currently_running](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/cf_coroutine_currently_running.md)  
