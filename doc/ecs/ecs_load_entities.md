# ecs_load_entities

Loads entities from a parsed kv instance.

## Syntax

```cpp
error_t ecs_load_entities(app_t* app, kv_t* kv, array<entity_t>* entities_out = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
kv | The parsed kv instance containing the serialized entity data.
entities_out | Optional parameter, will be filled with the entities.

## Remarks

This function is a part of Cute's ECS API. To learn more about this, see the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/README.md).

## Related Functions

[entity_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_make.md)  
[entity_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_destroy.md)  
[entity_delayed_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/entity_delayed_destroy.md)  
[ecs_save_entities](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_save_entities.md)  
