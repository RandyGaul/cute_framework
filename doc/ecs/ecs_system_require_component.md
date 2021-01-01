# ecs_system_require_component

Adds a required component to the system. The system will only run on entities containing matching required components.

## Syntax

```cpp
void ecs_system_require_component(app_t* app, const char* component_type);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
component_type | The component type to require.

## Remarks

This function is a part of Cute's ECS API. To learn more about this, see the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/README.md).

## Related Functions

[ecs_system_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_begin.md)  
[ecs_system_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_end.md)  
[ecs_system_set_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_update.md)  
[ecs_system_set_optional_pre_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_pre_update.md)  
[ecs_system_set_optional_post_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_post_update.md)  
[ecs_system_set_optional_update_udata](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_update_udata.md)  
[ecs_run_systems](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_run_systems.md)  
