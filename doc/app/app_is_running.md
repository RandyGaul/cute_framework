# app_is_running

Use this function to control your main loop. This function will return true until a window or game event is signaled to shutdown the process (such as clicking the exit button on a window).

## Syntax

```cpp
bool app_is_running(app_t* app);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.

## Return Value

Returns `true` until a signal is received that Cute needs to shutdown, then it will return `false`.

## Code Example

> Running an infinite loop, until ready to destroy Cute.

```cpp
while (app_is_running(app))
{
	// your code here ...
}
```

## Remarks

The function [app_stop_running](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_stop_running.md) can be used to force `app_is_running` to return `false` on the next call.

## Related Functions

[app_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_make.md)  
[app_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_destroy.md)  
[app_stop_running](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_stop_running.md)  
