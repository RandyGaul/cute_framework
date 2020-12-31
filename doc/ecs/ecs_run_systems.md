# ecs_run_systems

Runs all systems in the ECS in the order they were originally registered.

## Syntax

```cpp
void ecs_run_systems(app_t* app);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
udata | The udata passed to update callbacks for the system (update, pre-update and post-update).

## Remarks

This function is a part of Cute's ECS API. To learn more about this, see the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/README.md).

Order of system registration is the order updates are called. The flow is like so.

```
for each system
    call system pre update
    for each entity type with matching required component set
        call system update
    call system post update
```

## Related Functions

[ecs_system_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_begin.md)  
[ecs_system_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_end.md)  
[ecs_system_set_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_update.md)  
[ecs_system_require_component](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_require_component.md)  
[ecs_system_set_optional_pre_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_pre_update.md)  
[ecs_system_set_optional_post_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_post_update.md)  
[ecs_system_set_optional_update_udata](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_update_udata.md)  
