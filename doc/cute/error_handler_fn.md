# error_handler_fn

This function type can be set as the Cute error handler (by [error_handler_set](https://github.com/RandyGaul/cute_framework/new/master/doc/cute/error_handler_set.md)), used to get reports of errors in a concurrent environment.

## Syntax

```cpp
typedef void (error_handler_fn)(const char* error_string, void* udata);
```

## Function Parameters

Parameter Name | Description
--- | ---
string | The utf8 encoded string containing the clipboard text.
udata | The user data originally set by [error_handler_set](https://github.com/RandyGaul/cute_framework/new/master/doc/cute/error_handler_set.md).

## Remarks

This function can be called asynchronously, so be sure to implement your handler in a thread-safe way. Our suggestion is to somehow queue up errors in your handler, to be consumed at a later point by another thread.
