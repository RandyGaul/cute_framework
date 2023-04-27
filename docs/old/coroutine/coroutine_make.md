# coroutine_make

Creates a new coroutine.

## Syntax

```cpp
coroutine_t* coroutine_make(coroutine_fn* fn, int stack_size = 0, void* udata = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
fn | The function for the coroutine to run. See Remarksf or details.
stack_size | The size allocated on the heap for the coroutine's call stack.
udata | Optional pointer for user data, can be set to `NULL`.

## Return Value

Returns a new coroutine instance.

## Remarks

The `coroutine_fn` is a function pointer with the following signature.

```c
typedef void (coroutine_fn)(coroutine_t* co)
```

## Related Functions

[coroutine_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_destroy.md)  
[coroutine_yield](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_yield.md)  
[coroutine_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/coroutine/coroutine_resume.md)  
