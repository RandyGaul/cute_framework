# ecs_save_entities

Saves entities

## Syntax

```cpp
error_t ecs_save_entities(const array<entity_t>& entities, kv_t* kv);
error_t ecs_save_entities(const array<entity_t>& entities);
```

## Function Parameters

Parameter Name | Description
--- | ---
entities | The entities to save.
kv | A read-only kv instance for saving to.

## Remarks

This function is a part of Cute's ECS API. To learn more about this, see the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/README.md).

The second overload (without the `kv` instance) can be used if kv is not needed -- all serialization should occur on your own by the serialization callback set when calling [ecs_component_set_optional_serializer](https://github.com/RandyGaul/cute_framework/new/master/doc/ecs/ecs_component_set_optional_serializer.md).

## Related Functions

[entity_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/entity_make.md)  
[entity_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/entity_destroy.md)  
[entity_delayed_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/entity_delayed_destroy.md)  
[ecs_load_entities](https://github.com/RandyGaul/cute_framework/blob/master/docs/ecs/ecs_load_entities.md)  
