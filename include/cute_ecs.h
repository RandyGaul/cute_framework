/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_ECS_H
#define CF_ECS_H

#include "cute_result.h"
#include "cute_handle_table.h"
#include "cute_array.h"
#include "cute_typeless_array.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Entity
 * @category ecs
 * @brief    An opaque handle representing an entity.
 * @remarks  Entities are comprised of a tuple of components. To define an entity check out
 *           `cf_component_begin` and `cf_entity_begin`.
 * @related  CF_Entity cf_make_entity cf_entity_begin cf_component_begin cf_entity_get_component
 */
typedef struct CF_Entity { CF_Handle handle; } CF_Entity;
// @end

/**
 * @struct   CF_ComponentList
 * @category ecs
 * @brief    Contains all the components required for implementing an ECS system.
 * @remarks  The components can be fetched from the list with `cf_get_components`. You may also fetch
 *           all of the `CF_Entity` handles with `cf_get_entities`.
 * @related  CF_Entity CF_ComponentList cf_get_components cf_get_entities CF_SystemUpdateFn cf_system_begin cf_run_systems
 */
typedef struct CF_ComponentList { uint64_t id; } CF_ComponentList;
// @end

/**
 * @struct   CF_World
 * @category ecs
 * @brief    An opaque handle represent an ECS world.
 * @remarks  All entities reside within a world. There's a default world, but you may also create your own
 *           own worlds with `cf_make_world`. To select a current world, use `cf_world_push`. Whichever world
 *           is currently selected will be referenced when calling any ECS-related function.
 * @related  CF_World cf_make_world cf_destroy_world cf_world_push cf_world_pop cf_world_peek cf_make_entity
 */
typedef struct CF_World { uint64_t id; } CF_World;
// @end

/**
 * @function CF_SystemUpdateFn
 * @category ecs
 * @brief    A system.
 * @param    component_list  A tuple of components. See `cf_get_components`.
 * @param    entity_count    The number of entities to be updated.
 * @param    udata           An optional user data pointer. `NULL` by default, or set by `cf_system_set_optional_udata`.
 * @remarks  This function represents a single system in the ECS. See `cf_system_begin`.
 * @related  CF_SystemUpdateFn CF_ComponentList cf_run_systems cf_get_components cf_get_entities cf_system_begin
 */
typedef void (CF_SystemUpdateFn)(CF_ComponentList component_list, int entity_count, void* udata);

/**
 * @function CF_ComponentFn
 * @category ecs
 * @brief    A function for initializing or cleaning up a component.
 * @param    CF_Entity       The entity.
 * @param    component       A pointr to the component. You must typecast to the appropriate type.
 * @param    udata           An optional user data pointer. `NULL` by default, or set by one of: `cf_component_set_optional_initializer`; `cf_component_set_optional_cleanup`.
 * @remarks  This function gets called when an entity is created or destroyed.
 * @related  CF_ComponentFn cf_component_set_optional_initializer cf_component_set_optional_cleanup cf_make_entity cf_destroy_entity
 */
typedef void (CF_ComponentFn)(CF_Entity entity, void* component, void* udata);

/**
 * @struct   CF_INVALID_ENTITY
 * @category ecs
 * @brief    The value of an invalid entity.
 * @remarks  You may set a `CF_Entity` to this value to represent an uninitialized/invalid entity.
 * @related  CF_Entity cf_make_entity cf_entity_equals
 */
static CF_Entity CF_INVALID_ENTITY = { CF_INVALID_HANDLE };
// @end

/**
 * @struct   CF_INVALID_WORLD
 * @category ecs
 * @brief    The value of an invalid world.
 * @remarks  You may set a `CF_World` to this value to represent an uninitialized/invalid world.
 * @related  CF_World cf_make_world cf_world_equals
 */
static CF_World CF_INVALID_WORLD = { 0 };
// @end

/**
 * @function cf_entity_begin
 * @category ecs
 * @brief    Begins the definition of a new entity type.
 * @remarks  TODO
 * @related  CF_Entity cf_entity_begin cf_entity_set_name cf_entity_add_component cf_entity_end cf_make_entity
 */
CF_API void CF_CALL cf_entity_begin();

/**
 * @function cf_entity_set_name
 * @category ecs
 * @brief    Sets the name of a new entity type.
 * @param    entity_type     The name of the new entity type.
 * @remarks  You must first call `cf_entity_begin` to begin a new entity definition. `entity_type` is what gets
 *           passed into `cf_make_entity`.
 * @related  CF_Entity cf_entity_begin cf_entity_set_name cf_entity_add_component cf_entity_end cf_make_entity
 */
CF_API void CF_CALL cf_entity_set_name(const char* entity_type);

/**
 * @function cf_entity_add_component
 * @category ecs
 * @brief    Adds a component to the new entity type.
 * @param    component_type  The type of component to add.
 * @remarks  You must first call `cf_entity_begin` to begin a new entity definition. `component_type` is a component
 *           defined by `cf_component_begin` and friends.
 * @related  CF_Entity cf_entity_begin cf_entity_set_name cf_entity_add_component cf_entity_end cf_make_entity cf_component_begin
 */
CF_API void CF_CALL cf_entity_add_component(const char* component_type);

/**
 * @function cf_entity_end
 * @category ecs
 * @brief    Ends the definition of a new entity type.
 * @remarks  You must first call `cf_entity_begin` to begin a new entity definition.
 * @related  CF_Entity cf_entity_begin cf_entity_set_name cf_entity_add_component cf_entity_end cf_make_entity
 */
CF_API void CF_CALL cf_entity_end();

/**
 * @function cf_make_entity
 * @category ecs
 * @brief    Returns a newly constructed entity instance.
 * @remarks  TODO
 * @related  TODO
 */
CF_API CF_Entity CF_CALL cf_make_entity(const char* entity_type);

/**
 * @function cf_entity_is_valid
 * @category ecs
 * @brief    Returns true if the `CF_Entity` is a valid entity.
 * @remarks  TODO
 * @related  TODO
 */
CF_API bool CF_CALL cf_entity_is_valid(CF_Entity entity);

/**
 * @function cf_entity_is_type
 * @category ecs
 * @brief    Returns true if the entity is a certain type.
 * @remarks  TODO
 * @related  TODO
 */
CF_API bool CF_CALL cf_entity_is_type(CF_Entity entity, const char* entity_type);

/**
 * @function cf_entity_get_type_string
 * @category ecs
 * @brief    Returns an entity's type string.
 * @remarks  TODO
 * @related  TODO
 */
CF_API const char* CF_CALL cf_entity_get_type_string(CF_Entity entity);

/**
 * @function cf_entity_has_component
 * @category ecs
 * @brief    Returns true if an entity has a particular component.
 * @remarks  TODO
 * @related  TODO
 */
CF_API bool CF_CALL cf_entity_has_component(CF_Entity entity, const char* component_type);

/**
 * @function cf_entity_get_component
 * @category ecs
 * @brief    Returns a pointer to a specific component on an entity.
 * @remarks  TODO
 * @related  TODO
 */
CF_API void* CF_CALL cf_entity_get_component(CF_Entity entity, const char* component_type);

/**
 * @function cf_destroy_entity_delayed
 * @category ecs
 * @brief    Marks a specific entity for delayed destruction.
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_destroy_entity_delayed(CF_Entity entity);

/**
 * @function cf_destroy_entity
 * @category ecs
 * @brief    Destroys a specific entity right now.
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_destroy_entity(CF_Entity entity);

/**
 * @function cf_entity_equals
 * @category ecs
 * @brief    Returns true if two `CF_Entity`'s are equal.
 * @remarks  TODO
 * @related  TODO
 */
CF_INLINE bool cf_entity_equals(CF_Entity* a, CF_Entity* b) { return a->handle == b->handle; }

/**
 * @function cf_entity_delayed_deactivate
 * @category ecs
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_entity_delayed_deactivate(CF_Entity entity);

/**
 * @function cf_entity_delayed_activate
 * @category ecs
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_entity_delayed_activate(CF_Entity entity);

/**
 * @function cf_entity_deactivate
 * @category ecs
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_entity_deactivate(CF_Entity entity);

/**
 * @function cf_entity_activate
 * @category ecs
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_entity_activate(CF_Entity entity);

/**
 * @function cf_entity_is_active
 * @category ecs
 * @brief    TODO
 * @related  TODO
 */
CF_API bool CF_CALL cf_entity_is_active(CF_Entity entity);

/**
 * @function cf_entity_delayed_change_type
 * @category ecs
 * @brief    TODO
 * @remarks  CURRENTLY NOT IMPLEMENTED.
 * @related  TODO
 */
CF_API void CF_CALL cf_entity_delayed_change_type(CF_Entity entity, const char* entity_type);

/**
 * @function cf_entity_change_type
 * @category ecs
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_entity_change_type(CF_Entity entity, const char* entity_type);

/**
 * @function cf_entity_type_rename
 * @category ecs
 * @brief    TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_entity_type_rename(const char* entity_type, const char* new_entity_type_name);

/**
 * @function cf_component_begin
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_component_begin();

/**
 * @function cf_component_set_name
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_component_set_name(const char* name);

/**
 * @function cf_component_set_size
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_component_set_size(size_t size);

/**
 * @function cf_component_set_optional_initializer
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_component_set_optional_initializer(CF_ComponentFn* initializer, void* udata);

/**
 * @function cf_component_set_optional_cleanup
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_component_set_optional_cleanup(CF_ComponentFn* cleanup, void* udata);

/**
 * @function cf_component_end
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_component_end();

/**
 * @function cf_component_rename
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_component_rename(const char* component_name, const char* new_component_name);

/**
 * @function cf_system_begin
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_system_begin();

/**
 * @function cf_system_set_name
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_system_set_name(const char* name);

/**
 * @function cf_system_set_update
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_system_set_update(CF_SystemUpdateFn* update_fn);

/**
 * @function cf_system_require_component
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_system_require_component(const char* component_type);

/**
 * @function cf_system_set_optional_pre_update
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_system_set_optional_pre_update(void (*pre_update_fn)(void* udata));

/**
 * @function cf_system_set_optional_post_update
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_system_set_optional_post_update(void (*post_update_fn)(void* udata));

/**
 * @function cf_system_set_optional_udata
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_system_set_optional_udata(void* udata);

/**
 * @function cf_system_end
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_system_end();

/**
 * @function cf_run_systems
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_run_systems();

/**
 * @function cf_get_components
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void* CF_CALL cf_get_components(CF_ComponentList component_list, const char* component_type);

/**
 * @function cf_get_entities
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API CF_Entity* CF_CALL cf_get_entities(CF_ComponentList component_list);
#define CF_GET_COMPONENTS(component_list, T) (T*)cf_get_components(component_list, #T)

/**
 * @function cf_make_world
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API CF_World CF_CALL cf_make_world();

/**
 * @function cf_destroy_world
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_destroy_world(CF_World world);

/**
 * @function cf_world_push
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_world_push(CF_World world);

/**
 * @function cf_world_pop
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API CF_World CF_CALL cf_world_pop();

/**
 * @function cf_world_peek
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API CF_World CF_CALL cf_world_peek();

/**
 * @function cf_world_equals
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_INLINE bool cf_world_equals(CF_World a, CF_World b) { return a.id == b.id; }

/**
 * @function cf_is_entity_type_valid
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API bool CF_CALL cf_is_entity_type_valid(const char* entity_type);

/**
 * @function cf_get_entity_list
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API dyna const char** CF_CALL cf_get_entity_list();

/**
 * @function cf_get_component_list
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API dyna const char** CF_CALL cf_get_component_list();

/**
 * @function cf_get_system_list
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API dyna const char** CF_CALL cf_get_system_list();

/**
 * @function cf_get_component_list_for_entity_type
 * @category ecs
 * @brief    TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API dyna const char** CF_CALL cf_get_component_list_for_entity_type(const char* entity_type);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Entity = CF_Entity;
using ComponentList = CF_ComponentList;
using World = CF_World;
using SystemUpdateFn = CF_SystemUpdateFn;
using ComponentFn = CF_ComponentFn;

//--------------------------------------------------------------------------------------------------
// Entity

static constexpr Entity INVALID_ENTITY = { CF_INVALID_HANDLE };
static constexpr World INVALID_WORLD = { 0 };

CF_INLINE void entity_begin() { cf_entity_begin(); }
CF_INLINE void entity_set_name(const char* entity_type) { cf_entity_set_name(entity_type); }
CF_INLINE void entity_add_component(const char* component_type) { cf_entity_add_component(component_type); }
CF_INLINE void entity_end() { cf_entity_end(); }

CF_INLINE Entity make_entity(const char* entity_type) { return cf_make_entity(entity_type); }
CF_INLINE bool entity_is_valid(Entity entity) { return cf_entity_is_valid(entity); }
CF_INLINE bool entity_is_type(Entity entity, const char* entity_type) { return cf_entity_is_type(entity, entity_type); }
CF_INLINE const char* entity_get_type_string(Entity entity) { return cf_entity_get_type_string(entity); }
CF_INLINE bool entity_has_component(Entity entity, const char* component_type) { return cf_entity_has_component(entity, component_type); }
CF_INLINE void* entity_get_component(Entity entity, const char* component_type) { return cf_entity_get_component(entity, component_type); }
CF_INLINE void destroy_entity(Entity entity) { cf_destroy_entity(entity); }
CF_INLINE void destroy_entity_delayed(Entity entity) { cf_destroy_entity_delayed(entity); }

CF_INLINE bool entity_equals(CF_Entity* a, CF_Entity* b) { return a->handle == b->handle; }
CF_INLINE bool operator==(const CF_Entity& a, const CF_Entity& b) { return a.handle == b.handle; }
CF_INLINE bool operator!=(const CF_Entity& a, const CF_Entity& b) { return a.handle != b.handle; }

CF_INLINE void entity_delayed_deactivate(CF_Entity entity) { cf_entity_delayed_deactivate(entity); }
CF_INLINE void entity_delayed_activate(CF_Entity entity) { cf_entity_delayed_activate(entity); }
CF_INLINE void entity_deactivate(CF_Entity entity) { cf_entity_deactivate(entity); }
CF_INLINE void entity_activate(CF_Entity entity) { cf_entity_activate(entity); }
CF_INLINE bool entity_is_active(CF_Entity entity) { return cf_entity_is_active(entity); }

CF_INLINE void component_begin() { cf_component_begin(); }
CF_INLINE void component_set_name(const char* name) { cf_component_set_name(name); }
CF_INLINE void component_set_size(size_t size) { cf_component_set_size(size); }
CF_INLINE void component_set_optional_initializer(ComponentFn* intializer, void* udata = NULL) { cf_component_set_optional_initializer(intializer, udata); }
CF_INLINE void component_set_optional_cleanup(ComponentFn* cleanup, void* udata = NULL) { cf_component_set_optional_cleanup(cleanup, udata); }
CF_INLINE void component_end() { cf_component_end(); }

CF_INLINE void system_begin() { cf_system_begin(); }
CF_INLINE void system_set_name(const char* name) { cf_system_set_name(name); }
CF_INLINE void system_set_update(CF_SystemUpdateFn* update_fn) { cf_system_set_update(update_fn); }
CF_INLINE void system_require_component(const char* component_type) { cf_system_require_component(component_type); }
CF_INLINE void system_set_optional_pre_update(void (*pre_update_fn)(void* udata)) { cf_system_set_optional_pre_update(pre_update_fn); }
CF_INLINE void system_set_optional_post_update(void (*post_update_fn)(void* udata)) { cf_system_set_optional_post_update(post_update_fn); }
CF_INLINE void system_set_optional_udata(void* udata) { cf_system_set_optional_udata(udata); }
CF_INLINE void system_end() { cf_system_end(); }

CF_INLINE void run_systems() { cf_run_systems(); }

CF_INLINE void* CF_CALL get_components(ComponentList component_list, const char* component_type) { return cf_get_components(component_list, component_type); }
CF_INLINE Entity* CF_CALL get_entities(ComponentList component_list) { return cf_get_entities(component_list); }

CF_INLINE CF_World make_world() { return cf_make_world(); }
CF_INLINE void destroy_world(CF_World world) { cf_destroy_world(world); }
CF_INLINE void world_push(CF_World world) { cf_world_push(world); }
CF_INLINE CF_World world_pop() { return cf_world_pop(); }
CF_INLINE CF_World world_peek() { return cf_world_peek(); }
CF_INLINE bool world_equals(CF_World a, CF_World b) { return a.id == b.id; }
CF_INLINE bool operator==(CF_World a, CF_World b) { return a.id == b.id; }
CF_INLINE bool operator!=(CF_World a, CF_World b) { return a.id != b.id; }

CF_INLINE bool is_entity_type_valid(const char* entity_type) { return cf_is_entity_type_valid(entity_type); }

CF_INLINE Array<const char*> get_entity_list()
{
	dyna const char** list = cf_get_entity_list();
	Array<const char*> result;
	for (int i = 0; i < acount(list); ++i) {
		result.add(list[i]);
	}
	afree(list);
	return result;
}

CF_INLINE Array<const char*> get_component_list()
{
	dyna const char** list = cf_get_component_list();
	Array<const char*> result;
	for (int i = 0; i < acount(list); ++i) {
		result.add(list[i]);
	}
	afree(list);
	return result;
}

CF_INLINE Array<const char*> get_system_list()
{
	dyna const char** list = cf_get_system_list();
	Array<const char*> result;
	for (int i = 0; i < acount(list); ++i) {
		result.add(list[i]);
	}
	afree(list);
	return result;
}

CF_INLINE Array<const char*> get_component_list_for_entity_type(const char* entity_type)
{
	dyna const char** list = cf_get_component_list_for_entity_type(entity_type);
	Array<const char*> result;
	for (int i = 0; i < acount(list); ++i) {
		result.add(list[i]);
	}
	afree(list);
	return result;
}

}

#endif // CF_CPP

#endif // CF_ECS_H
