[](../header.md ':include')

# cf_entity_set_name

Category: [ecs](/api_reference?id=ecs)  
GitHub: [cute_ecs.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_ecs.h)  
---

Sets the name of a new entity type.

```cpp
void cf_entity_set_name(const char* entity_type);
```

Parameters | Description
--- | ---
entity_type | The name of the new entity type.

## Remarks

You must first call [cf_entity_begin](/ecs/cf_entity_begin.md) to begin a new entity definition. `entity_type` is what gets
passed into [cf_make_entity](/ecs/cf_make_entity.md).

## Related Pages

[CF_Entity](/ecs/cf_entity.md)  
[cf_entity_begin](/ecs/cf_entity_begin.md)  
[cf_make_entity](/ecs/cf_make_entity.md)  
[cf_entity_add_component](/ecs/cf_entity_add_component.md)  
[cf_entity_end](/ecs/cf_entity_end.md)  
