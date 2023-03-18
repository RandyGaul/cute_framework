/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
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
	
typedef struct CF_Entity { CF_Handle handle; } CF_Entity;
typedef struct CF_ComponentList { uint64_t id; } CF_ComponentList;
typedef struct CF_World { uint64_t id; } CF_World;

typedef void (CF_SystemUpdateFn)(CF_ComponentList component_list, int count, void* udata);
typedef void (CF_ComponentFn)(CF_Entity entity, void* component, void* udata);

static CF_Entity CF_INVALID_ENTITY = { CF_INVALID_HANDLE };
static CF_World CF_INVALID_WORLD = { 0 };

CF_API void CF_CALL cf_entity_begin();
CF_API void CF_CALL cf_entity_set_name(const char* entity_type);
CF_API void CF_CALL cf_entity_add_component(const char* component_type);
CF_API void CF_CALL cf_entity_end();

CF_API CF_Entity CF_CALL cf_make_entity(const char* entity_type, CF_Result* err /*= NULL*/);
CF_API bool CF_CALL cf_entity_is_valid(CF_Entity entity);
CF_API bool CF_CALL cf_entity_is_type(CF_Entity entity, const char* entity_type);
CF_API const char* CF_CALL cf_entity_get_type_string(CF_Entity entity);
CF_API bool CF_CALL cf_entity_has_component(CF_Entity entity, const char* component_type);
CF_API void* CF_CALL cf_entity_get_component(CF_Entity entity, const char* component_type);
CF_API void CF_CALL cf_destroy_entity(CF_Entity entity);
CF_API void CF_CALL cf_destroy_entity_delayed(CF_Entity entity);

CF_INLINE bool cf_entity_equals(CF_Entity* a, CF_Entity* b) { return a->handle == b->handle; }

CF_API void CF_CALL cf_entity_delayed_deactivate(CF_Entity entity);
CF_API void CF_CALL cf_entity_delayed_activate(CF_Entity entity);
CF_API void CF_CALL cf_entity_deactivate(CF_Entity entity);
CF_API void CF_CALL cf_entity_activate(CF_Entity entity);
CF_API bool CF_CALL cf_entity_is_active(CF_Entity entity);

CF_API void CF_CALL cf_component_begin();
CF_API void CF_CALL cf_component_set_name(const char* name);
CF_API void CF_CALL cf_component_set_size(size_t size);
CF_API void CF_CALL cf_component_set_optional_initializer(CF_ComponentFn* initializer, void* udata);
CF_API void CF_CALL cf_component_set_optional_cleanup(CF_ComponentFn* cleanup, void* udata);
CF_API void CF_CALL cf_component_end();

CF_API void CF_CALL cf_system_begin();
CF_API void CF_CALL cf_system_set_name(const char* name);
CF_API void CF_CALL cf_system_set_update(CF_SystemUpdateFn* update_fn);
CF_API void CF_CALL cf_system_require_component(const char* component_type);
CF_API void CF_CALL cf_system_set_optional_pre_update(void (*pre_update_fn)(void* udata));
CF_API void CF_CALL cf_system_set_optional_post_update(void (*post_update_fn)(void* udata));
CF_API void CF_CALL cf_system_set_optional_udata(void* udata);
CF_API void CF_CALL cf_system_end();

CF_API void CF_CALL cf_run_systems();

CF_API void* CF_CALL cf_get_components(CF_ComponentList component_list, const char* component_type);
CF_API CF_Entity* CF_CALL cf_get_entities(CF_ComponentList component_list);
#define CF_GET_COMPONENTS(component_list, T) (T*)cf_get_components(component_list, #T)

CF_API CF_World CF_CALL cf_make_world();
CF_API void CF_CALL cf_destroy_world(CF_World world);
CF_API void CF_CALL cf_world_push(CF_World world);
CF_API CF_World CF_CALL cf_world_pop();
CF_API CF_World CF_CALL cf_world_peek();
CF_INLINE bool cf_world_equals(CF_World a, CF_World b) { return a.id == b.id; }

CF_API bool CF_CALL cf_is_entity_type_valid(const char* entity_type);
CF_API dyna const char** CF_CALL cf_get_entity_list();
CF_API dyna const char** CF_CALL cf_get_component_list();
CF_API dyna const char** CF_CALL cf_get_system_list();
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

CF_INLINE Entity make_entity(const char* entity_type, Result* err = NULL) { return cf_make_entity(entity_type, err); }
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
