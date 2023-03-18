[](../header.md ':include')

<br>

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
cf_entity_begin();
cf_entity_set_name("Octorok");
cf_entity_add_component("Transform");
cf_entity_add_component("GridObject");
cf_entity_add_component("Sprite");
cf_entity_add_component("OctorokComponent");
cf_entity_end();
```

All components are user defined (see [Components](https://randygaul.github.io/cute_framework/#/topics/entity_component_system?id=component)).

Once registered, an entity instance can be made by calling a single function `cf_make_entity`.

```cpp
CF_Entity e = cf_make_entity("Octorok");
```

From here you can call a few different functions upon the entity.

- [`cf_make_entity`](https://randygaul.github.io/cute_framework/#/ecs/cf_make_entity)
- [`cf_destroy_entity`](https://randygaul.github.io/cute_framework/#/ecs/cf_destroy_entity)
- [`cf_destroy_entity_delayed`](https://randygaul.github.io/cute_framework/#/ecs/cf_destroy_entity_delayed)
- [`cf_entity_is_valid`](https://randygaul.github.io/cute_framework/#/ecs/cf_entity_is_valid)
- [`cf_entity_is_type`](https://randygaul.github.io/cute_framework/#/ecs/cf_entity_is_type)
- [`cf_entity_get_type_string`](https://randygaul.github.io/cute_framework/#/ecs/cf_entity_get_type_string)
- [`cf_entity_has_component`](https://randygaul.github.io/cute_framework/#/ecs/cf_entity_has_component)
- [`cf_entity_get_component`](https://randygaul.github.io/cute_framework/#/ecs/cf_entity_get_component)

Here's an example of how to use [`cf_entity_get_component`](https://randygaul.github.io/cute_framework/#/ecs/cf_entity_get_component).

```cpp
OctorokComponent* octorok = (OctorokComponent*)entity_get_component(e, "OctorokComponent");
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
ecs_component_begin();
ecs_component_set_name("OctorokComponent");
ecs_component_set_size(sizeof(OctorokComponent));
ecs_component_set_optional_initializer(octorok_component_initialize);
ecs_component_end();
```

### Creating or Destroying Components

In the above section the `octorok_component_initialize` was not explained. This is a function pointer to be called whenever a new instance of a component is created. The function pointer type is of type [`CF_ComponentFn`](https://randygaul.github.io/cute_framework/#/ecs/cf_componentfn).

For the Octorok example the implemented function might look like this.

```cpp
void octorok_component_initialize(CF_Entity entity, void* component, void* udata)
{
	OctorokComponent* octorok = (OctorokComponent*)component;
	CF_PLACEMENT_NEW(octorok) OctorokComponent;
}
```

This initialize function will be called any time an `Octorok` entity is created, thus creating an `OctorokComponent`. A similar function can be called upon destruction to cleanup your component, [`cf_component_set_optional_cleanup`](https://randygaul.github.io/cute_framework/#/ecs/cf_component_set_optional_cleanup).

?> You may be wondering about `CF_PLACEMENT_NEW`. This is a way to call a constructor in C++. In C there are no constructors, so a good alternative is to zero-out the entire component as an initial state.

### Component Dependency

Other ECS's out there have the notion of component dependencies, where a component can not be added to an entity based on what other components the entity has. CF does not have any kind of dependency API. Instead it is suggested for users to enforce their own dependencies rules. Either the registration functions can be wrapped, or, within component serialize functions the [`cf_entity_has_component`](https://randygaul.github.io/cute_framework/#/ecs/cf_entity_has_component) function can be used to enforce custom dependencies.

For example, say there is a `BlueComponent` but it requires a `RedComponent`. Make sure that `BlueComponent` is defined *after* `RedComponent`.

```cpp
void register_components()
{
	ecs_entity_begin();
	ecs_entity_set_name("MyEntity");
	ecs_entity_add_component("RedComponent");
	ecs_entity_add_component("BlueComponent");
	ecs_entity_end();
}
```

The order components are added is the order they are constructed and initialized. This means when a `BlueComponent` is created it can lookup on the entity to see if the `RedComponent` is present. If it does not exist an error can be reported about the missing dependency.

```cpp
void blue_component_intialize(CF_Entity entity, void* component, void* udata)
{
	BlueComponent* blue = (BlueComponent*)component;
	CF_PLACEMENT_NEW(blue) BlueComponent;
	if (!cf_entity_has_component(entity, "RedComponent")) {
		// Report the error however you prefer.
		my_report_function("The BlueComponent requires the RedComponent be present.");
	}
}
```

### Component `self` Reference

Other ECS's usually have components share a lot of common functionality. For example in [Unity](https://unity.com/) components store a reference to the owning entity. However, in CF components are 100% controlled by you, so unless added explicitly there will be no references to any entities at all.

If you'd like your components to reference the owning entities simply store the reference in your component when initialized (with [`cf_component_set_optional_initializer`](https://randygaul.github.io/cute_framework/#/ecs/cf_component_set_optional_initializer)).

```cpp
void blue_component_intialize(CF_Entity entity, void* component, void* udata)
{
	MyComponent* my_component = (MyComponent*)component;
	CF_PLACEMENT_NEW(my_component) MyComponent;
	my_component->self = entity; // Assign the "self" reference here, upon initialization.
}
```

### World 'self' Reference

Just like the above section, if you would like your entities or components to know what world they belong you may store a world reference yourself. This may not at all be necessary, but some people prefer to add it in like so:

```cpp
void blue_component_intialize(CF_Entity entity, void* component, void* udata)
{
	MyComponent* my_component = (MyComponent*)component;
	CF_PLACEMENT_NEW(my_component) MyComponent;
	my_component->self = entity; // Assign the "self" reference here, upon initialization.
	my_component->world = cf_world_peek(); // Remember which world this entity belongs to.
}
```

## System

In the ECS systems are just functions. The purpose is to write your gameplay code systems. Each system must be registered into the ECS. The order the systems are updated is the order in which they are registered.

> Registering a system. This system only intakes a single type of component, the Octorok component.

```cpp
cf_system_begin();
cf_system_set_update((void*)update_octorok_system);
cf_system_require_component("OctorokComponent");
// Require more component types here by calling `cf_system_require_component`.
cf_system_end();
```

> An example of an empty (not yet fully implemented) system for the Octorok component.

```cpp
void update_octorok_system(CF_ComponentList component_list, int entity_count, void* udata)
{
	OctorokComponent* octoroks = (OctorokComponent*)cf_get_components(component_list, "OctorokComponent");
	for (int i = 0; i < entity_count; ++i) {
		OctorokComponent* octorok = octoroks + i;
		// Update octoroks here ...
	}
}
```

Alternatively, you may use `CF_GET_COMPONENTS` for a short-hand notation to fetch components out of the component list. This only works if you follow the naming convention that your component type in C/C++ exactly matches the string name defined by [`cf_component_set_name`](https://randygaul.github.io/cute_framework/#/ecs/cf_component_set_name).

```cpp
void update_octorok_system(CF_ComponentList component_list, int entity_count, void* udata)
{
	OctorokComponent* octoroks = CF_GET_COMPONENTS(component_list, OctorokComponent);
	for (int i = 0; i < entity_count; ++i) {
		OctorokComponent* octorok = octoroks + i;
		// Update octoroks here ...
	}
}
```

A system can operate on more than just one kind of component, but the above example only operates on the Octorok component. The above system will be called updated *all entities that contain an `Octorok` component*.

Here's an example of a different system that operates on two different component types. The below system will operate on *all entities that contain a `Transform` and a `Sprite` component*.


```cpp
cf_system_begin();
cf_system_set_update(update_sprite_system);
cf_system_require_component("Transform");
cf_system_require_component("Sprite");
cf_system_end();

void update_sprite_system(CF_ComponentList component_list, int entity_count, void* udata)
{
	Transform* transforms = (Transform*)cf_get_components(component_list, "Transform");
	Sprite* sprites = (Sprite*)cf_get_components(component_list, "Sprite");
	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;
		Sprite* sprite = Sprites + i;
		// Update the components of any entity that contains a Sprite and a Transform here ...
	}
}
```

If a particular entity contained many kinds of components, but also still contained a `Sprite` and a `Transform` component, then the `update_sprite_system` will still be called on that entity's `Sprite` and `Transform` component. In this way systems find and match any entity with the correct types of components, even if there are other types which will be ignored, and runs upon the matched components.

## Full Example

TODO - Make a sample.
