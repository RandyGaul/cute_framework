# coroutine_space_remaining

Returns the number of bytes available on the coroutine's data stack.

## Syntax

```cpp
size_t coroutine_space_remaining(coroutine_t* co);
```

## Function Parameters

Parameter Name | Description
--- | ---
co | The coroutine.

## Return Value

Returns the number of bytes available on the coroutine's data stack.

## Related Functions

[coroutine_pop](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_pop.md)  
[coroutine_push](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_push.md)  
[coroutine_bytes_pushed](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_bytes_pushed.md)  
