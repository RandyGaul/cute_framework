# app_entity_is_type

Returns true if an entity matches the specified type, false otherwise.

## Syntax

```cpp
bool app_entity_is_type(app_t* app, entity_t entity, const char* entity_type);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
entity_t | Identifier for a specific entity instance.
entity_type | The type of entity to check against.

## Return Value

Returns true if an entity matches the specified type, false otherwise.

## Related Functions

[app_register_entity_type](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_register_entity_type.md)  
[app_entity_type_string](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_entity_type_string.md)  
