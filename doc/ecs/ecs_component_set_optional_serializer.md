# ecs_component_set_optional_serializer

Sets the optional serializer of a component during registration within Cute's ECS.

## Syntax

```cpp
void ecs_component_set_optional_serializer(app_t* app, component_serialize_fn* serializer_fn, void* udata = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
serializer_fn | The optional serializer of the component being registered.
udata | Optional user data pointer to passed to the `serializer_fn` whenever it is called.

## Remarks

This function is a part of Cute's ECS API. To learn more about this, see the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/README.md).

## Related Functions

[ecs_component_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_begin.md)  
[ecs_component_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_end.md)  
[ecs_component_set_name](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_name.md)  
[ecs_component_set_size](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_size.md)  
[ecs_component_set_optional_cleanup](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_optional_cleanup.md)  
