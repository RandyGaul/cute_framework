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

#include <cute_error.h>
#include <cute_handle_table.h>
#include <cute_array.h>
#include <cute_typeless_array.h>
#include <cute_dictionary.h>

#include <internal/cute_object_table_internal.h>

namespace cute
{

struct kv_t;

//--------------------------------------------------------------------------------------------------
// Entity

using entity_type_t = uint32_t;
#define CUTE_INVALID_ENTITY_TYPE ((entity_type_t)(~0))

struct entity_t
{
	entity_type_t type;
	handle_t handle;
};

extern CUTE_API entity_type_t CUTE_CALL app_register_entity_type(app_t* app, const char* schema);
extern CUTE_API const char* CUTE_CALL app_entity_type_string(app_t* app, entity_type_t type);

//--------------------------------------------------------------------------------------------------
// Component

typedef error_t (component_serialize_fn)(app_t* app, kv_t* kv, entity_t entity, void* component, void* udata);

struct component_config_t
{
	const char* name = NULL;
	size_t size_of_component = 0;
	void* serializer_fn_udata = NULL;
	component_serialize_fn* serializer_fn = NULL;
};

extern CUTE_API void CUTE_CALL app_register_component_type(app_t* app, component_config_t component_config);
// TODO - Dependencies.

//--------------------------------------------------------------------------------------------------
// System

struct system_t
{
	void* update_fn_udata = NULL;
	void* update_fn = NULL;
	array<const char*> component_types;
};

extern CUTE_API void CUTE_CALL app_register_system(app_t* app, system_t system);
extern CUTE_API void CUTE_CALL app_update_systems(app_t* app, float dt);

//--------------------------------------------------------------------------------------------------
// Run-time functions and entity lifetime management.

extern CUTE_API error_t CUTE_CALL app_make_entity(app_t* app, const char* entity_type, entity_t* entity_out = NULL);
extern CUTE_API void CUTE_CALL app_destroy_entity(app_t* app, entity_t entity);
extern CUTE_API bool CUTE_CALL app_is_entity_valid(app_t* app, entity_t entity);
extern CUTE_API void* CUTE_CALL app_get_component(app_t* app, entity_t entity, const char* name);

/**
 * `kv` needs to be in `KV_STATE_READ` mode.
 */
extern CUTE_API error_t CUTE_CALL app_load_entities(app_t* app, kv_t* kv, array<entity_t>* entities_out = NULL);

/**
 * `kv` needs to be in `KV_STATE_WRITE` mode.
 */
extern CUTE_API error_t CUTE_CALL app_save_entities(app_t* app, const array<entity_t>& entities, kv_t* kv);


}

#endif // CUTE_ECS_H
