# ecs_system_set_optional_post_update

Sets the optional post-update function for the system during registration with Cute's ECS.

## Syntax

```cpp
void ecs_system_set_optional_post_update(app_t* app, void (*post_update_fn)(app_t* app, float dt, void* udata));
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
post_update_fn | The function to be called during the post-update phase. Called once after the `update_fn` is called, as set by [ecs_system_set_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_update.md). The `udata` is set by [ecs_system_set_optional_update_udata](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_update_udata.md).

## Remarks

This function is a part of Cute's ECS API. To learn more about this, see the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/README.md).

## Related Functions

[ecs_system_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_begin.md)  
[ecs_system_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_end.md)  
[ecs_system_set_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_update.md)  
[ecs_system_require_component](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_require_component.md)  
[ecs_system_set_optional_pre_update](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_pre_update.md)  
[ecs_system_set_optional_update_udata](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_system_set_optional_update_udata.md)  
[ecs_run_systems](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_run_systems.md)  
