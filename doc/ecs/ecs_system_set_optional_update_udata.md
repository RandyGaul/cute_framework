# ecs_system_set_optional_update_udata

Sets the optional udata to be passed to all update functions for the system (update, pre-update and post-update).

## Syntax

```cpp
void ecs_system_set_optional_update_udata(app_t* app, void* udata);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
udata | The udata passed to update callbacks for the system (update, pre-update and post-update).

## Remarks

This function is a part of Cute's ECS API. To learn more about this, see the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/README.md).

## Related Functions

[ecs_system_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_begin.md)  
[ecs_system_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_end.md)  
[ecs_system_set_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_update.md)  
[ecs_system_require_component](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_require_component.md)  
[ecs_system_set_optional_pre_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_pre_update.md)  
[ecs_system_set_optional_post_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_post_update.md)  
[ecs_run_systems](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_run_systems.md)  
