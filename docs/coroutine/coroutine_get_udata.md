# coroutine_get_udata

Returns the user data pointer `udata` set by calling [coroutine_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_make.md).

## Syntax

```cpp
void* coroutine_get_udata(coroutine_t* co);
```

## Function Parameters

Parameter Name | Description
--- | ---
co | The coroutine.

## Return Value

Returns the user data pointer.

## Related Functions

[coroutine_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_make.md)  
