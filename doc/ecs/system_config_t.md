# system_config_t

All the parameters needed to call [app_register_system](https://github.com/RandyGaul/cute_framework/blob/master/doc/ecs/app_register_system.md) in order to register a new type of system.

## Data Fields

type | name | Description
--- | --- | ---
void* | udata | A user data pointer for your convenience, it is the `udata` parameter passed to one of the system update functions, `pre_update_fn`, `post_update_fn`, or `post_update_fn`.
`void (*)(app_t* app, float dt, void* udata)` | pre_update_fn | Called once before `update_fn` is called.
void* | update_fn | Called to update components. Can be called multiple times within a single game tick.
`void (*)(app_t* app, float dt, void* udata)` | post_update_fn | Called once after `update_fn` is done being called.

## Code Example

> Registering a system. This system only intakes a single type of component, the Octorok component. Systems can also intake up to 8 different kinds of components. This excerpt was taken from the [main ECS readme page](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs).

```cpp
system_config_t system;
system.component_types.add("OctorokComponent");
system.update_fn = (void*)update_octorok_system;
app_register_system(app, system);
```

## Related Functions

[app_register_system](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/app_register_system.md)  
[app_run_ecs_systems](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/app_run_ecs_systems.md)  
