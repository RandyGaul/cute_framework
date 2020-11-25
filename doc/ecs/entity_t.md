# entity_t

Represents an instance of an entity.

## Data Fields

type | name | Description
--- | --- | ---
entity_type_t | type | `uint32_t` representing the entity type, as defined by [app_register_entity_type](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_register_entity_type.md). You can fetch the associated type string by calling [app_entity_type_string](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_entity_type_string.md).
handle_t | handle | A handle for internal use.

## Code Example

> Making an instance of an entity (the entity must have been already registered with [app_register_entity_type](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_register_entity_type.md).

```cpp
entity e = INVALID_ENTITY;
error_t err = app_make_entity(app, "YourEntityType", &entity);
if (err.is_error()) {
	// Unable to make entity...
}
```

## Related Functions

[app_make_entity](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/app_make_entity.md)  
[app_delayed_destroy_entity](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/app_delayed_destroy_entity.md)  
[app_is_entity_valid](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/app_is_entity_valid.md)  
[app_get_component](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/app_get_component.md)  
[app_has_component](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/app_has_component.md)  
