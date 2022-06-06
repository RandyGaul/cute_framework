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

namespace cute
{

struct cf_kv_t;

//--------------------------------------------------------------------------------------------------
// Entity

struct cf_entity_t
{
	CUTE_INLINE bool operator==(const cf_entity_t& other) { return handle == other.handle; }
	CUTE_INLINE bool operator!=(const cf_entity_t& other) { return handle != other.handle; }
	cf_handle_t handle; // For internal use -- don't touch.
};

static constexpr cf_entity_t CF_INVALID_ENTITY = { CUTE_INVALID_HANDLE };

CUTE_API void CUTE_CALL cf_ecs_entity_begin();
CUTE_API void CUTE_CALL cf_ecs_entity_end();
CUTE_API void CUTE_CALL cf_ecs_entity_set_name(const char* entity_type);
CUTE_API void CUTE_CALL cf_ecs_entity_add_component(const char* component_type);
CUTE_API void CUTE_CALL cf_ecs_entity_set_optional_schema(const char* schema);

CUTE_API cf_entity_t CUTE_CALL cf_entity_make(const char* entity_type, cf_error_t* err = NULL);
CUTE_API bool CUTE_CALL cf_entity_is_valid(cf_entity_t entity);
CUTE_API bool CUTE_CALL cf_entity_is_type(cf_entity_t entity, const char* entity_type);
CUTE_API const char* CUTE_CALL cf_entity_get_type_string(cf_entity_t entity);
CUTE_API bool CUTE_CALL cf_entity_has_component(cf_entity_t entity, const char* component_type);
CUTE_API void* CUTE_CALL cf_entity_get_component(cf_entity_t entity, const char* component_type);
CUTE_API void CUTE_CALL cf_entity_destroy(cf_entity_t entity);
CUTE_API void CUTE_CALL cf_entity_delayed_destroy(cf_entity_t entity);

/**
 * `kv` needs to be in `KV_STATE_READ` mode.
 */
CUTE_API cf_error_t CUTE_CALL cf_ecs_load_entities(cf_kv_t* kv, cf_array<cf_entity_t>* entities_out = NULL);

/**
 * `kv` needs to be in `KV_STATE_WRITE` mode.
 */
CUTE_API cf_error_t CUTE_CALL cf_ecs_save_entities(const cf_array<cf_entity_t>& entities, cf_kv_t* kv);
CUTE_API cf_error_t CUTE_CALL cf_ecs_save_entities(const cf_array<cf_entity_t>& entities);

//--------------------------------------------------------------------------------------------------
// Component

typedef cf_error_t (cf_component_serialize_fn)(cf_kv_t* kv, bool reading, cf_entity_t entity, void* component, void* udata);
typedef void (cf_component_cleanup_fn)(cf_entity_t entity, void* component, void* udata);

CUTE_API void CUTE_CALL cf_ecs_component_begin();
CUTE_API void CUTE_CALL cf_ecs_component_end();
CUTE_API void CUTE_CALL cf_ecs_component_set_name(const char* name);
CUTE_API void CUTE_CALL cf_ecs_component_set_size(size_t size);
CUTE_API void CUTE_CALL cf_ecs_component_set_optional_serializer(cf_component_serialize_fn* serializer_fn, void* udata = NULL);
CUTE_API void CUTE_CALL cf_ecs_component_set_optional_cleanup(cf_component_cleanup_fn* cleanup_fn, void* udata = NULL);

//--------------------------------------------------------------------------------------------------
// System

CUTE_API void CUTE_CALL cf_ecs_system_begin();
CUTE_API void CUTE_CALL cf_ecs_system_end();
CUTE_API void CUTE_CALL cf_ecs_system_set_name(const char* name);
CUTE_API void CUTE_CALL cf_ecs_system_set_update(void* update_fn);
CUTE_API void CUTE_CALL cf_ecs_system_require_component(const char* component_type);
CUTE_API void CUTE_CALL cf_ecs_system_set_optional_pre_update(void (*pre_update_fn)(float dt, void* udata));
CUTE_API void CUTE_CALL cf_ecs_system_set_optional_post_update(void (*post_update_fn)(float dt, void* udata));
CUTE_API void CUTE_CALL cf_ecs_system_set_optional_update_udata(void* udata);

CUTE_API void CUTE_CALL cf_ecs_run_systems(float dt);

//--------------------------------------------------------------------------------------------------
// Introspection

CUTE_API bool CUTE_CALL cf_ecs_is_entity_type_valid(const char* entity_type);
CUTE_API cf_array<const char*> CUTE_CALL cf_ecs_get_entity_list();
CUTE_API cf_array<const char*> CUTE_CALL cf_ecs_get_component_list();
CUTE_API cf_array<const char*> CUTE_CALL cf_ecs_get_system_list();
CUTE_API cf_array<const char*> CUTE_CALL cf_ecs_get_component_list_for_entity_type(const char* entity_type);

}

#endif // CUTE_ECS_H
