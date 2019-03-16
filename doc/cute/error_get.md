# error_get

Use this function to get the error message for the last error that has occurred.

## Syntax

```cpp
const char* error_get(cute_t* cute);
```

## Function Parameters

Parameter Name | Description
--- | ---
cute | The instance of the Cute framework to get the last error about.

## Return Value

Returns the last error that has occurred. The string contains a description of the error. This function will return `NULL` if there are no errors to get. Since multiple errors may have occurred since the last time this function was called, only the last error will be returned.

## Remarks

You do not own the string returned from error_get, as it is located in static memory.

To handle errors in a concurrent environment the [error_handler_set] should be used.

## Related Functions

[cute_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/cute_make.md)
