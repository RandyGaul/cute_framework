# app_make_entity

Creates a new entity.

## Syntax

```cpp
error_t app_make_entity(app_t* app, const char* entity_type, entity_t* entity_out = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
entity_type | The type of entity to create.
entity_out | The identifier for the created entity instance, will be `INVALID_ENTITY` upon errors.

## Return Value

Returns any error details upon failure.

## Related Functions

[app_delayed_destroy_entity](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_delayed_destroy_entity.md)  
[app_destroy_entity](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_destroy_entity.md)  
[app_is_entity_valid](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_is_entity_valid.md)  
[app_get_component](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_get_component.md)  
[app_has_component](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_has_component.md)  
