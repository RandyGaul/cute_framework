# Entity Component System

The Entity Component Systems is an optional feature of Cute Framework used to organize your game code and gameplay logic. The idea is to write systems to hold your gameplay logic, components to hold your data, and entities to define collections of components.

The main purpose of using the ECS is to answer the question of "where does the code that does X belong", where X is some gameplay feature.

## Component

A component is merely some memory to hold a struct or class. Cute's ECS requires you to register component types. This tells the ECS some critical information like the size and name of your component. Here is an example of registering a component.

> An example of a struct you might make, used for gameplay. Octorok is a type of enemy from Zelda games.

```cpp
enum OCTOROK_STATE
{
	OCTOROK_STATE_IDLE,
	OCTOROK_STATE_PATROL,
	OCTOROK_STATE_FIRE,
};

struct Octorok
{
	int pellet_count = 0;
	Pellet pellets[3];
	OCTOROK_STATE state;
};
```

> Registering the Octorok.

```cpp
component_config_t config;
config.name = "Octorok";
config.size_of_component = sizeof(Octorok);
config.serializer_fn = octorok_serialize;
app_register_component_type(app, config);
```

## Creating or Destroying Components

In the above section the `octorok_serialize` was not explained. This is a function pointer to be called whenever a new instance of a component is loaded or saved. The function pointer type looks like this.

```cpp
typedef error_t (*serialize_fn)(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata);
```

For the Octorok example the implemented function might look like this.

```cpp
error_t octorok_serialize(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata)
{
	Octorok* octorok = (Octorok*)component;
	if (reading) {
		CUTE_PLACEMENT_NEW(octorok) Octorok;
	}
	return error_success();
}
```

For a detailed description of all the pieces of this function please see [here TODO](broken_link). Briefly: If this function is called for making a new component then `reading` is true, and false when saving. `entity` is the entity id for the new entity being created. The rest can be ignored for now.

## Entities

An entity is merely a collection of components that represents a "thing" in your game, like an enemy, the player, or other game features. Before an entity can be made an entity type must be registered with the ECS.

> Registering a new type of entity with the ECS. This registers an entity type named `Octorok` along with a list of components that each Octorok entity possesses.

```cpp
array<const char*> component_types = {
	"Transform",
	"GridObject",
	"Sprite",
	"Octorok"
};
app_register_entity_type(app, component_types, "Octorok");
```

Each Octorok entity has a Transform component, along with GridObject, Sprite, and an Octorok component. All components are defined by you. In this example only Octorok is shown to keep this readme small, but in your game you would have had to implement all components yourself, and pick their names during registration.

Once registered an entity can be made by calling a single function `app_make_entity`.

```cpp
entity_t e;
app_make_entity(app, "Octorok", &e);
```

From here you can call a few different functions upon the entity.

```cpp
void app_delayed_destroy_entity(app_t* app, entity_t entity);
void app_destroy_entity(app_t* app, entity_t entity);
bool app_is_entity_valid(app_t* app, entity_t entity);
void* app_get_component(app_t* app, entity_t entity, const char* name);
bool app_has_component(app_t* app, entity_t entity, const char* name);
```

Here's an example of how to use `app_get_component`.

```cpp
Octorok* octorok = (Octorok*)app_get_component(app, e, "Octorok");
```

## System

Systems are just functions. The purpose is to write your gameplay code systems. Each system must be registered into the ECS. The order the systems are updated is the order in which they are registered.

> Registering a system. This system only intakes a single type of component, the Octorok component. Systems can also intake up to 8 different kinds of components.

```cpp
system_config_t system;
system.component_types.add("Octorok");
system.update_fn = (void*)update_octorok_system;
app_register_system(app, system);
```

> An example of an empty (unimplemented) system for the Octorok component.

```cpp
void update_octorok_system(app_t* app, float dt, void* udata, Octorok* octoroks, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		Octorok* octorok = octoroks + i;
		// Update octoroks here ...
	}
}
```

A system can operate on more than just one kind of component, but the above example only operates on the Octorok component. This system will be called updated *all entities that contain an Octorok component*.

Here's an example of a different system that operates on two different component types.


```cpp
system_config_t system;
system.component_types = {
	"Transform",
	"Sprite"
};
system.update_fn = (void*)update_sprite_system;
app_register_system(app, system);

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
