[](../header.md ':include')

# CF_World

Category: [ecs](/api_reference?id=ecs)  
GitHub: [cute_ecs.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_ecs.h)  
---

An opaque handle represent an ECS world.

## Remarks

All entities reside within a world. There's a default world, but you may also create your own
own worlds with [cf_make_world](/ecs/cf_make_world.md). To select a current world, use [cf_world_push](/ecs/cf_world_push.md). Whichever world
is currently selected will be referenced when calling any ECS-related function.

## Related Pages

[cf_make_entity](/ecs/cf_make_entity.md)  
[cf_make_world](/ecs/cf_make_world.md)  
[cf_destroy_world](/ecs/cf_destroy_world.md)  
[cf_world_push](/ecs/cf_world_push.md)  
[cf_world_pop](/ecs/cf_world_pop.md)  
[cf_world_peek](/ecs/cf_world_peek.md)  
