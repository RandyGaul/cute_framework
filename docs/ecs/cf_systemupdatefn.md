[](../header.md ':include')

# CF_SystemUpdateFn

Category: [ecs](/api_reference?id=ecs)  
GitHub: [cute_ecs.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_ecs.h)  
---

A system.

```cpp
typedef void (CF_SystemUpdateFn)(CF_ComponentList component_list, int count, void* udata);
```

Parameters | Description
--- | ---
component_list | A tuple of components. See [cf_get_components](/ecs/cf_get_components.md).
count | The number of entities to be updated.
udata | An optional user data pointer. `NULL` by default, or set by [cf_system_set_optional_udata](/ecs/cf_system_set_optional_udata.md).

## Remarks

This function represents a single system in the ECS. See [cf_system_begin](/ecs/cf_system_begin.md).

## Related Pages

[cf_system_begin](/ecs/cf_system_begin.md)  
[CF_ComponentList](/ecs/cf_componentlist.md)  
[cf_run_systems](/ecs/cf_run_systems.md)  
[cf_get_components](/ecs/cf_get_components.md)  
[cf_get_entities](/ecs/cf_get_entities.md)  
