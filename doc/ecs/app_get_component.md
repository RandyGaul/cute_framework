# app_get_component

Retrieves a component from an entity.

## Syntax

```cpp
void* app_get_component(app_t* app, entity_t entity, const char* name);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
entity_t | Identifier for a specific entity instance.
name | The type of the component.

## Return Value

Returns a pointer to the component. Typecast to the appropriate type yourself.

## Related Functions

[app_has_component](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_has_component.md)  
