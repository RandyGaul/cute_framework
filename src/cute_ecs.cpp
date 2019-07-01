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

//--------------------------------------------------------------------------------------------------

void app_register_system(app_t* app, system_fn* system_update_function, component_type_t* types, int types_count)
{
	system_t& system = app->systems.add();
	system.update_func = system_update_function;
	for (int i = 0; i < types_count; ++i) system.component_types.add(types[i]);
}

void app_register_entity_type(app_t* app, entity_type_t entity_type, component_type_t* types, int types_count)
{
	CUTE_ASSERT(types_count);
	entity_collection_t* collection = app->entity_collections.insert(entity_type);
	for (int i = 0; i < types_count; ++i)
	{
		collection->component_types.add(types[i]);
		collection->component_tables.add();
	}
}

entity_t app_make_entity(app_t* app, entity_type_t type)
{
	// TODO: Implement me.
	entity_t entity;
	entity.type = CUTE_INVALID_ENTITY_TYPE;
	entity.handle = CUTE_INVALID_HANDLE;
	return entity;
}

void app_destroy_entity(app_t* app, entity_t entity)
{
	// TODO: Implement me.
}

bool app_is_entity_valid(app_t* app, entity_t entity)
{
	return false; // TODO: Implement me.
}

static void s_1(float dt, system_fn* fn_uncasted, typeless_array& c0)
{
	void (*fn)(float, void*) = (void (*)(float, void*))fn_uncasted;
	int count = c0.count();

	for (int i = 0; i < count; ++i)
		fn(dt, c0[i]);
}

static void s_2(float dt, system_fn* fn_uncasted, typeless_array& c0, typeless_array& c1)
{
	CUTE_ASSERT(c0.count() == c1.count());
	void (*fn)(float, void*, void*) = (void (*)(float, void*, void*))fn_uncasted;
	int count = c0.count();

	for (int i = 0; i < count; ++i)
		fn(dt, c0[i], c1[i]);
}

static void s_3(float dt, system_fn* fn_uncasted, typeless_array& c0, typeless_array& c1, typeless_array& c2)
{
	CUTE_ASSERT(c0.count() == c1.count() == c2.count());
	void (*fn)(float, void*, void*, void*) = (void (*)(float, void*, void*, void*))fn_uncasted;
	int count = c0.count();

	for (int i = 0; i < count; ++i)
		fn(dt, c0[i], c1[i], c2[i]);
}

static inline void s_match(array<int>* matches, const array<component_type_t>& a, const array<component_type_t>& b)
{
	for (int i = 0; i < a.count(); ++i)
	{
		for (int j = 0; j < b.count(); ++j)
		{
			if (a[i] == b[j]) {
				matches->add(i);
				break;
			}
		}
	}
}

void app_update_systems(app_t* app)
{
	float dt = 0;
	int system_count = app->systems.count();
	for (int i = 0; i < system_count; ++i)
	{
		system_t* system = app->systems + i;
		system_fn* func = system->update_func;

		for (int j = 0; j < app->entity_collections.count(); ++j)
		{
			entity_collection_t* collection = app->entity_collections.items() + j;
			CUTE_ASSERT(collection->component_tables.count() == collection->component_types.count());
			int component_count = collection->component_tables.count();

			array<int> matches;
			s_match(&matches, system->component_types, collection->component_types);

			array<typeless_array>& tables = collection->component_tables;
			if (matches.count() == component_count) {
				switch (component_count)
				{
				case 1: s_1(dt, func, tables[matches[0]]); break;
				case 2: s_2(dt, func, tables[matches[0]], tables[matches[1]]); break;
				case 3: s_3(dt, func, tables[matches[0]], tables[matches[1]], tables[matches[2]]); break;
				default: CUTE_ASSERT(0);
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------

void app_register_component_type(app_t* app, const component_config_t* component_config)
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
	entity_schema.entity_type = entity_type;
	entity_schema.parsed_kv_schema = kv;

	int component_config_count = app->component_configs.count();
	const component_config_t* component_configs = app->component_configs.items();
	array<component_type_t> component_types; // TODO: Use me.
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
				component_types.add(*component_type);
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
	// TODO: Implement me.
	return error_failure(NULL);
}

}
