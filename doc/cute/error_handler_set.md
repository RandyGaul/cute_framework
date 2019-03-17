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

## Code Example

> Setting up a trivial error handler.

```cpp
// Print errors to the stderr stream.
void my_error_handler(const char* error_string, void* udata)
{
	(void)udata;
	fprintf(stderr, "%s", error_string);
}

void setup_cute_error_handler(cute_t* cute)
{
	void* udata = NULL; // No userdata is needed in this example.
	error_handler_set(cute, my_error_handler, udata);
}
```

## Remarks

[error_handler_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/error_handler_fn.md) is a function called when a Cute error occurs. Your handler should be able to handle multiple calls from a concurrent environment.

## Related Functions

[error_get](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/error_get.md),
[error_set](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/error_set.md)
