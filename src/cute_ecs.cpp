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
#include <cute_kv.h>
#include <cute_log.h>

#include <internal/cute_app_internal.h>

namespace cute
{

system_interface_t::system_interface_t(const char* name, const char* component_name, component_type_t component_type)
	: m_name(name)
	, m_component_name(component_name)
	, m_component_type(component_type)
{
}

const char* system_interface_t::get_name() const
{
	return m_name;
}
const char* system_interface_t::get_component_name() const
{
	return m_component_name;
}

component_type_t system_interface_t::get_component_type() const
{
	return m_component_type;
}

//--------------------------------------------------------------------------------------------------

void app_add_system(app_t* app, system_interface_t* system)
{
	app->systems.add(system);
	app->system_names.add(system->get_name());
	app->system_component_names.add(system->get_component_name());
}

system_interface_t* app_get_system(app_t* app, const char* name)
{
	for (int i = 0; i < app->system_names.count(); ++i)
	{
		if (!CUTE_STRCMP(name, app->system_names[i])) {
			return app->systems[i];
		}
	}
	return NULL;
}

void app_set_update_systems_for_me_flag(app_t* app, bool true_to_update_false_to_do_nothing)
{
	app->udpate_systems_flag = true_to_update_false_to_do_nothing;
}

//--------------------------------------------------------------------------------------------------

void app_register_component(app_t* app, const component_config_t* component_config)
{
	app->component_name_to_type_table.insert(component_config->component_name, component_config->component_type);
	app->component_configs.insert(component_config->component_type, *component_config);
}

//--------------------------------------------------------------------------------------------------

error_t app_register_entity_schema(app_t* app, const char* entity_name, entity_type_t entity_type, const void* schema, int schema_size)
{
	kv_t* kv = kv_make(app->mem_ctx);
	error_t err = kv_reset_io(kv, schema, schema_size, CUTE_KV_MODE_READ);
	if (err.is_error()) {
		log(CUTE_LOG_LEVEL_ERROR, "Unable to find parse entity schema for %s.\n", entity_name);
		return err;
	}

	entity_schema_t entity_schema;
	entity_schema.entity_name = entity_name;
	entity_schema.entity.type = entity_type;
	entity_schema.parsed_kv_schema = kv;

	int component_config_count = app->component_configs.count();
	const component_config_t* component_configs = app->component_configs.items();
	for (int i = 0; i < component_config_count; ++i)
	{
		const component_config_t* config = component_configs + i;

		err = kv_key(kv, config->component_name);
		if (!err.is_error()) {
			component_type_t* component_type = app->component_name_to_type_table.find(config->component_name);
			if (!component_type) {
				log(CUTE_LOG_LEVEL_ERROR, "Unable to find type for component name %s.\n", config->component_name);
				return error_failure("Encountered invalid component name.");
			} else {
				entity_schema.entity.add(CUTE_INVALID_COMPONENT_ID, *component_type);
			}
		}
	}

	kv_reset_read(kv);
	app->entity_name_to_type_table.insert(entity_name, entity_type);
	app->entity_schemas.insert(entity_type, entity_schema);

	return error_success();
}

error_t app_load_entities(app_t* app, const void* memory, int size)
{
	return error_failure(NULL);
}

}
