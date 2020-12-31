# entity_get_component

Retrieves a component from an entity.

## Syntax

```cpp
void* entity_get_component(app_t* app, entity_t entity, const char* name);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
entity_t | Identifier for a specific entity instance.
name | The type of the component.

## Return Value

Returns a pointer to the component. Typecast to the appropriate type yourself.

## Related Functions

[entity_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_make.md)  
[entity_is_valid](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_is_valid.md)  
[entity_is_type](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_is_type.md)  
[entity_get_type_string](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_get_type_string.md)  
[entity_has_component](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_has_component.md)  
[entity_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_destroy.md)  
[entity_delayed_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_delayed_destroy.md)  
[ecs_load_entities](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_load_entities.md)  
[ecs_save_entities](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_save_entities.md)  
