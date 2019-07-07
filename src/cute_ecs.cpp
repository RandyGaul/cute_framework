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
	auto fn = (void (*)(float, void*))fn_uncasted;
	int count = c0.count();

	for (int i = 0; i < count; ++i)
		fn(dt, c0[i]);
}

static void s_2(float dt, system_fn* fn_uncasted, typeless_array& c0, typeless_array& c1)
{
	CUTE_ASSERT(c0.count() == c1.count());
	auto fn = (void (*)(float, void*, void*))fn_uncasted;
	int count = c0.count();

	for (int i = 0; i < count; ++i)
		fn(dt, c0[i], c1[i]);
}

static void s_3(float dt, system_fn* fn_uncasted, typeless_array& c0, typeless_array& c1, typeless_array& c2)
{
	CUTE_ASSERT(c0.count() == c1.count() && c0.count() == c2.count());
	auto fn = (void (*)(float, void*, void*, void*))fn_uncasted;
	int count = c0.count();

	for (int i = 0; i < count; ++i)
		fn(dt, c0[i], c1[i], c2[i]);
}

static void s_4(float dt, system_fn* fn_uncasted, typeless_array& c0, typeless_array& c1, typeless_array& c2, typeless_array& c3)
{
	CUTE_ASSERT(c0.count() == c1.count() && c0.count() == c2.count() && c0.count() == c3.count());
	auto fn = (void (*)(float, void*, void*, void*, void*))fn_uncasted;
	int count = c0.count();

	for (int i = 0; i < count; ++i)
		fn(dt, c0[i], c1[i], c2[i], c3[i]);
}

static void s_5(float dt, system_fn* fn_uncasted, typeless_array& c0, typeless_array& c1, typeless_array& c2, typeless_array& c3, typeless_array& c4)
{
	CUTE_ASSERT(c0.count() == c1.count() && c0.count() == c2.count() && c0.count() == c3.count() && c0.count() == c4.count());
	auto fn = (void (*)(float, void*, void*, void*, void*, void*))fn_uncasted;
	int count = c0.count();

	for (int i = 0; i < count; ++i)
		fn(dt, c0[i], c1[i], c2[i], c3[i], c4[i]);
}

static void s_6(float dt, system_fn* fn_uncasted, typeless_array& c0, typeless_array& c1, typeless_array& c2, typeless_array& c3, typeless_array& c4, typeless_array& c5)
{
	CUTE_ASSERT(c0.count() == c1.count() && c0.count() == c2.count() && c0.count() == c3.count() && c0.count() == c4.count() && c0.count() == c5.count());
	auto fn = (void (*)(float, void*, void*, void*, void*, void*, void*))fn_uncasted;
	int count = c0.count();

	for (int i = 0; i < count; ++i)
		fn(dt, c0[i], c1[i], c2[i], c3[i], c4[i], c5[i]);
}

static void s_7(float dt, system_fn* fn_uncasted, typeless_array& c0, typeless_array& c1, typeless_array& c2, typeless_array& c3, typeless_array& c4, typeless_array& c5, typeless_array& c6)
{
	CUTE_ASSERT(c0.count() == c1.count() && c0.count() == c2.count() && c0.count() == c3.count() && c0.count() == c4.count() && c0.count() == c5.count() && c0.count() == c6.count());
	auto fn = (void (*)(float, void*, void*, void*, void*, void*, void*, void*))fn_uncasted;
	int count = c0.count();

	for (int i = 0; i < count; ++i)
		fn(dt, c0[i], c1[i], c2[i], c3[i], c4[i], c5[i], c6[i]);
}

static void s_8(float dt, system_fn* fn_uncasted, typeless_array& c0, typeless_array& c1, typeless_array& c2, typeless_array& c3, typeless_array& c4, typeless_array& c5, typeless_array& c6, typeless_array& c7)
{
	CUTE_ASSERT(c0.count() == c1.count() && c0.count() == c2.count() && c0.count() == c3.count() && c0.count() == c4.count() && c0.count() == c5.count() && c0.count() == c6.count() && c0.count() == c7.count());
	auto fn = (void (*)(float, void*, void*, void*, void*, void*, void*, void*, void*))fn_uncasted;
	int count = c0.count();

	for (int i = 0; i < count; ++i)
		fn(dt, c0[i], c1[i], c2[i], c3[i], c4[i], c5[i], c6[i], c7[i]);
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
				case 4: s_4(dt, func, tables[matches[0]], tables[matches[1]], tables[matches[2]], tables[matches[3]]); break;
				case 5: s_5(dt, func, tables[matches[0]], tables[matches[1]], tables[matches[2]], tables[matches[3]], tables[matches[4]]); break;
				case 6: s_6(dt, func, tables[matches[0]], tables[matches[1]], tables[matches[2]], tables[matches[3]], tables[matches[4]], tables[matches[5]]); break;
				case 7: s_7(dt, func, tables[matches[0]], tables[matches[1]], tables[matches[2]], tables[matches[3]], tables[matches[4]], tables[matches[5]], tables[matches[6]]); break;
				case 8: s_8(dt, func, tables[matches[0]], tables[matches[1]], tables[matches[2]], tables[matches[3]], tables[matches[4]], tables[matches[5]], tables[matches[6]], tables[matches[7]]); break;
				default: CUTE_ASSERT(0);
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------

error_t app_register_component_type(app_t* app, const component_config_t* component_config)
{
	app->component_name_to_type_table.insert(component_config->name, component_config->type);
	app->component_type_to_name_table.insert(component_config->type, component_config->name);
	app->component_configs.insert(component_config->type, *component_config);
	return error_success();
}

//--------------------------------------------------------------------------------------------------

error_t app_register_entity_type(app_t* app, const entity_config_t* config)
{
	// Register serialization schema.
	kv_t* kv = kv_make(app->mem_ctx);
	error_t err = kv_parse(kv, config->schema, config->schema_size);
	if (err.is_error()) {
		log(CUTE_LOG_LEVEL_ERROR, "Unable to find parse entity schema for %s.\n", config->name);
		return err;
	}

	entity_schema_t entity_schema;
	entity_schema.entity_name = config->name;
	entity_schema.entity_type = config->type;
	entity_schema.parsed_kv_schema = kv;

	int component_config_count = app->component_configs.count();
	const component_config_t* component_configs = app->component_configs.items();
	array<component_type_t> component_types;
	for (int i = 0; i < component_config_count; ++i)
	{
		const component_config_t* config = component_configs + i;

		err = kv_key(kv, config->name);
		if (!err.is_error()) {
			component_type_t* component_type = app->component_name_to_type_table.find(config->name);
			if (!component_type) {
				log(CUTE_LOG_LEVEL_ERROR, "Unable to find type for component name %s.\n", config->name);
				return error_failure("Encountered invalid component name.");
			} else {
				component_types.add(*component_type);
			}
		}
	}

	// Register types.
	entity_collection_t* collection = app->entity_collections.insert(config->type);
	for (int i = 0; i < component_types.count(); ++i)
	{
		collection->component_types.add(component_types[i]);
		typeless_array& table = collection->component_tables.add();
		component_config_t* config = app->component_configs.find(component_types[i]);
		table.m_element_size = config->size;
	}

	kv_reset_read_state(kv);
	app->entity_name_to_type_table.insert(config->name, config->type);
	app->entity_schemas.insert(config->type, entity_schema);

	return error_success();
}

error_t app_load_entities(app_t* app, const void* memory, size_t size)
{
	kv_t* kv = kv_make(app->mem_ctx);
	error_t err = kv_parse(kv, memory, size);
	if (err.is_error()) {
		return err;
	}

	err = kv_key(kv, "entities");
	if (err.is_error()) {
		return err;
	}

	// WORKING HERE
	// Using array of entities, and not top level objects.
	// Requires a fairly big refactor of kv.
	// But it's all for the best.
	kv_val_

	while (kv_has_more_objects_to_read(kv))
	{
		kv_object_begin(kv);

		const char* entity_type_str = NULL;
		size_t sz = 0;
		kv_key(kv, "entity_type");
		kv_val_string(kv, &entity_type_str, &sz);

		entity_type_t* entity_type_ptr = app->entity_name_to_type_table.find(entity_type_str, sz);
		if (!entity_type_ptr) {
			return error_failure("Unable to find entity type.");
		}

		entity_type_t type = *entity_type_ptr;
		entity_collection_t* collection = app->entity_collections.find(type);
		CUTE_ASSERT(collection);

		const array<component_type_t>& types = collection->component_types;
		for (int i = 0; i < types.count(); ++i)
		{
			const char** ptr = app->component_type_to_name_table.find(types[i]);
			CUTE_ASSERT(ptr);
			const char* type_str = *ptr;
			error_t err = kv_key(kv, type_str);

			component_config_t* config = app->component_configs.find(types[i]);
			if (!config) {
				return error_failure("Unable to find component config.");
			}

			int index = collection->component_tables.count();
			void* component = collection->component_tables[i].add();
			config->initializer_fn(component);

			if (!err.is_error()) {
				kv_object_begin(kv);
				config->serializer_fn(kv, component);
				kv_object_end(kv);
			}

			handle_t h = collection->entity_handle_table.alloc_handle(index);
			collection->entity_handles.add(h);
		}

		kv_object_end(kv);
	}

	kv_destroy(kv);

	return error_success();
}

}
