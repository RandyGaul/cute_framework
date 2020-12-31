# component_cleanup_fn

Cleans up a component upon destruction.

## Syntax

```cpp
typedef void (component_cleanup_fn)(app_t* app, entity_t entity, void* component, void* udata);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
entity | The entity being serialized.
component | A pointer to the specific component being serialized.
udata | A user data pointer for your convenience, as set by [ecs_component_set_optional_cleanup](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_optional_cleanup.md).

## Related Functions

[component_serialize_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_serialize_fn.md)  
[ecs_component_set_optional_serializer](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_optional_serializer.md)  
[ecs_component_set_optional_cleanup](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_optional_cleanup.md)  
[ecs_load_entities](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_load_entities.md)  
[ecs_save_entities](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_save_entities.md)  
