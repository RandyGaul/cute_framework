# Entity Component System

The [Entity Component System](https://en.wikipedia.org/wiki/Entity_component_system) (ECS) is an optional feature in Cute Framework used to organize your game code and gameplay logic. The idea is to clearly separate data from logic and make game object composition modular.

A key benefit of using the ECS is having a clear answer to the question "where does the code that does X belong", where X is some gameplay feature. Write systems to hold your gameplay logic, components to hold your data, and entities to define grouped collections of components. With ECS, typically you think about what entities your game needs, break them down into reusable components, and define systems to handle their behaviors.

* An **Entity** is a game object, defined as a collection of components and registered with a name.
* A **Component** is an aspect or trait of an entity. They are like building-blocks that can be mix-and-matched to create different entities.
* A **System** is a function that performs gameplay logic on entities. They are how behavior is implemented within the ECS.

In laymen's terms, entities are things, components are parts of things, and systems are how things behave.

**Disclaimer**: This is an oversimplification of ECS, but it is one that serves our purpose while minimizing confusion for now. For a full understanding, please consult your local [Google](http://www.google.com/search?q=entity+component+system).

## Entities

An entity is merely a collection of components that represents a "thing" in your game, like an enemy, the player, or other game features. Before an entity instance can be created an entity type must be defined and registered with the ECS (like a blueprint, or archetype). How to make components is explained in the next section.

> Registering a collection of 4 components as a new entity type called `Octorok`.

```cpp
ecs_entity_begin(app);
ecs_entity_set_name(app, "Octorok");
ecs_entity_add_component(app, "Transform");
ecs_entity_add_component(app, "GridObject");
ecs_entity_add_component(app, "Sprite");
ecs_entity_add_component(app, "OctorokComponent"); // Named with "Component" at the end to avoid confusion with
ecs_entity_end(app);                               // the entity type string "Octorok".
```

All components are user defined (see Components section).

Once registered, an entity instance can be made by calling a single function `entity_make`.

```cpp
entity_t e = entity_make(app, "Octorok");
```

From here you can call a few different functions upon the entity.

```cpp
entity_t entity_make(app_t* app, const char* entity_type, error_t* err = NULL);
bool entity_is_valid(app_t* app, entity_t entity);
bool entity_is_type(app_t* app, entity_t entity, const char* entity_type);
const char* entity_get_type_string(app_t* app, entity_t entity);
bool entity_has_component(app_t* app, entity_t entity, const char* name);
void* entity_get_component(app_t* app, entity_t entity, const char* name);
void entity_destroy(app_t* app, entity_t entity);
void entity_delayed_destroy(app_t* app, entity_t entity);
```

Here's an example of how to use `entity_get_component`.

```cpp
OctorokComponent* octorok = (OctorokComponent*)entity_get_component(app, e, "OctorokComponent");
```

## Component

A component is merely some memory to hold a struct or class. Cute's ECS requires you to register component types. This tells the ECS some critical information like the size and name of your component. Here is an example of registering a component.

> An example of a component you might make for your game. Octorok is a type of enemy from Zelda games. In this example there is the `Octorok` entity, and the `OctorokComponent`.

```cpp
enum OCTOROK_STATE
{
	OCTOROK_STATE_IDLE,
	OCTOROK_STATE_PATROL,
	OCTOROK_STATE_FIRE,
};

struct OctorokComponent
{
	int pellet_count = 0;
	Pellet pellets[3];
	OCTOROK_STATE state = OCTOROK_STATE_IDLE;
};
```

> Registering the Octorok component.

```cpp
ecs_component_begin(app);
ecs_component_set_name(app, "OctorokComponent");
ecs_component_set_size(app, sizeof(OctorokComponent));
ecs_component_set_optional_serializer(app, octorok_component_serialize);
ecs_component_end(app);
```

### Creating or Destroying Components

In the above section the `octorok_serialize` was not explained. This is a function pointer to be called whenever a new instance of a component is loaded or saved. The function pointer type looks like this.

```cpp
typedef error_t (*serialize_fn)(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata);
```

For the Octorok example the implemented function might look like this.

```cpp
error_t octorok_component_serialize(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata)
{
	OctorokComponent* octorok = (OctorokComponent*)component;
	if (reading) {
		CUTE_PLACEMENT_NEW(octorok) OctorokComponent;
	}
	return error_success();
}
```

For a detailed description of all the pieces of this function please see [here TODO](broken_link). Briefly: If this function is called for making a new component then `reading` is true, and false when saving. `entity` is the entity id for the new entity being created. The rest can be ignored for now.

### Component Dependency

Other ECS's out there have the notion of component depencenies, where a component can not be added to an entity based on what other components the entity has. CF does not have any kind of dependency API. Instead it is suggested for users to enforce their own dependencies rules. Either the registration functions can be wrapped, or, within component serialize functions the `entity_has_component` function can be used to enforce custom dependencies.

For example, say there is a `BlueComponent` but it requires a `RedComponent`. Make sure that `BlueComponent` is required *after* `RedComponent`.

```cpp
void register_components(app_t* app)
{
	ecs_entity_begin(app);
	ecs_entity_set_name(app, "MyEntity");
	ecs_entity_require_component(app, "RedComponent");
	ecs_entity_require_component(app, "BlueComponent");
	ecs_entity_end(app);
}
```

The order components are required is the order they are constructed and initialized. This means when a `BlueComponent` is created it can lookup on the entity to see if the `RedComponent` is present. If it does not exist an error can be reported about the missing dependency.

```cpp
error_t blue_component_serialize(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata)
{
	BlueComponent* blue = (BlueComponent*)component;
	if (reading) {
		CUTE_PLACEMENT_NEW(blue) BlueComponent;
		if (!entity_has_component(app, entity, "RedComponent")) {
			// Report the error.
			return error_failure("The BlueComponent requires the RedComponent be present.");
		}
	}
	return error_success();
}
```

### Component `self` Reference

Other ECS's usually have components share a lot of common functionality. For example in [Unity](https://unity.com/) components store a reference to the owning entity. However, in CF components are 100% controlled by you, so unless added explicitly there will be no references to any entities at all.

If you'd like your components to reference the owning entities simply store the reference in your component when initialized (with the serializer `ecs_set_optional_serializer`).

```cpp
error_t blue_component_serialize(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata)
{
	MyComponent* my_component = (MyComponent*)component;
	if (reading) {
		CUTE_PLACEMENT_NEW(my_component) MyComponent;
		my_component->self = entity; // Assign the "self" reference here, upon initialization.
	}
	return error_success();
}
```

## System

In the ECS systems are just functions. The purpose is to write your gameplay code systems. Each system must be registered into the ECS. The order the systems are updated is the order in which they are registered.

> Registering a system. This system only intakes a single type of component, the Octorok component. Systems can also intake up to 8 different kinds of components.

```cpp
ecs_system_begin(app);
ecs_system_set_update(app, (void*)update_octorok_system);
ecs_system_require_component(app, "OctorokComponent");
// Require more component types here by calling `ecs_system_require_component` up to 7 more times, for a total of 8.
ecs_system_end(app);
```

> An example of an empty (unimplemented) system for the Octorok component.

```cpp
void update_octorok_system(app_t* app, float dt, void* udata, OctorokComponent* octoroks, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		OctorokComponent* octorok = octoroks + i;
		// Update octoroks here ...
	}
}
```

A system can operate on more than just one kind of component, but the above example only operates on the Octorok component. The above system will be called updated *all entities that contain an `Octorok` component*.

Here's an example of a different system that operates on two different component types. The below system will operate on *all entities that contain a `Transform` and a `Sprite` component*.


```cpp
ecs_system_begin(app);
ecs_system_set_update(app, (void*)update_sprite_system);
ecs_system_require_component(app, "Transform");
ecs_system_require_component(app, "Sprite");
ecs_system_end(app);

void update_sprite_system(app_t* app, float dt, void* udata, Transform* transforms, Sprite* sprites, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;
		Sprite* sprite = Sprites + i;
		// Update the components of any entity that contains a Sprite and a Transform here ...
	}
}
```

If a particular entity contained many kinds of components, but also still contained a `Sprite` and a `Transform` component, then the `update_sprite_system` will still be called on that entity's `Sprite` and `Transform` component. In this way systems find and match any entity with the correct types of components, even if there are other types which will be ignored, and runs upon the matched components.

### void* System Signature

Due to a limitation of the C programming language (and a disinterest in using complicated C++ templates) there is one quirk in Cute's ECS API regarding the system update function. When registering a system the `ecs_system_set_update` function is used. Notice that the `update_fn` parameter is merely a `void*`.

```cpp
void ecs_system_set_update(app_t* app, void* update_fn);
```

`update_fn` is a `void*` to signify that many different kinds of function signatures are acceptable. However, some care is needed to follow a strict pattern. The basic form of a system that has no components looks like this.

```cpp
void system_with_no_components(app_t* app, float dt, void* udata);
```

Here is the form of a system taking one component. Notice that an array of `Transform` components is added, along with the `entity_count`.

```cpp
void system_with_one_component(app_t* app, float dt, void* udata, Transform* transforms, int entity_count);
```

Here is an example of the form for two components.

```cpp
void system_with_two_components(app_t* app, float dt, void* udata, Transform* transforms, Sprite* sprites, int entity_count);
```

`transforms` and `sprites` are both arrays of the same length (of length `entity_count`), where each index represents a different entity. Requiring more components types, as required by calling `ecs_system_require_component`, will increase the number of components expected for your system to intake as parameters. The order of the component types is the same as the order in which `ecs_system_require_component` was originally called. The maximum number of different components a system can require is 8. If you want to go above 8 different types of components it is suggested to redesign your components to abide by this limitation, or modify the source code of Cute yourself.

**Important Note** - If care is not taken to require components in the same order as the associated system's signature _no error checking will occur whatsoever_. Your system will be called and the pointers will be mismatched, resulting in undefined behavior (likely immediate crashes and/or heap corruption). Cute's ECS is designed to run very efficiently in a simple way, so type checking will **not** be performed here.

# API List

[entity_t](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_t.md)  

[ecs_entity_begin](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_entity_begin.md)  
[ecs_entity_end](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_entity_end.md)  
[ecs_entity_set_name](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_entity_set_name.md)  
[ecs_entity_add_component](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_entity_add_component.md)  
[ecs_entity_set_optional_schema](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_entity_set_optional_schema.md)  

[entity_make](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_make.md)  
[entity_is_valid](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_is_valid.md)  
[entity_is_type](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_is_type.md)  
[entity_get_type_string](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_get_type_string.md)  
[entity_has_component](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_has_component.md)  
[entity_get_component](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_get_component.md)  
[entity_destroy](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_destroy.md)  
[entity_delayed_destroy](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/entity_delayed_destroy.md)  

[ecs_load_entities](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_load_entities.md)  
[ecs_save_entities](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_save_entities.md)  

[component_serialize_fn](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/component_serialize_fn.md)  
[component_cleanup_fn](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/component_cleanup_fn.md)  

[ecs_component_begin](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_component_begin.md)  
[ecs_component_end](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_component_end.md)  
[ecs_component_set_name](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_component_set_name.md)  
[ecs_component_set_size](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_component_set_size.md)  
[ecs_component_set_optional_serializer](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_component_set_optional_serializer.md)  
[ecs_component_set_optional_cleanup](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_component_set_optional_cleanup.md)  

[ecs_system_begin](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_system_begin.md)  
[ecs_system_end](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_system_end.md)  
[ecs_system_set_update](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_system_set_update.md)  
[ecs_system_require_component](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_system_require_component.md)  
[ecs_system_set_optional_pre_update](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_system_set_optional_pre_update.md)  
[ecs_system_set_optional_post_update](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_system_set_optional_post_update.md)  
[ecs_system_set_optional_update_udata](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_system_set_optional_update_udata.md)  

[ecs_run_systems](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs/ecs_run_systems.md)  
