# app_has_component

Retrieves a component from an entity.

## Syntax

```cpp
bool app_has_component(app_t* app, entity_t entity, const char* name);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
entity_t | Identifier for a specific entity instance.
name | The type of the component.

## Return Value

Returns true of the entity has the requested component type, false otherwise.

## Related Functions

[app_get_component](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_get_component.md)  
