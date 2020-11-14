# app_stop_running

Signals to Cute to stop running, and will cause [app_is_running](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/app_is_running.md) to return `false`.

## Syntax

```cpp
void app_stop_running(app_t* app);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.

## Related Functions

[app_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_make.md)  
[app_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_destroy.md)  
[app_is_running](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_is_running.md)  
