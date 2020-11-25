# app_register_component_type

Registers a new type of component.

## Syntax

```cpp
void app_register_component_type(app_t* app, component_config_t component_config);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
component_config | All the parameters to register the component type, [see here](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_config_t.md) for more details.

## Related Functions

[component_serialize_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_serialize_fn.md)  
[component_cleanup_fn](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_cleanup_fn.md)  
[component_config_t](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/component_config_t.md)  
