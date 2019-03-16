# is_running

Use this function to control your main loop. This function will return true until a window or game event is signaled to shutdown the process (such as clicking the exit button on a window).

## Syntax

```cpp
int is_running(cute_t* cute);
```

## Function Parameters

Parameter Name | Description
--- | ---
cute | The instance of the Cute framework to check the running status of.

## Return Value

Returns `1` until a signal is received that Cute needs to shutdown, then it will return `0`.

## Code Example

> Running an infinite loop, until ready to destroy Cute.

```cpp
while (is_running(cute))
{
	// your code here ...
}
```

## Remarks

The function [stop_running](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/stop_running.md) can be used to force `is_running` to return `0` on the next call.

## Related Functions

[cute_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/cute_make.md),
[cute_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/cute_destroy.md),
[stop_running](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/stop_running.md)
