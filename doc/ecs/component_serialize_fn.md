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
kv | A kv instance for access to the registered schema hierarchy. [See here](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_register_entity_type.md) for details about entity schemas.
reading | True if this is a reading operation, false otherwise for writing.
entity | The entity being serialized.
component | A pointer to the specific component being serialized.
udata | A user data pointer for your convenience, it is the `udata` field of `component_config_t`. [See here](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_config_t.md) for more details.

## Return Value

You should return any errors encountered as an `error_t` instance, for example any errors reported from kv serialization functions such as `kv_key` or `kv_val` (if you're using kv).

## Related Functions

[component_config_t](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_config_t.md)  
[component_cleanup_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_cleanup_fn.md)  
[app_register_component_type](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_register_component_type.md)  
