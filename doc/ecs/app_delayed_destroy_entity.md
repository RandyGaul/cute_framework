# app_delayed_destroy_entity

Queues up the destruction of an entity to occur at the end of the next [app_run_ecs_systems](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_run_ecs_systems.md) function call.

## Syntax

```cpp
void app_delayed_destroy_entity(app_t* app, entity_t entity);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
entity | The entity to destroy.

## Related Functions

[app_make_entity](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_make_entity.md)  
[app_destroy_entity](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_destroy_entity.md)  
