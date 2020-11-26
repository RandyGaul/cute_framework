# app_is_entity_valid

Checks to see whether or not the `entity_t` references an active entity instance.

## Syntax

```cpp
bool app_is_entity_valid(app_t* app, entity_t entity);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
entity | The entity to check.

## Related Functions

[app_make_entity](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_make_entity.md)  
[app_delayed_destroy_entity](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_delayed_destroy_entity.md)  
[app_destroy_entity](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_destroy_entity.md)  
