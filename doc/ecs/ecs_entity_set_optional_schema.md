# ecs_entity_set_optional_schema

Adds an optional schema string during entity registration.

## Syntax

```cpp
void ecs_entity_set_optional_schema(app_t* app, const char* schema);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
schema | The kv serialized schema.

## Remarks

The `schema` string is the advanced style of entity type registration. If you'd prefer a simpler alternative, the second `app_register_entity_type` function overload is preferred. The schema string is a [kv serialized string](https://github.com/RandyGaul/cute_framework/tree/master/doc/serialization) with some special restricted keys. Here is an example.

```cpp
const char* schema_ice_block = CUTE_STRINGIZE(
	entity_type = "IceBlock",
	Transform = { },
	Animator = { name = "ice_block.aseprite" },
	BoardPiece = { },
	IceBlock = { },
	Shadow = { },
);
```

There are two special restricted keys for schema strings.

1. `entity_type` - REQUIRED - A string containing the type of the entity.
2. `inherits_from` - OPTIONAL - A string containing the type of the entity this entity inherits from, using [kv-serialization data inheritence](https://github.com/RandyGaul/cute_framework/tree/master/doc/serialization).

The rest of the top-level keys are names of components this entity uses. In the above example each component name (Transform, Animator, BoardPiece, IceBlock, and Shadow) are all components defined by you in your game. Besides restricted keys and names of components, no other **top-level** keys will be seen or used by the ECS system.

Keys within components will be used to serialize the component. For example the `name` key of the `Animator` component will be set to "ice_block.aseprite" when serializing.

For entities registered with this function, calling `ecs_entity_add_component` is not necessary as the components have been specified within the schema itself.

## Related Functions

[ecs_entity_begin](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_entity_begin.md)  
[ecs_entity_end](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_entity_end.md)  
[ecs_entity_set_name](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_entity_set_name.md)  
[ecs_entity_add_component](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/ecs_entity_add_component.md)  
