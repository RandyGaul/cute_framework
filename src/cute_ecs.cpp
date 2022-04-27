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
#include <cute_defer.h>
#include <cute_string.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_object_table_internal.h>

#define INJECT(s) strpool_inject(app->strpool, s, (int)CUTE_STRLEN(s))

namespace cute
{

static error_t s_load_from_schema(entity_type_t schema_type, entity_t entity, component_config_t* config, void* component, void* udata)
{
	// Look for parent.
	// If parent exists, load values from it first.
	entity_type_t inherits_from = INVALID_ENTITY_TYPE;
	error_t err = app->entity_schema_inheritence.find(schema_type, &inherits_from);
	if (!err.is_error()) {
		err = s_load_from_schema(inherits_from, entity, config, component, udata);
		if (err.is_error()) return err;
	}

	kv_t* schema = NULL;
	err = app->entity_parsed_schemas.find(schema_type, &schema);
	if (err.is_error()) {
		err = error_success();
		if (config->serializer_fn) err = config->serializer_fn(schema, true, entity, component, udata);
	} else {
		err = kv_object_begin(schema, config->name);
		if (!err.is_error()) {
			if (config->serializer_fn) err = config->serializer_fn(schema, true, entity, component, udata);
			if (err.is_error()) return err;
			err = kv_object_end(schema);
		}
	}

	return err;
}

//--------------------------------------------------------------------------------------------------

void ecs_system_begin()
{
	app->system_internal_builder.clear();
}

void ecs_system_end()
{
	app->systems.add(app->system_internal_builder);
}

void ecs_system_set_name(const char* name)
{
	app->system_internal_builder.name = INJECT(name);
}

void ecs_system_set_update(void* update_fn)
{
	app->system_internal_builder.update_fn = update_fn;
}

void ecs_system_require_component(const char* component_type)
{
	app->system_internal_builder.component_type_tuple.add(INJECT(component_type));
}

void ecs_system_set_optional_pre_update(void (*pre_update_fn)(float dt, void* udata))
{
	app->system_internal_builder.pre_update_fn = pre_update_fn;
}

void ecs_system_set_optional_post_update(void (*post_update_fn)(float dt, void* udata))
{
	app->system_internal_builder.post_update_fn = post_update_fn;
}

void ecs_system_set_optional_update_udata(void* udata)
{
	app->system_internal_builder.udata = udata;
}

static CUTE_INLINE uint16_t s_entity_type(entity_t entity)
{
	return (uint16_t)((entity.handle & 0x00000000FFFF0000ULL) >> 16);
}

entity_t entity_make(const char* entity_type, error_t* err_out)
{
	entity_type_t type = INVALID_ENTITY_TYPE;
	app->entity_type_string_to_id.find(INJECT(entity_type), &type);
	if (type == INVALID_ENTITY_TYPE) {
		if (err_out) *err_out = error_failure("`entity_type` is not valid.");
		return INVALID_ENTITY;
	}

	entity_collection_t* collection = app->entity_collections.find(type);
	CUTE_ASSERT(collection);

	int index = collection->entity_handles.count();
	handle_t h = collection->entity_handle_table.alloc_handle(index, type);
	collection->entity_handles.add(h);
	entity_t entity = { h };

	const array<strpool_id>& component_type_tuple = collection->component_type_tuple;
	for (int i = 0; i < component_type_tuple.count(); ++i)
	{
		strpool_id component_type = component_type_tuple[i];
		component_config_t* config = app->component_configs.find(component_type);

		if (!config) {
			if (err_out) *err_out = error_failure("Unable to find component config.");
			return INVALID_ENTITY;
		}

		void* component = collection->component_tables[i].add();
		error_t err = s_load_from_schema(type, entity, config, component, config->serializer_udata);
		if (err.is_error()) {
			// TODO - Unload the components that were added with `.add()` a couple lines above here.
			return INVALID_ENTITY;
		}
	}

	if (err_out) *err_out = error_success();
	return entity;
}

static entity_collection_t* s_collection(entity_t entity)
{
	entity_collection_t* collection = NULL;
	uint16_t entity_type = s_entity_type(entity);
	if (entity_type == app->current_collection_type_being_iterated) {
		// Fast path -- check the current entity collection for this entity type first.
		collection = app->current_collection_being_updated;
		CUTE_ASSERT(collection);
	} else {
		// Slightly slower path -- lookup collection first.
		collection = app->entity_collections.find(entity_type);
		if (!collection) return NULL;
	}
	return collection;
}

void entity_delayed_destroy(entity_t entity)
{
	app->delayed_destroy_entities.add(entity);
}

void entity_destroy(entity_t entity)
{
	uint16_t entity_type = s_entity_type(entity);
	entity_collection_t* collection = app->entity_collections.find(entity_type);
	CUTE_ASSERT(collection);

	if (collection->entity_handle_table.is_valid(entity.handle)) {
		int index = collection->entity_handle_table.get_index(entity.handle);

		// Call cleanup function on each component.
		for (int i = 0; i < collection->component_tables.count(); ++i) {
			component_config_t config;
			app->component_configs.find(collection->component_type_tuple[i], &config);
			if (config.cleanup_fn) {
				config.cleanup_fn(entity, collection->component_tables[i][index], config.cleanup_udata);
			}
		}

		// Update index in case user changed it (by destroying enties).
		index = collection->entity_handle_table.get_index(entity.handle);

		// Free the handle.
		collection->entity_handles.unordered_remove(index);
		collection->entity_handle_table.free_handle(entity.handle);

		// Free each component.
		for (int i = 0; i < collection->component_tables.count(); ++i) {
			collection->component_tables[i].unordered_remove(index);
		}

		// Update handle of the swapped entity.
		if (index < collection->entity_handles.size()) {
			uint64_t h = collection->entity_handles[index];
			collection->entity_handle_table.update_index(h, index);
		}
	}
}

bool entity_is_valid(entity_t entity)
{
	entity_collection_t* collection = s_collection(entity);
	if (collection) return collection->entity_handle_table.is_valid(entity.handle);
	else return false;
}

void* entity_get_component(entity_t entity, const char* component_type)
{
	entity_collection_t* collection = s_collection(entity);
	if (!collection) return NULL;

	strpool_id type = INJECT(component_type);
	const array<strpool_id>& component_type_tuple = collection->component_type_tuple;
	for (int i = 0; i < component_type_tuple.count(); ++i)
	{
		if (component_type_tuple[i].val == type.val) {
			int index = collection->entity_handle_table.get_index(entity.handle);
			return collection->component_tables[i][index];
		}
	}

	return NULL;
}

bool entity_has_component(entity_t entity, const char* component_type)
{
	return entity_get_component(entity, component_type) ? true : false;
}

//--------------------------------------------------------------------------------------------------

static void s_0(float dt, void* fn_uncasted, void* udata)
{
	auto fn = (void (*)(float, void*))fn_uncasted;
	fn(dt, udata);
}

static void s_1(float dt, void* fn_uncasted, void* udata, typeless_array& c0)
{
	auto fn = (void (*)(float, void*, void*, int))fn_uncasted;
	int count = c0.count();
	fn(dt, udata, c0.data(), count);
}

static void s_2(float dt, void* fn_uncasted, void* udata, typeless_array& c0, typeless_array& c1)
{
	CUTE_ASSERT(c0.count() == c1.count());
	auto fn = (void (*)(float, void*, void*, void*, int))fn_uncasted;
	int count = c0.count();
	fn(dt, udata, c0.data(), c1.data(), count);
}

static void s_3(float dt, void* fn_uncasted, void* udata, typeless_array& c0, typeless_array& c1, typeless_array& c2)
{
	CUTE_ASSERT(c0.count() == c1.count() && c0.count() == c2.count());
	auto fn = (void (*)(float, void*, void*, void*, void*, int))fn_uncasted;
	int count = c0.count();
	fn(dt, udata, c0.data(), c1.data(), c2.data(), count);
}

static void s_4(float dt, void* fn_uncasted, void* udata, typeless_array& c0, typeless_array& c1, typeless_array& c2, typeless_array& c3)
{
	CUTE_ASSERT(c0.count() == c1.count() && c0.count() == c2.count() && c0.count() == c3.count());
	auto fn = (void (*)(float, void*, void*, void*, void*, void*, int))fn_uncasted;
	int count = c0.count();
	fn(dt, udata, c0.data(), c1.data(), c2.data(), c3.data(), count);
}

static void s_5(float dt, void* fn_uncasted, void* udata, typeless_array& c0, typeless_array& c1, typeless_array& c2, typeless_array& c3, typeless_array& c4)
{
	CUTE_ASSERT(c0.count() == c1.count() && c0.count() == c2.count() && c0.count() == c3.count() && c0.count() == c4.count());
	auto fn = (void (*)(float, void*, void*, void*, void*, void*, void*, int))fn_uncasted;
	int count = c0.count();
	fn(dt, udata, c0.data(), c1.data(), c2.data(), c3.data(), c4.data(), count);
}

static void s_6(float dt, void* fn_uncasted, void* udata, typeless_array& c0, typeless_array& c1, typeless_array& c2, typeless_array& c3, typeless_array& c4, typeless_array& c5)
{
	CUTE_ASSERT(c0.count() == c1.count() && c0.count() == c2.count() && c0.count() == c3.count() && c0.count() == c4.count() && c0.count() == c5.count());
	auto fn = (void (*)(float, void*, void*, void*, void*, void*, void*, void*, int))fn_uncasted;
	int count = c0.count();
	fn(dt, udata, c0.data(), c1.data(), c2.data(), c3.data(), c4.data(), c5.data(), count);
}

static void s_7(float dt, void* fn_uncasted, void* udata, typeless_array& c0, typeless_array& c1, typeless_array& c2, typeless_array& c3, typeless_array& c4, typeless_array& c5, typeless_array& c6)
{
	CUTE_ASSERT(c0.count() == c1.count() && c0.count() == c2.count() && c0.count() == c3.count() && c0.count() == c4.count() && c0.count() == c5.count() && c0.count() == c6.count());
	auto fn = (void (*)(float, void*, void*, void*, void*, void*, void*, void*, void*, int))fn_uncasted;
	int count = c0.count();
	fn(dt, udata, c0.data(), c1.data(), c2.data(), c3.data(), c4.data(), c5.data(), c6.data(), count);
}

static void s_8(float dt, void* fn_uncasted, void* udata, typeless_array& c0, typeless_array& c1, typeless_array& c2, typeless_array& c3, typeless_array& c4, typeless_array& c5, typeless_array& c6, typeless_array& c7)
{
	CUTE_ASSERT(c0.count() == c1.count() && c0.count() == c2.count() && c0.count() == c3.count() && c0.count() == c4.count() && c0.count() == c5.count() && c0.count() == c6.count() && c0.count() == c7.count());
	auto fn = (void (*)(float, void*, void*, void*, void*, void*, void*, void*, void*, void*, int))fn_uncasted;
	int count = c0.count();
	fn(dt, udata, c0.data(), c1.data(), c2.data(), c3.data(), c4.data(), c5.data(), c6.data(), c7.data(), count);
}

static inline void s_match(array<int>* matches, const array<strpool_id>& a, const array<strpool_id>& b)
{
	for (int i = 0; i < a.count(); ++i)
	{
		for (int j = 0; j < b.count(); ++j)
		{
			if (a[i].val == b[j].val) {
				matches->add(j);
				break;
			}
		}
	}
}

void ecs_run_systems(float dt)
{
	int system_count = app->systems.count();
	for (int i = 0; i < system_count; ++i)
	{
		system_internal_t* system = app->systems + i;
		void* update_fn = system->update_fn;
		auto pre_update_fn = system->pre_update_fn;
		auto post_update_fn = system->post_update_fn;
		void* udata = system->udata;

		if (pre_update_fn) pre_update_fn(dt, udata);

		if (update_fn) {
			for (int j = 0; j < app->entity_collections.count(); ++j)
			{
				entity_collection_t* collection = app->entity_collections.items() + j;
				CUTE_ASSERT(collection->component_tables.count() == collection->component_type_tuple.count());
				int component_count = collection->component_tables.count();
				app->current_collection_type_being_iterated = app->entity_collections.keys()[j];
				app->current_collection_being_updated = collection;
				CUTE_DEFER(app->current_collection_type_being_iterated = INVALID_ENTITY_TYPE);
				CUTE_DEFER(app->current_collection_being_updated = NULL);

				array<int> matches;
				s_match(&matches, system->component_type_tuple, collection->component_type_tuple);

				array<typeless_array>& tables = collection->component_tables;

				if (matches.count() == system->component_type_tuple.count()) {
					switch (matches.count())
					{
					case 0: s_0(dt, update_fn, udata); break;
					case 1: s_1(dt, update_fn, udata, tables[matches[0]]); break;
					case 2: s_2(dt, update_fn, udata, tables[matches[0]], tables[matches[1]]); break;
					case 3: s_3(dt, update_fn, udata, tables[matches[0]], tables[matches[1]], tables[matches[2]]); break;
					case 4: s_4(dt, update_fn, udata, tables[matches[0]], tables[matches[1]], tables[matches[2]], tables[matches[3]]); break;
					case 5: s_5(dt, update_fn, udata, tables[matches[0]], tables[matches[1]], tables[matches[2]], tables[matches[3]], tables[matches[4]]); break;
					case 6: s_6(dt, update_fn, udata, tables[matches[0]], tables[matches[1]], tables[matches[2]], tables[matches[3]], tables[matches[4]], tables[matches[5]]); break;
					case 7: s_7(dt, update_fn, udata, tables[matches[0]], tables[matches[1]], tables[matches[2]], tables[matches[3]], tables[matches[4]], tables[matches[5]], tables[matches[6]]); break;
					case 8: s_8(dt, update_fn, udata, tables[matches[0]], tables[matches[1]], tables[matches[2]], tables[matches[3]], tables[matches[4]], tables[matches[5]], tables[matches[6]], tables[matches[7]]); break;
					default: CUTE_ASSERT(0);
					}
				}
			}
		}

		if (post_update_fn) post_update_fn(dt, udata);
	}

	for (int i = 0; i < app->delayed_destroy_entities.count(); ++i) {
		entity_t e = app->delayed_destroy_entities[i];
		entity_destroy(e);
	}
	app->delayed_destroy_entities.clear();
}

//--------------------------------------------------------------------------------------------------

void ecs_component_begin()
{
	app->component_config_builder.clear();
}

void ecs_component_end()
{
	app->component_configs.insert(INJECT(app->component_config_builder.name), app->component_config_builder);
}

void ecs_component_set_name(const char* name)
{
	app->component_config_builder.name = name;
}

void ecs_component_set_size(size_t size)
{
	app->component_config_builder.size_of_component = size;
}

void ecs_component_set_optional_serializer(component_serialize_fn* serializer_fn, void* udata)
{
	app->component_config_builder.serializer_fn = serializer_fn;
	app->component_config_builder.serializer_udata = udata;
}

void ecs_component_set_optional_cleanup(component_cleanup_fn* cleanup_fn, void* udata)
{
	app->component_config_builder.cleanup_fn = cleanup_fn;
	app->component_config_builder.cleanup_udata = udata;
}

static strpool_id s_kv_string(kv_t* kv, const char* key)
{
	error_t err = kv_key(kv, key);
	if (err.is_error()) {
		if (CUTE_STRCMP(key, "inherits_from")) {
			CUTE_DEBUG_PRINTF("Unable to find the `%s` key.\n", key);
		}
		return { 0 };
	}

	const char* string_raw;
	size_t string_sz;
	err = kv_val_string(kv, &string_raw, &string_sz);
	if (err.is_error()) {
		CUTE_DEBUG_PRINTF("`%s` key found, but is not a string.\n", key);
		return { 0 };
	}

	return strpool_inject(app->strpool, string_raw, (int)string_sz);
}

static void s_register_entity_type(const char* schema)
{
	// Parse the schema.
	kv_t* kv = kv_make();
	bool cleanup_kv = true;
	CUTE_DEFER(if (cleanup_kv) kv_destroy(kv));

	error_t err = kv_parse(kv, schema, CUTE_STRLEN(schema));
	if (err.is_error()) {
		CUTE_DEBUG_PRINTF("Unable to parse the schema when registering entity type.");
		return;
	}

	strpool_id entity_type_string = s_kv_string(kv, "entity_type");
	if (!strpool_isvalid(app->strpool, entity_type_string)) return;
	
	strpool_id inherits_from_string = s_kv_string(kv, "inherits_from");
	entity_type_t inherits_from = INVALID_ENTITY_TYPE;
	if (strpool_isvalid(app->strpool, inherits_from_string)) {
		app->entity_type_string_to_id.find(inherits_from_string, &inherits_from);
	}

	// Search for all component types present in the schema.
	int component_config_count = app->component_configs.count();
	const component_config_t* component_configs = app->component_configs.items();
	array<strpool_id> component_type_tuple;
	for (int i = 0; i < component_config_count; ++i)
	{
		const component_config_t* config = component_configs + i;

		err = kv_key(kv, config->name);
		if (!err.is_error()) {
			component_type_tuple.add(INJECT(config->name));
		}
	}
	kv_reset_read_state(kv);

	// Register component types.
	entity_type_t entity_type = app->entity_type_gen++;
	app->entity_type_string_to_id.insert(entity_type_string, entity_type);
	app->entity_type_id_to_string.add(entity_type_string);
	entity_collection_t* collection = app->entity_collections.insert(entity_type);
	for (int i = 0; i < component_type_tuple.count(); ++i)
	{
		collection->component_type_tuple.add(component_type_tuple[i]);
		typeless_array& table = collection->component_tables.add();
		component_config_t* config = app->component_configs.find(component_type_tuple[i]);
		table.m_element_size = config->size_of_component;
	}

	// Store the parsed schema.
	app->entity_parsed_schemas.insert(entity_type, kv);
	if (inherits_from != INVALID_ENTITY_TYPE) {
		app->entity_schema_inheritence.insert(entity_type, inherits_from);
	}

	cleanup_kv = false;
}

static void s_register_entity_type(array<const char*> component_type_tuple, const char* entity_type_string)
{
	// Search for all component types present in the schema.
	int component_config_count = app->component_configs.count();
	const component_config_t* component_configs = app->component_configs.items();
	array<strpool_id> component_type_ids;
	for (int i = 0; i < component_config_count; ++i)
	{
		const component_config_t* config = component_configs + i;

		bool found = false;
		for (int i = 0; i < component_type_tuple.count(); ++i) {
			if (!CUTE_STRCMP(component_type_tuple[i], config->name)) {
				found = true;
				break;
			}
		}

		if (found) {
			component_type_ids.add(INJECT(config->name));
		}
	}

	// Register component types.
	strpool_id entity_type_string_id = INJECT(entity_type_string);
	entity_type_t entity_type = app->entity_type_gen++;
	app->entity_type_string_to_id.insert(entity_type_string_id, entity_type);
	app->entity_type_id_to_string.add(entity_type_string_id);
	entity_collection_t* collection = app->entity_collections.insert(entity_type);
	for (int i = 0; i < component_type_ids.count(); ++i)
	{
		collection->component_type_tuple.add(component_type_ids[i]);
		typeless_array& table = collection->component_tables.add();
		component_config_t* config = app->component_configs.find(component_type_ids[i]);
		table.m_element_size = config->size_of_component;
	}
}


void ecs_entity_begin()
{
	app->entity_config_builder.clear();
}

void ecs_entity_end()
{
	if (app->entity_config_builder.schema.is_valid()) {
		s_register_entity_type(app->entity_config_builder.schema.c_str());
	} else {
		s_register_entity_type(app->entity_config_builder.component_types, app->entity_config_builder.entity_type);
	}
}

void ecs_entity_set_name(const char* entity_type)
{
	app->entity_config_builder.entity_type = entity_type;
}

void ecs_entity_add_component(const char* component_type)
{
	app->entity_config_builder.component_types.add(component_type);
}

void ecs_entity_set_optional_schema(const char* schema)
{
	app->entity_config_builder.schema = schema;
}

const char* entity_get_type_string(entity_t entity)
{
	entity_type_t entity_type = s_entity_type(entity);
	return strpool_cstr(app->strpool, app->entity_type_id_to_string[entity_type]);
}

bool entity_is_type(entity_t entity, const char* entity_type_name)
{
	if (!entity_is_valid(entity)) return false;
	const char* type_string = entity_get_type_string(entity);
	return !CUTE_STRCMP(type_string, entity_type_name);
}

entity_type_t s_entity_type(kv_t* kv)
{
	strpool_id entity_type_string = s_kv_string(kv, "entity_type");
	if (!strpool_isvalid(app->strpool, entity_type_string)) return INVALID_ENTITY_TYPE;
	entity_type_t entity_type = INVALID_ENTITY_TYPE;
	app->entity_type_string_to_id.find(entity_type_string, &entity_type);
	return entity_type;
}

static error_t s_fill_load_id_table(kv_t* kv)
{
	int entity_count;
	error_t err = kv_array_begin(kv, &entity_count, "entities");
	if (err.is_error()) {
		return error_failure("Unable to find `entities` array in kv file.");
	}

	while (entity_count--)
	{
		kv_object_begin(kv);

		entity_type_t entity_type = s_entity_type(kv);
		if (entity_type == INVALID_ENTITY_TYPE) {
			return error_failure("Unable to find entity type.");
		}

		entity_collection_t* collection = app->entity_collections.find(entity_type);
		CUTE_ASSERT(collection);

		int index = collection->entity_handles.count();
		handle_t h = collection->entity_handle_table.alloc_handle(index, entity_type);
		collection->entity_handles.add(h);

		entity_t entity;
		entity.handle = h;
		app->load_id_table->add(entity);

		kv_object_end(kv);
	}

	kv_array_end(kv);

	return error_success();
}

error_t ecs_load_entities(kv_t* kv, array<entity_t>* entities_out)
{
	if (kv_get_state(kv) != KV_STATE_READ) {
		return error_failure("`kv` must be in `KV_STATE_READ` mode.");
	}
	
	array<entity_t> load_id_table;
	app->load_id_table = &load_id_table;
	CUTE_DEFER(app->load_id_table = NULL);

	error_t err = s_fill_load_id_table(kv);
	if (err.is_error()) return err;

	int entity_count;
	err = kv_array_begin(kv, &entity_count, "entities");
	if (err.is_error()) {
		return error_failure("Unable to find `entities` array in kv file.");
	}

	int entity_index = 0;
	while (entity_count--)
	{
		entity_t entity = load_id_table[entity_index++];
		kv_object_begin(kv);

		entity_type_t entity_type = s_entity_type(kv);
		if (entity_type == INVALID_ENTITY_TYPE) {
			return error_failure("Unable to find entity type.");
		}

		entity_collection_t* collection = app->entity_collections.find(entity_type);
		CUTE_ASSERT(collection);

		const array<strpool_id>& component_type_tuple = collection->component_type_tuple;
		for (int i = 0; i < component_type_tuple.count(); ++i)
		{
			strpool_id component_type = component_type_tuple[i];
			component_config_t* config = app->component_configs.find(component_type);

			if (!config) {
				return error_failure("Unable to find component config.");
			}

			// First load values from the schema.
			void* component = collection->component_tables[i].add();
			err = s_load_from_schema(entity_type, entity, config, component, config->serializer_udata);
			if (err.is_error()) {
				return error_failure("Unable to parse component from schema.");
			}

			// Then load values from the instance.
			error_t err = kv_object_begin(kv, config->name);
			if (!err.is_error()) {
				err = config->serializer_fn(kv, true, entity, component, config->serializer_udata);
				kv_object_end(kv);
				if (err.is_error()) {
					return error_failure("Unable to parse component.");
				}
			}
		}

		kv_object_end(kv);
	}

	kv_array_end(kv);

	if (entities_out) {
		entities_out->steal_from(&load_id_table);
	}

	return error_success();
}

error_t ecs_save_entities(const array<entity_t>& entities, kv_t* kv)
{
	if (kv_get_state(kv) != KV_STATE_WRITE) {
		return error_failure("`kv` must be in `KV_STATE_WRITE` mode.");
	}

	dictionary<entity_t, int> id_table;
	for (int i = 0; i < entities.count(); ++i)
		id_table.insert(entities[i], i);

	app->save_id_table = &id_table;
	CUTE_DEFER(app->save_id_table = NULL);

	int entity_count = entities.count();
	error_t err = kv_array_begin(kv, &entity_count, "entities");
	if (err.is_error()) return err;

	for (int i = 0; i < entities.count(); ++i)
	{
		entity_t entity = entities[i];
		entity_type_t entity_type = s_entity_type(entity);
		entity_collection_t* collection = app->entity_collections.find(entity_type);
		if (!collection) {
			return error_failure("Unable to find entity type.");
		}

		bool is_valid = collection->entity_handle_table.is_valid(entity.handle);
		if (!is_valid) {
			return error_failure("Attempted to save an invalid entity.");
		}
		uint32_t index = collection->entity_handle_table.get_index(entity.handle);

		kv_object_begin(kv);

		kv_key(kv, "entity_type");
		const char* entity_type_string = strpool_cstr(app->strpool, app->entity_type_id_to_string[entity_type]);
		size_t entity_type_string_len = CUTE_STRLEN(entity_type_string);
		kv_val_string(kv, &entity_type_string, &entity_type_string_len);

		const array<strpool_id>& component_type_tuple = collection->component_type_tuple;
		const array<typeless_array>& component_tables = collection->component_tables;
		for (int j = 0; j < component_type_tuple.count(); ++j)
		{
			strpool_id component_type = component_type_tuple[j];
			const typeless_array& component_table = component_tables[j];
			component_config_t* config = app->component_configs.find(component_type);
			const void* component = component_table[index];

			error_t err = kv_object_begin(kv, config->name);
			if (!err.is_error()) {
				err = config->serializer_fn(kv, false, entity, (void*)component, config->serializer_udata);
				kv_object_end(kv);
				if (err.is_error()) {
					return error_failure("Unable to save component.");
				}
			}
		}

		kv_object_end(kv);
	}

	kv_array_end(kv);

	return error_success();
}

error_t ecs_save_entities(const array<entity_t>& entities)
{
	dictionary<entity_t, int> id_table;
	for (int i = 0; i < entities.count(); ++i)
		id_table.insert(entities[i], i);

	app->save_id_table = &id_table;
	CUTE_DEFER(app->save_id_table = NULL);

	int entity_count = entities.count();
	for (int i = 0; i < entities.count(); ++i)
	{
		entity_t entity = entities[i];
		entity_type_t entity_type = s_entity_type(entity);
		entity_collection_t* collection = app->entity_collections.find(entity_type);
		if (!collection) {
			return error_failure("Unable to find entity type.");
		}

		bool is_valid = collection->entity_handle_table.is_valid(entity.handle);
		if (!is_valid) {
			return error_failure("Attempted to save an invalid entity.");
		}
		uint32_t index = collection->entity_handle_table.get_index(entity.handle);

		const char* entity_type_string = strpool_cstr(app->strpool, app->entity_type_id_to_string[entity_type]);

		const array<strpool_id>& component_type_tuple = collection->component_type_tuple;
		const array<typeless_array>& component_tables = collection->component_tables;
		for (int j = 0; j < component_type_tuple.count(); ++j)
		{
			strpool_id component_type = component_type_tuple[j];
			const typeless_array& component_table = component_tables[j];
			component_config_t* config = app->component_configs.find(component_type);
			const void* component = component_table[index];

			error_t err = config->serializer_fn(NULL, false, entity, (void*)component, config->serializer_udata);
			if (err.is_error()) {
				return error_failure("Unable to save component.");
			}
		}
	}

	return error_success();
}

bool ecs_is_entity_type_valid(const char* entity_type)
{
	if (app->entity_type_string_to_id.find(INJECT(entity_type))) {
		return true;
	} else {
		return false;
	}
}

array<const char*> ecs_get_entity_list()
{
	array<const char*> names;

	for (int i = 0; i < app->entity_type_id_to_string.count(); ++i) {
		strpool_id id = app->entity_type_id_to_string[i];
		const char* name = strpool_cstr(app->strpool, id);
		names.add(name);
	}

	return names;
}

array<const char*> ecs_get_component_list()
{
	array<const char*> names;
	int count = app->component_configs.count();
	strpool_id* ids = app->component_configs.keys();

	for (int i = 0; i < count; ++i) {
		strpool_id id = ids[i];
		const char* name = strpool_cstr(app->strpool, id);
		names.add(name);
	}

	return names;
}

array<const char*> ecs_get_system_list()
{
	array<const char*> names;

	for (int i = 0; i < app->systems.count(); ++i) {
		strpool_id id = app->systems[i].name;
		const char* name = id.val != 0 ? strpool_cstr(app->strpool, id) : "System name was not set.";
		names.add(name);
	}

	return names;
}

array<const char*> ecs_get_component_list_for_entity_type(const char* entity_type)
{
	array<const char*> result;

	entity_type_t type = INVALID_ENTITY_TYPE;
	app->entity_type_string_to_id.find(INJECT(entity_type), &type);
	if (type == INVALID_ENTITY_TYPE) {
		return result;
	}

	entity_collection_t* collection = app->entity_collections.find(type);
	CUTE_ASSERT(collection);

	const array<strpool_id>& component_type_tuple = collection->component_type_tuple;
	for (int i = 0; i < component_type_tuple.count(); ++i)
	{
		strpool_id component_type = component_type_tuple[i];
		component_config_t* config = app->component_configs.find(component_type);
		CUTE_ASSERT(config);
		result.add(config->name);
	}

	return result;
}

}
