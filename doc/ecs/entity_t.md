# entity_t

Represents an instance of an entity.

## Operators

Entities can be used with the equals `==` and not equals `!=` operators. Here is an example.

```cpp
entity_t a = entity_make(app, "EntityTypeA");
entity_t b = entity_make(app, "EntityTypeB");
entity_t c = a;

printf("a %s b\n", a == b ? "equals" : "does not equal");
printf("b %s c\n", b == c ? "equals" : "does not equal");
printf("a %s c\n", a == c ? "equals" : "does not equal");
```

Which prints the following.

```
a does not equal b
b does not equal c
a equals c
```

## Data Fields

All data fields are for internal use only.

## Code Example

> Making an instance of an entity (the entity must have been already registered with the ECS. You can learn how to register entities in the [ECS readme](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/README.md)).

```cpp
error_t err;
entity e = entity_make(app, "YourEntityType", &err);
if (err.is_error()) {
	// Unable to make entity...
}
```

## Related Functions

[entity_make](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_make.md)  
[entity_destroy](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_destroy.md)  
[entity_delayed_destroy](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_delayed_destroy.md)  
[entity_is_valid](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_is_valid.md)  
[entity_is_type](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_is_type.md)  
[entity_get_type_string](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_get_type_string.md)  
[entity_get_component](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_get_component.md)  
[entity_has_component](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_has_component.md)  
