# ecs_entity_add_component

Adds a component type during entity registration.

## Syntax

```cpp
void ecs_entity_add_component(app_t* app, const char* component_type);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
component_type | The name of the component type as specified by [ecs_component_set_name](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_name.md).

## Remarks

This function is a part of Cute's ECS API. To learn more about this, see the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/README.md).

## Related Functions

[ecs_entity_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_entity_begin.md)  
[ecs_entity_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_entity_end.md)  
[ecs_entity_set_name](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_entity_set_name.md)  
[ecs_entity_set_optional_schema](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_entity_set_optional_schema.md)  
