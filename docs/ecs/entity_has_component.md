# entity_has_component

Checks if an entity has a certain type of component.

## Syntax

```cpp
bool entity_has_component(entity_t entity, const char* name);
```

## Function Parameters

Parameter Name | Description
--- | ---
entity_t | Identifier for a specific entity instance.
name | The type of the component.

## Return Value

Returns true of the entity has the requested component type, false otherwise.

## Related Functions

[entity_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/entity_make.md)  
[entity_is_valid](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/entity_is_valid.md)  
[entity_is_type](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/entity_is_type.md)  
[entity_get_type_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/entity_get_type_string.md)  
[entity_get_component](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/entity_get_component.md)  
[entity_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/entity_destroy.md)  
[entity_delayed_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/entity_delayed_destroy.md)  
[ecs_load_entities](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/ecs_load_entities.md)  
[ecs_save_entities](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/ecs_save_entities.md)  
