# error_set

Use this function to set a Cute error.

## Syntax

```cpp
void error_set(cute_t* cute, const char* error_string);
```

## Function Parameters

Parameter Name | Description
--- | ---
cute | The instance of the Cute framework to set the error of.
error_string | The error string to set.

## Remarks

Will overwrite any previously set error. Will cause the error handler to be invoked, if an error handler is set. See [error_handler_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/error_handler_fn.md) for more details.

## Related Functions

[error_get](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/error_get.md),
[error_handler_set](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/error_handler_set.md)
