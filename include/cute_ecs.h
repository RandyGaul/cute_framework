/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

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

#ifndef CUTE_ECS_H
#define CUTE_ECS_H

#include "cute_error.h"
#include "cute_handle_table.h"
#include "cute_array.h"
#include "cute_typeless_array.h"
#include "cute_dictionary.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct cf_kv_t cf_kv_t;

//--------------------------------------------------------------------------------------------------
// Entity

typedef struct cf_entity_t
{
	#ifdef CUTE_CPP
	CUTE_INLINE bool operator==(const cf_entity_t& other) { return handle == other.handle; }
	CUTE_INLINE bool operator!=(const cf_entity_t& other) { return handle != other.handle; }
	#endif // CUTE_CPP

	cf_handle_t handle; // For internal use -- don't touch.
} cf_entity_t;


static cf_entity_t CF_INVALID_ENTITY = { CUTE_INVALID_HANDLE };

CUTE_API void CUTE_CALL cf_ecs_entity_begin();
CUTE_API void CUTE_CALL cf_ecs_entity_end();
CUTE_API void CUTE_CALL cf_ecs_entity_set_name(const char* entity_type);
CUTE_API void CUTE_CALL cf_ecs_entity_add_component(const char* component_type);
CUTE_API void CUTE_CALL cf_ecs_entity_set_optional_schema(const char* schema);

CUTE_API cf_entity_t CUTE_CALL cf_entity_make(const char* entity_type, cf_error_t* err /*= NULL*/);
CUTE_API bool CUTE_CALL cf_entity_is_valid(cf_entity_t entity);
CUTE_API bool CUTE_CALL cf_entity_is_type(cf_entity_t entity, const char* entity_type);
CUTE_API const char* CUTE_CALL cf_entity_get_type_string(cf_entity_t entity);
CUTE_API bool CUTE_CALL cf_entity_has_component(cf_entity_t entity, const char* component_type);
CUTE_API void* CUTE_CALL cf_entity_get_component(cf_entity_t entity, const char* component_type);
CUTE_API void CUTE_CALL cf_entity_destroy(cf_entity_t entity);
CUTE_API void CUTE_CALL cf_entity_delayed_destroy(cf_entity_t entity);

CUTE_INLINE bool cf_entity_equals(cf_entity_t* a, cf_entity_t* b) { return a->handle == b->handle; }

CUTE_API void CUTE_CALL cf_entity_delayed_deactivate(cf_entity_t entity);
CUTE_API void CUTE_CALL cf_entity_delayed_activate(cf_entity_t entity);
CUTE_API void CUTE_CALL cf_entity_deactivate(cf_entity_t entity);
CUTE_API void CUTE_CALL cf_entity_activate(cf_entity_t entity);
CUTE_API bool CUTE_CALL cf_entity_is_active(cf_entity_t entity);

/**
 * `kv` needs to be in `KV_STATE_READ` mode.
 */
CUTE_API cf_error_t CUTE_CALL cf_ecs_load_entities(cf_kv_t* kv, cf_entity_t** entities_out /*= NULL*/, int* entities_count_out /*= NULL*/);
CUTE_API void CUTE_CALL cf_ecs_free_entities(cf_entity_t* entities);

/**
 * `kv` needs to be in `KV_STATE_WRITE` mode.
 */
CUTE_API cf_error_t CUTE_CALL cf_ecs_save_entities_kv(const cf_entity_t* entities, int entities_count, cf_kv_t* kv);
CUTE_API cf_error_t CUTE_CALL cf_ecs_save_entities(const cf_entity_t* entities, int entities_count);

//--------------------------------------------------------------------------------------------------
// Component

typedef cf_error_t(cf_component_serialize_fn)(cf_kv_t* kv, bool reading, cf_entity_t entity, void* component, void* udata);
typedef void (cf_component_cleanup_fn)(cf_entity_t entity, void* component, void* udata);

CUTE_API void CUTE_CALL cf_ecs_component_begin();
CUTE_API void CUTE_CALL cf_ecs_component_end();
CUTE_API void CUTE_CALL cf_ecs_component_set_name(const char* name);
CUTE_API void CUTE_CALL cf_ecs_component_set_size(size_t size);
CUTE_API void CUTE_CALL cf_ecs_component_set_optional_serializer(cf_component_serialize_fn* serializer_fn, void* udata /*= NULL*/);
CUTE_API void CUTE_CALL cf_ecs_component_set_optional_cleanup(cf_component_cleanup_fn* cleanup_fn, void* udata /*= NULL*/);

//--------------------------------------------------------------------------------------------------
// System

typedef struct cf_ecs_arrays_t cf_ecs_arrays_t;

typedef void (system_update_fn)(float dt, cf_ecs_arrays_t* arrays, int count, void* udata);
CUTE_API void* CUTE_CALL cf_ecs_arrays_find_components(cf_ecs_arrays_t* arrays, const char* component_type);
CUTE_API entity_t* CUTE_CALL cf_ecs_arrays_get_entities(cf_ecs_arrays_t* arrays);

CUTE_API void CUTE_CALL cf_ecs_system_begin();
CUTE_API void CUTE_CALL cf_ecs_system_end();
CUTE_API void CUTE_CALL cf_ecs_system_set_name(const char* name);
CUTE_API void CUTE_CALL cf_ecs_system_set_update(system_update_fn* update_fn);
CUTE_API void CUTE_CALL cf_ecs_system_require_component(const char* component_type);
CUTE_API void CUTE_CALL cf_ecs_system_set_optional_pre_update(void (*pre_update_fn)(float dt, void* udata));
CUTE_API void CUTE_CALL cf_ecs_system_set_optional_post_update(void (*post_update_fn)(float dt, void* udata));
CUTE_API void CUTE_CALL cf_ecs_system_set_optional_update_udata(void* udata);

CUTE_API void CUTE_CALL cf_ecs_run_systems(float dt);

//--------------------------------------------------------------------------------------------------
// Introspection

CUTE_API bool CUTE_CALL cf_ecs_is_entity_type_valid(const char* entity_type);
CUTE_API const char** CUTE_CALL cf_ecs_get_entity_list(int* entities_count_out /*optional*/);
CUTE_API const char** CUTE_CALL cf_ecs_get_component_list(int* components_count_out /*optional*/);
CUTE_API const char** CUTE_CALL cf_ecs_get_system_list(int* systems_count_out /*optional*/);
CUTE_API const char** CUTE_CALL cf_ecs_get_component_list_for_entity_type(const char* entity_type, int* components_count_out /*optional*/);
CUTE_API void CUTE_CALL cf_ecs_free_list(const char** list);

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef CUTE_CPP

namespace cute
{
using kv_t = cf_kv_t;
using entity_t = cf_entity_t;
using component_serialize_fn = cf_component_serialize_fn;
using component_cleanup_fn = cf_component_cleanup_fn;

//--------------------------------------------------------------------------------------------------
// Entity

static constexpr entity_t INVALID_ENTITY = { CUTE_INVALID_HANDLE };

CUTE_INLINE void ecs_entity_begin() { cf_ecs_entity_begin(); }
CUTE_INLINE void ecs_entity_end() { cf_ecs_entity_end(); }
CUTE_INLINE void ecs_entity_set_name(const char* entity_type) { cf_ecs_entity_set_name(entity_type); }
CUTE_INLINE void ecs_entity_add_component(const char* component_type) { cf_ecs_entity_add_component(component_type); }
CUTE_INLINE void ecs_entity_set_optional_schema(const char* schema) { cf_ecs_entity_set_optional_schema(schema); }

CUTE_INLINE entity_t entity_make(const char* entity_type, error_t* err = NULL) { return cf_entity_make(entity_type, err); }
CUTE_INLINE bool entity_is_valid(entity_t entity) { return cf_entity_is_valid(entity); }
CUTE_INLINE bool entity_is_type(entity_t entity, const char* entity_type) { return cf_entity_is_type(entity, entity_type); }
CUTE_INLINE const char* entity_get_type_string(entity_t entity) { return cf_entity_get_type_string(entity); }
CUTE_INLINE bool entity_has_component(entity_t entity, const char* component_type) { return cf_entity_has_component(entity, component_type); }
CUTE_INLINE void* entity_get_component(entity_t entity, const char* component_type) { return cf_entity_get_component(entity, component_type); }
CUTE_INLINE void entity_destroy(entity_t entity) { cf_entity_destroy(entity); }
CUTE_INLINE void entity_delayed_destroy(entity_t entity) { cf_entity_delayed_destroy(entity); }

CUTE_API error_t CUTE_CALL ecs_load_entities(kv_t* kv, array<entity_t>* entities_out = NULL);

CUTE_API error_t CUTE_CALL ecs_save_entities(const array<entity_t>& entities, kv_t* kv);
CUTE_API error_t CUTE_CALL ecs_save_entities(const array<entity_t>& entities);

//--------------------------------------------------------------------------------------------------
// Component

CUTE_INLINE void ecs_component_begin() { cf_ecs_component_begin(); }
CUTE_INLINE void ecs_component_end() { cf_ecs_component_end(); }
CUTE_INLINE void ecs_component_set_name(const char* name) { cf_ecs_component_set_name(name); }
CUTE_INLINE void ecs_component_set_size(size_t size) { cf_ecs_component_set_size(size); }
CUTE_INLINE void ecs_component_set_optional_serializer(component_serialize_fn* serializer_fn, void* udata = NULL) { cf_ecs_component_set_optional_serializer(serializer_fn, udata); }
CUTE_INLINE void ecs_component_set_optional_cleanup(component_cleanup_fn* cleanup_fn, void* udata = NULL) { cf_ecs_component_set_optional_cleanup(cleanup_fn, udata); }

//--------------------------------------------------------------------------------------------------
// System

CUTE_INLINE void ecs_system_begin() { cf_ecs_system_begin(); }
CUTE_INLINE void ecs_system_end() { cf_ecs_system_end(); }
CUTE_INLINE void ecs_system_set_name(const char* name) { cf_ecs_system_set_name(name); }
CUTE_INLINE void ecs_system_set_update(system_update_fn* update_fn) { cf_ecs_system_set_update(update_fn); }
CUTE_INLINE void ecs_system_require_component(const char* component_type) { cf_ecs_system_require_component(component_type); }
CUTE_INLINE void ecs_system_set_optional_pre_update(void (*pre_update_fn)(float dt, void* udata)) { cf_ecs_system_set_optional_pre_update(pre_update_fn); }
CUTE_INLINE void ecs_system_set_optional_post_update(void (*post_update_fn)(float dt, void* udata)) { cf_ecs_system_set_optional_post_update(post_update_fn); }
CUTE_INLINE void ecs_system_set_optional_update_udata(void* udata) { cf_ecs_system_set_optional_update_udata(udata); }

CUTE_INLINE void ecs_run_systems(float dt) { cf_ecs_run_systems(dt); }

//--------------------------------------------------------------------------------------------------
// Introspection

CUTE_INLINE bool ecs_is_entity_type_valid(const char* entity_type) { return cf_ecs_is_entity_type_valid(entity_type); }
CUTE_API array<const char*> CUTE_CALL ecs_get_entity_list();
CUTE_API array<const char*> CUTE_CALL ecs_get_component_list();
CUTE_API array<const char*> CUTE_CALL ecs_get_system_list();
CUTE_API array<const char*> CUTE_CALL ecs_get_component_list_for_entity_type(const char* entity_type);
}

#endif // CUTE_CPP

#endif // CUTE_ECS_H
