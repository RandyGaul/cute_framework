# app_register_entity_type

Registers a new type of entity.

## Syntax

```cpp
entity_type_t app_register_entity_type(app_t* app, const char* schema);
entity_type_t app_register_entity_type(app_t* app, array<const char*> component_types, const char* entity_type);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
schema | The schema string, see remarks for details.
component_types | The type of each component that comprise this kind of entity.
entity_type | The name for this type of entity.

## Return Value

Returns the entity type encoded as a 32-bit integer `entity_type_t`.

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

## Related Functions

[app_entity_type_string](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_entity_type_string.md)  
[app_entity_is_type](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_entity_is_type.md)  
