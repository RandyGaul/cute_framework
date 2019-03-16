# error_handler_set

Use this function to set function handler for errors, which is called when an error occurs in Cute.

## Syntax

```cpp
void error_handler_set(cute_t* cute, error_handler_fn* handler, void* udata);
```

## Function Parameters

Parameter Name | Description
--- | ---
cute | The instance of the Cute framework to set the error handler of.
handler | The function pointer to be called when a Cute error occurs. See [Remarks](https://github.com/RandyGaul/cute_framework/new/master/doc/cute/error_handler_set.md#Remarks) for more details.
udata | The user data to be passed to the handler upon invocation.

## Remarks

[error_handler_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/error_handler_fn.md) is a function called when a Cute error occurs. Your handler should be able to handle multiple calls from a concurrent environment.

## Related Functions

[error_get](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/error_get.md)
