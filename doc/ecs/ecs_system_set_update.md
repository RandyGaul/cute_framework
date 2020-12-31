# ecs_system_set_update

Sets the update function of a system during registration with Cute's ECS.

## Syntax

```cpp
void ecs_system_set_update(app_t* app, void* update_fn);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
update_fn | The function pointer of your update function. The signature should be typecasted to `void*`, but must follow a specific pattern detailed here in the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/README.md).

## Remarks

This function is a part of Cute's ECS API. To learn more about this, see the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/README.md).

## Related Functions

[ecs_system_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_begin.md)  
[ecs_system_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_end.md)  
[ecs_system_require_component](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_require_component.md)  
[ecs_system_set_optional_pre_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_pre_update.md)  
[ecs_system_set_optional_post_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_post_update.md)  
[ecs_system_set_optional_update_udata](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_update_udata.md)  
[ecs_run_systems](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_run_systems.md)  
