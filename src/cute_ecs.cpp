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

#include <cute_ecs.h>
#include <cute_c_runtime.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_ecs_internal.h>

namespace cute
{

system_t::system_t(const char* name, const char* component_name, component_type_t component_type, int component_size, int max_components, int reserve_count, void* user_allocator_context)
	: m_name(name)
	, m_component_name(component_name)
	, m_component_type(component_type)
	, m_components(ecs_allocator_make(component_size, max_components, reserve_count, user_allocator_context))
{
}

system_t::~system_t()
{
	ecs_allocator_destroy(m_components);
}

component_id_t system_t::add_component(const component_t* component)
{
	return ecs_allocator_allocate(m_components, component);
}

error_t system_t::get_component(component_id_t id, component_t* component)
{
	return ecs_allocator_get_object(m_components, id, component);
}

void system_t::remove_component(component_id_t id)
{
	ecs_allocator_remove_object(m_components, id);
}

void system_t::remove_component(int index)
{
	ecs_allocator_remove_object(m_components, index);
}

bool system_t::has_component(component_id_t id) const
{
	return ecs_allocator_has_object(m_components, id);
}

void* system_t::get_components()
{
	return ecs_allocator_get_objects(m_components);
}

const void* system_t::get_components() const
{
	return ecs_allocator_get_objects(m_components);
}

int system_t::get_components_count()
{
	return ecs_allocator_get_object_count(m_components);
}

const char* system_t::get_name() const
{
	return m_name;
}

const char* system_t::get_component_name() const
{
	return m_component_name;
}

component_type_t system_t::get_component_type() const
{
	return m_component_type;
}

//--------------------------------------------------------------------------------------------------

void app_add_system(app_t* app, system_t* system, const char* system_name, const char* component_name)
{
	CUTE_ASSERT(0);
}

system_t* app_get_system(app_t* app, const char* name)
{
	CUTE_ASSERT(0);
	return NULL;
}

void app_set_dont_update_systems_for_me_flag(app_t* app)
{
	CUTE_ASSERT(0);
}

//--------------------------------------------------------------------------------------------------

void app_register_component(app_t* app, const component_config_t* component_config)
{
}

//--------------------------------------------------------------------------------------------------

error_t app_register_entity_schema(app_t* app, const char* entity_name, entity_id_t entity_id, const void* schema, int schema_size)
{
	return error_failure(NULL);
}

error_t app_load_entities(app_t* app, const void* memory, int size)
{
	return error_failure(NULL);
}

}
