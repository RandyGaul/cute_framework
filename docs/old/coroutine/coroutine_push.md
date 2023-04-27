# coroutine_pop

Pushes some data off the data stack of the coroutine.

## Syntax

```cpp
error_t coroutine_pop(coroutine_t* co, void* data, size_t size);
```

## Function Parameters

Parameter Name | Description
--- | ---
co | The coroutine.
data | Data popped off of the data stack is written here.
size | The size in bytes of data to pop.

## Return Value

Returns any errors upon failure.

## Remarks

For convenience each coroutine has 1kb of memory used for [coroutine_push](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_push.md) and `coroutine_pop`. These two functions are for transfering data such as local variables or bits of state back and forth over the call site boundary between [coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_resume.md) and [coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_yield.md). These functions are totally optional and here merely for preferential purposes.

## Related Functions

[coroutine_push](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_push.md)  
[coroutine_bytes_pushed](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_bytes_pushed.md)  
[coroutine_space_remaining](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_space_remaining.md)  
