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

typedef struct CF_KeyValue CF_KeyValue;

//--------------------------------------------------------------------------------------------------
// Entity

typedef struct CF_Entity
{
	#ifdef CF_CPP
	CF_INLINE bool operator==(const CF_Entity& other) { return handle == other.handle; }
	CF_INLINE bool operator!=(const CF_Entity& other) { return handle != other.handle; }
	#endif // CF_CPP

	CF_Handle handle; // For internal use -- don't touch.
} CF_Entity;


static CF_Entity CF_INVALID_ENTITY = { CF_INVALID_HANDLE };

CF_API void CF_CALL cf_ecs_entity_begin();
CF_API void CF_CALL cf_ecs_entity_end();
CF_API void CF_CALL cf_ecs_entity_set_name(const char* Entityype);
CF_API void CF_CALL cf_ecs_entity_add_component(const char* component_type);
CF_API void CF_CALL cf_ecs_entity_set_optional_schema(const char* schema);

CF_API CF_Entity CF_CALL cf_make_entity(const char* Entityype, CF_Result* err /*= NULL*/);
CF_API bool CF_CALL cf_entity_is_valid(CF_Entity entity);
CF_API bool CF_CALL cf_entity_is_type(CF_Entity entity, const char* Entityype);
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

/**
 * `kv` needs to be in `KV_STATE_READ` mode.
 */
CF_API CF_Result CF_CALL cf_ecs_load_entities(CF_KeyValue* kv, CF_Entity** entities_out /*= NULL*/, int* entities_count_out /*= NULL*/);
CF_API void CF_CALL cf_ecs_free_entities(CF_Entity* entities);

/**
 * `kv` needs to be in `KV_STATE_WRITE` mode.
 */
CF_API CF_Result CF_CALL cf_ecs_save_entities_kv(const CF_Entity* entities, int entities_count, CF_KeyValue* kv);
CF_API CF_Result CF_CALL cf_ecs_save_entities(const CF_Entity* entities, int entities_count);

//--------------------------------------------------------------------------------------------------
// Component

typedef bool (CF_ComponentSerializeFn)(CF_KeyValue* kv, bool reading, CF_Entity entity, void* component, void* udata);
typedef void (CF_ComponentCleanupFn)(CF_Entity entity, void* component, void* udata);

CF_API void CF_CALL cf_ecs_component_begin();
CF_API void CF_CALL cf_ecs_component_end();
CF_API void CF_CALL cf_ecs_component_set_name(const char* name);
CF_API void CF_CALL cf_ecs_component_set_size(size_t size);
CF_API void CF_CALL cf_ecs_component_set_optional_serializer(CF_ComponentSerializeFn* serializer_fn, void* udata /*= NULL*/);
CF_API void CF_CALL cf_ecs_component_set_optional_cleanup(CF_ComponentCleanupFn* cleanup_fn, void* udata /*= NULL*/);

//--------------------------------------------------------------------------------------------------
// System

typedef struct CF_EcsArrays CF_EcsArrays;

typedef void (CF_SystemUpdateFn)(float dt, CF_EcsArrays* arrays, int count, void* udata);
CF_API void* CF_CALL cf_ecs_arrays_find_components(CF_EcsArrays* arrays, const char* component_type);
CF_API CF_Entity* CF_CALL cf_ecs_arrays_get_entities(CF_EcsArrays* arrays);

CF_API void CF_CALL cf_ecs_system_begin();
CF_API void CF_CALL cf_ecs_system_end();
CF_API void CF_CALL cf_ecs_system_set_name(const char* name);
CF_API void CF_CALL cf_ecs_system_set_update(CF_SystemUpdateFn* update_fn);
CF_API void CF_CALL cf_ecs_system_require_component(const char* component_type);
CF_API void CF_CALL cf_ecs_system_set_optional_pre_update(void (*pre_update_fn)(float dt, void* udata));
CF_API void CF_CALL cf_ecs_system_set_optional_post_update(void (*post_update_fn)(float dt, void* udata));
CF_API void CF_CALL cf_ecs_system_set_optional_update_udata(void* udata);

CF_API void CF_CALL cf_ecs_run_systems(float dt);

//--------------------------------------------------------------------------------------------------
// Introspection

CF_API bool CF_CALL cf_ecs_is_Entityype_valid(const char* Entityype);
CF_API const char** CF_CALL cf_ecs_get_entity_list(int* entities_count_out /*optional*/);
CF_API const char** CF_CALL cf_ecs_get_component_list(int* components_count_out /*optional*/);
CF_API const char** CF_CALL cf_ecs_get_system_list(int* systems_count_out /*optional*/);
CF_API const char** CF_CALL cf_ecs_get_component_list_for_Entityype(const char* Entityype, int* components_count_out /*optional*/);
CF_API void CF_CALL cf_ecs_free_list(const char** list);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using KeyValue = CF_KeyValue;
using Entity = CF_Entity;
using EcsArrays = CF_EcsArrays;
using SystemUpdateFn = CF_SystemUpdateFn;
using ComponentSerializeFn = CF_ComponentSerializeFn;
using ComponentCleanupFn = CF_ComponentCleanupFn;

//--------------------------------------------------------------------------------------------------
// Entity

static constexpr Entity INVALID_ENTITY = { CF_INVALID_HANDLE };

CF_INLINE void ecs_entity_begin() { cf_ecs_entity_begin(); }
CF_INLINE void ecs_entity_end() { cf_ecs_entity_end(); }
CF_INLINE void ecs_entity_set_name(const char* Entityype) { cf_ecs_entity_set_name(Entityype); }
CF_INLINE void ecs_entity_add_component(const char* component_type) { cf_ecs_entity_add_component(component_type); }
CF_INLINE void ecs_entity_set_optional_schema(const char* schema) { cf_ecs_entity_set_optional_schema(schema); }

CF_INLINE Entity make_entity(const char* Entityype, Result* err = NULL) { return cf_make_entity(Entityype, err); }
CF_INLINE bool entity_is_valid(Entity entity) { return cf_entity_is_valid(entity); }
CF_INLINE bool entity_is_type(Entity entity, const char* Entityype) { return cf_entity_is_type(entity, Entityype); }
CF_INLINE const char* entity_get_type_string(Entity entity) { return cf_entity_get_type_string(entity); }
CF_INLINE bool entity_has_component(Entity entity, const char* component_type) { return cf_entity_has_component(entity, component_type); }
CF_INLINE void* entity_get_component(Entity entity, const char* component_type) { return cf_entity_get_component(entity, component_type); }
CF_INLINE void destroy_entity(Entity entity) { cf_destroy_entity(entity); }
CF_INLINE void destroy_entity_delayed(Entity entity) { cf_destroy_entity_delayed(entity); }

CF_API Result CF_CALL ecs_load_entities(KeyValue* kv, Array<Entity>* entities_out = NULL);

CF_API Result CF_CALL ecs_save_entities(const Array<Entity>& entities, KeyValue* kv);
CF_API Result CF_CALL ecs_save_entities(const Array<Entity>& entities);

//--------------------------------------------------------------------------------------------------
// Component

CF_INLINE void ecs_component_begin() { cf_ecs_component_begin(); }
CF_INLINE void ecs_component_end() { cf_ecs_component_end(); }
CF_INLINE void ecs_component_set_name(const char* name) { cf_ecs_component_set_name(name); }
CF_INLINE void ecs_component_set_size(size_t size) { cf_ecs_component_set_size(size); }
CF_INLINE void ecs_component_set_optional_serializer(ComponentSerializeFn* serializer_fn, void* udata = NULL) { cf_ecs_component_set_optional_serializer(serializer_fn, udata); }
CF_INLINE void ecs_component_set_optional_cleanup(ComponentCleanupFn* cleanup_fn, void* udata = NULL) { cf_ecs_component_set_optional_cleanup(cleanup_fn, udata); }

//--------------------------------------------------------------------------------------------------
// System

CF_INLINE void* CF_CALL ecs_arrays_find_components(EcsArrays* arrays, const char* component_type) { return cf_ecs_arrays_find_components(arrays, component_type); }
CF_INLINE Entity* CF_CALL ecs_arrays_get_entities(EcsArrays* arrays) { return cf_ecs_arrays_get_entities(arrays); }

CF_INLINE void ecs_system_begin() { cf_ecs_system_begin(); }
CF_INLINE void ecs_system_end() { cf_ecs_system_end(); }
CF_INLINE void ecs_system_set_name(const char* name) { cf_ecs_system_set_name(name); }
CF_INLINE void ecs_system_set_update(CF_SystemUpdateFn* update_fn) { cf_ecs_system_set_update(update_fn); }
CF_INLINE void ecs_system_require_component(const char* component_type) { cf_ecs_system_require_component(component_type); }
CF_INLINE void ecs_system_set_optional_pre_update(void (*pre_update_fn)(float dt, void* udata)) { cf_ecs_system_set_optional_pre_update(pre_update_fn); }
CF_INLINE void ecs_system_set_optional_post_update(void (*post_update_fn)(float dt, void* udata)) { cf_ecs_system_set_optional_post_update(post_update_fn); }
CF_INLINE void ecs_system_set_optional_update_udata(void* udata) { cf_ecs_system_set_optional_update_udata(udata); }

CF_INLINE void ecs_run_systems(float dt) { cf_ecs_run_systems(dt); }

//--------------------------------------------------------------------------------------------------
// Introspection

CF_INLINE bool ecs_is_Entityype_valid(const char* Entityype) { return cf_ecs_is_Entityype_valid(Entityype); }
CF_API Array<const char*> CF_CALL ecs_get_entity_list();
CF_API Array<const char*> CF_CALL ecs_get_component_list();
CF_API Array<const char*> CF_CALL ecs_get_system_list();
CF_API Array<const char*> CF_CALL ecs_get_component_list_for_Entityype(const char* Entityype);

}

#endif // CF_CPP

#endif // CF_ECS_H
