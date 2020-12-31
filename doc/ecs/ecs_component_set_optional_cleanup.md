# ecs_component_set_optional_cleanup

Sets the optional cleanup of a component during registration within Cute's ECS.

## Syntax

```cpp
void ecs_component_set_optional_cleanup(app_t* app, component_cleanup_fn* cleanup_fn, void* udata = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
cleanup_fn | The optional cleanup of the component being registered.
udata | Optional user data pointer to passed to the `cleanup_fn` whenever it is called.

## Remarks

This function is a part of Cute's ECS API. To learn more about this, see the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/README.md).

## Related Functions

[ecs_component_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_begin.md)  
[ecs_component_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_end.md)  
[ecs_component_set_name](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_name.md)  
[ecs_component_set_size](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_size.md)  
[ecs_component_set_optional_serializer](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_optional_serializer.md)  
