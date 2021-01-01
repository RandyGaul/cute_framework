# entity_delayed_destroy

Queues up the destruction of an entity to occur at the end of the next [ecs_run_systems](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_run_systems.md) function call.

## Syntax

```cpp
void entity_delayed_destroy(app_t* app, entity_t entity);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
entity | The entity to destroy.

## Related Functions

[entity_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_make.md)  
[entity_is_valid](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_is_valid.md)  
[entity_is_type](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_is_type.md)  
[entity_get_type_string](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_get_type_string.md)  
[entity_has_component](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_has_component.md)  
[entity_get_component](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_get_component.md)  
[entity_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_destroy.md)  
[ecs_load_entities](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_load_entities.md)  
[ecs_save_entities](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_save_entities.md)  
