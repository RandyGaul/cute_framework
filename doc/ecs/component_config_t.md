# component_config_t

All the parameters needed to call [app_register_component_type](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_register_component_type.md) in order to register a new type of component.

## Data Fields

type | name | Description
--- | --- | ---
const char* | name | Name for this type of component. Can not be `NULL`.
size_t | size_of_component | Size in bytes of this component's struct or class instantiation (must not be 0).
udata | A user data pointer for your convenience, it is the `udata` parameter passed to either [component_serialize_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_serialize_fn.md) or [component_clneaup_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_clneaup_fn.md).

## Code Example

> Registering the Octorok component. A snippet from the main [ECS readme page](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs).

```cpp
component_config_t config;
config.name = "OctorokComponent";
config.size_of_component = sizeof(OctorokComponent);
config.serializer_fn = octorok_component_serialize;
app_register_component_type(app, config);
```

## Related Functions

[component_serialize_fn](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/component_serialize_fn.md)  
[component_cleanup_fn](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/component_cleanup_fn.md)  
[app_register_component_type](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/app_register_component_type.md)  
