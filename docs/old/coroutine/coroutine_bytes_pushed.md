# coroutine_bytes_pushed

Returns the number of bytes pushed onto the coroutine's data stack.

## Syntax

```cpp
size_t coroutine_bytes_pushed(coroutine_t* co);
```

## Function Parameters

Parameter Name | Description
--- | ---
co | The coroutine.

## Return Value

Returns the number of bytes pushed onto the coroutine's data stack.

## Related Functions

[coroutine_pop](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_pop.md)  
[coroutine_push](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_push.md)  
[coroutine_space_remaining](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_space_remaining.md)  
