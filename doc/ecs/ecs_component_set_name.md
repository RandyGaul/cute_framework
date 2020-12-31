# ecs_component_set_name

Sets the name of a component during registration within Cute's ECS.

## Syntax

```cpp
void ecs_component_set_name(app_t* app, const char* name);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
name | The name of the component being registered.

## Remarks

This function is a part of Cute's ECS API. To learn more about this, see the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/README.md).

## Related Functions

[ecs_component_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_begin.md)  
[ecs_component_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_end.md)  
[ecs_component_set_size](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_size.md)  
[ecs_component_set_optional_serializer](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_optional_serializer.md)  
[ecs_component_set_optional_cleanup](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_optional_cleanup.md)  
