# app_entity_type_string

Returns the string associated with an entity integer type `entity_type_t`.

## Syntax

```cpp
const char* app_entity_type_string(app_t* app, entity_t entity);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
entity | The entity to fetch the type of.

## Return Value

Returns the string associated with an entity's type.

## Related Functions

[app_register_entity_type](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_register_entity_type.md)  
[app_entity_is_type](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_entity_is_type.md)  
