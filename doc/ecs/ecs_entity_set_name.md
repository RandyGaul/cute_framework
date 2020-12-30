# ecs_entity_set_name

Specifies the unique identifier for a new entity type to be registered within Cute's ECS.

## Syntax

```cpp
void ecs_entity_set_name(app_t* app, const char* entity_type);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
entity_type | The name of the new entity type to register. This can be any unique string, but is recommended to be the [stringized](https://en.wikipedia.org/wiki/C_preprocessor#Token_stringification) version of the associated component struct or class name.

## Remarks

This function is a part of Cute's ECS API. To learn more about this, see the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/README.md).

## Related Functions

[ecs_entity_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_entity_begin.md)  
[ecs_entity_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_entity_end.md)  
[ecs_entity_add_component](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_entity_add_component.md)  
[ecs_entity_set_optional_schema](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_entity_set_optional_schema.md)  
