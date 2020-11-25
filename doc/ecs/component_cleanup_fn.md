# component_cleanup_fn

Cleans up a component upon destruction. This is for a member of [component_config_t](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_config_t.md), and is optional (it may be `NULL`).

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
udata | A user data pointer for your convenience, it is the `udata` field of `component_config_t`. [See here](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_config_t.md) for more details.

## Related Functions

[component_config_t](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_config_t.md)  
[component_serialize_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_serialize_fn.md)  
[app_register_component_type](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_register_component_type.md)  
