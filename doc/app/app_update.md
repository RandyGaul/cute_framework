# app_update

Updates the application by gathering inputs and window events.

## Syntax

```cpp
void app_update(app_t* app);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.

## Remarks

This is not the Entity Component System update, all it does is update the application window and internal utilities. If you're looking for how to update the Entity Component System, please see the [ECS docs](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs).

## Related Functions

[app_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_make.md)  
[app_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_destroy.md)  
[app_is_running](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_is_running.md)  
