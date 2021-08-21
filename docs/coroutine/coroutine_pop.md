# coroutine_push

Pushes some data onto the data stack of the coroutine.

## Syntax

```cpp
error_t coroutine_push(coroutine_t* co, const void* data, size_t size);
```

## Function Parameters

Parameter Name | Description
--- | ---
co | The coroutine.
data | Data to push onto the data stack.
size | The size in bytes of `data`.

## Return Value

Returns any errors upon failure.

## Remarks

For convenience each coroutine has 1kb of memory used for `coroutine_push` and [coroutine_pop](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_pop.md). These two functions are for transfering data such as local variables or bits of state back and forth over the call site boundary between [coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_resume.md) and [coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_yield.md). These functions are totally optional and here merely for preferential purposes.

## Related Functions

[coroutine_pop](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_pop.md)  
[coroutine_bytes_pushed](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_bytes_pushed.md)  
[coroutine_space_remaining](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_space_remaining.md)  
