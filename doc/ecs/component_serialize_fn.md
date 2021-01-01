# component_serialize_fn

Serializes a component. This can be a read or write operation.

## Syntax

```cpp
typedef error_t (component_serialize_fn)(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
kv | A kv instance for access to the registered schema hierarchy. [See the Remarks section here](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_entity_set_optional_schema.md) for details about entity schemas.
reading | True if this is a reading operation, false otherwise for writing.
entity | The entity being serialized.
component | A pointer to the specific component being serialized.
udata | A user data pointer for your convenience, as set by [ecs_component_set_optional_serializer](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_optional_serializer.md).

## Return Value

You should return any errors encountered as an `error_t` instance, for example any errors reported from kv serialization functions such as `kv_key` or `kv_val` (if you're using kv).

## Related Functions

[component_cleanup_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_cleanup_fn.md)  
[ecs_component_set_optional_serializer](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_optional_serializer.md)  
[ecs_component_set_optional_cleanup](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_component_set_optional_cleanup.md)  
[ecs_load_entities](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_load_entities.md)  
[ecs_save_entities](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_save_entities.md)  
