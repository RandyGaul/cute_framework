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

#define INJECT(s) cf_strpool_inject_len(cf_app->strpool, s, (int)CUTE_STRLEN(s))

struct cf_ecs_arrays_t
{
	int count;
	cf_handle_t* entities;
	cf_strpool_id* types;
	cf_array<cf_typeless_array>* ptrs;

	void* find_components(const char* type)
	{
		cf_strpool_id id = INJECT(type);
		for (int i = 0; i < count; ++i) {
			if (types[i].val == id.val) {
				return (*ptrs)[i].data();
			}
		}
		return NULL;
	}
};

static cf_ecs_arrays_t s_arrays;

static cf_error_t cf_s_load_from_schema(cf_entity_type_t schema_type, cf_entity_t entity, cf_component_config_t* config, void* component, void* udata)
{
	// Look for parent.
	// If parent exists, load values from it first.
	cf_entity_type_t inherits_from = CF_INVALID_ENTITY_TYPE;
	cf_error_t err = cf_app->entity_schema_inheritence.find(schema_type, &inherits_from);
	if (!cf_is_error(err)) {
		err = cf_s_load_from_schema(inherits_from, entity, config, component, udata);
		if (cf_is_error(err)) return err;
	}

	cf_kv_t* schema = NULL;
	err = cf_app->entity_parsed_schemas.find(schema_type, &schema);
	if (cf_is_error(err)) {
		err = cf_error_success();
		if (config->serializer_fn) err = config->serializer_fn(schema, true, entity, component, udata);
	} else {
		err = cf_kv_object_begin(schema, config->name);
		if (!cf_is_error(err)) {
			if (config->serializer_fn) err = config->serializer_fn(schema, true, entity, component, udata);
			if (cf_is_error(err)) return err;
			err = cf_kv_object_end(schema);
		}
	}

	return err;
}

//--------------------------------------------------------------------------------------------------

void* cf_ecs_arrays_find_components(cf_ecs_arrays_t* arrays, const char* component_type)
{
	return arrays->find_components(component_type);
}

cf_entity_t* cf_ecs_arrays_get_entities(cf_ecs_arrays_t* arrays)
{
	return (cf_entity_t*)arrays->entities;
}

void cf_ecs_system_begin()
{
	cf_app->system_internal_builder.clear();
}

void cf_ecs_system_end()
{
	cf_app->systems.add(cf_app->system_internal_builder);
}

void cf_ecs_system_set_name(const char* name)
{
	cf_app->system_internal_builder.name = INJECT(name);
}

void cf_ecs_system_set_update(cf_system_update_fn* update_fn)
{
	cf_app->system_internal_builder.update_fn = update_fn;
}

void cf_ecs_system_require_component(const char* component_type)
{
	cf_app->system_internal_builder.component_type_tuple.add(INJECT(component_type));
}

void cf_ecs_system_set_optional_pre_update(void (*pre_update_fn)(float dt, void* udata))
{
	cf_app->system_internal_builder.pre_update_fn = pre_update_fn;
}

void cf_ecs_system_set_optional_post_update(void (*post_update_fn)(float dt, void* udata))
{
	cf_app->system_internal_builder.post_update_fn = post_update_fn;
}

void cf_ecs_system_set_optional_update_udata(void* udata)
{
	cf_app->system_internal_builder.udata = udata;
}

static CUTE_INLINE uint16_t cf_s_entity_type(cf_entity_t entity)
{
	return (uint16_t)((entity.handle & 0x00000000FFFF0000ULL) >> 16);
}

cf_entity_t cf_entity_make(const char* entity_type, cf_error_t* err_out)
{
	cf_entity_type_t type = CF_INVALID_ENTITY_TYPE;
	cf_app->entity_type_string_to_id.find(INJECT(entity_type), &type);
	if (type == CF_INVALID_ENTITY_TYPE) {
		if (err_out) *err_out = cf_error_failure("`entity_type` is not valid.");
		return CF_INVALID_ENTITY;
	}

	cf_entity_collection_t* collection = cf_app->entity_collections.find(type);
	CUTE_ASSERT(collection);

	int index = collection->entity_handles.count();
	cf_handle_t h = collection->entity_handle_table.alloc_handle(index, type);
	collection->entity_handles.add(h);
	cf_entity_t entity = { h };

	const cf_array<cf_strpool_id>& component_type_tuple = collection->component_type_tuple;
	for (int i = 0; i < component_type_tuple.count(); ++i) {
		cf_strpool_id component_type = component_type_tuple[i];
		cf_component_config_t* config = cf_app->component_configs.find(component_type);

		if (!config) {
			if (err_out) *err_out = cf_error_failure("Unable to find component config.");
			return CF_INVALID_ENTITY;
		}

		void* component = collection->component_tables[i].add();
		cf_error_t err = cf_s_load_from_schema(type, entity, config, component, config->serializer_udata);
		if (cf_is_error(err)) {
			// TODO - Unload the components that were added with `.add()` a couple lines above here.
			return CF_INVALID_ENTITY;
		}
	}

	if (err_out) *err_out = cf_error_success();
	return entity;
}

static cf_entity_collection_t* cf_s_collection(cf_entity_t entity)
{
	cf_entity_collection_t* collection = NULL;
	uint16_t entity_type = cf_s_entity_type(entity);
	if (entity_type == cf_app->current_collection_type_being_iterated) {
		// Fast path -- check the current entity collection for this entity type first.
		collection = cf_app->current_collection_being_updated;
		CUTE_ASSERT(collection);
	} else {
		// Slightly slower path -- lookup collection first.
		collection = cf_app->entity_collections.find(entity_type);
		if (!collection) return NULL;
	}
	return collection;
}

void cf_entity_destroy(cf_entity_t entity)
{
	uint16_t entity_type = cf_s_entity_type(entity);
	cf_entity_collection_t* collection = cf_app->entity_collections.find(entity_type);
	CUTE_ASSERT(collection);

	if (collection->entity_handle_table.is_valid(entity.handle)) {
		int index = collection->entity_handle_table.get_index(entity.handle);

		// Call cleanup function on each component.
		for (int i = 0; i < collection->component_tables.count(); ++i) {
			cf_component_config_t config;
			cf_app->component_configs.find(collection->component_type_tuple[i], &config);
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

void cf_entity_delayed_destroy(cf_entity_t entity)
{
	cf_app->delayed_destroy_entities.add(entity);
}

void cf_entity_delayed_deactivate(cf_entity_t entity)
{
	cf_app->delayed_deactivate_entities.add(entity);
}

void cf_entity_delayed_activate(cf_entity_t entity)
{
	cf_app->delayed_activate_entities.add(entity);
}

void cf_entity_deactivate(cf_entity_t entity)
{
	uint16_t entity_type = cf_s_entity_type(entity);
	cf_entity_collection_t* collection = cf_app->entity_collections.find(entity_type);
	CUTE_ASSERT(collection);

	if (collection->entity_handle_table.is_valid(entity.handle)) {
		if (!collection->entity_handle_table.is_active(entity.handle)) {
			return;
		}

		int index = collection->entity_handle_table.get_index(entity.handle);

		int copy_index = -1; // TODO.

		// Swap all components into the deactive section (the end) of their respective arrays.
		for (int i = 0; i < collection->component_tables.count(); ++i) {
			collection->component_tables[i].copy(index, copy_index); // WORKING HERE
		}

		// Update handle of the swapped entity.
		if (index < collection->entity_handles.size()) {
			uint64_t h = collection->entity_handles[index];
			collection->entity_handle_table.update_index(h, index);
		}
	}
}

void cf_entity_activate(cf_entity_t entity)
{
}

bool cf_entity_is_active(cf_entity_t entity)
{
	return false;
}

bool cf_entity_is_valid(cf_entity_t entity)
{
	cf_entity_collection_t* collection = cf_s_collection(entity);
	if (collection) return collection->entity_handle_table.is_valid(entity.handle);
	else return false;
}

void* cf_entity_get_component(cf_entity_t entity, const char* component_type)
{
	cf_entity_collection_t* collection = cf_s_collection(entity);
	if (!collection) return NULL;

	cf_strpool_id type = INJECT(component_type);
	const cf_array<cf_strpool_id>& component_type_tuple = collection->component_type_tuple;
	for (int i = 0; i < component_type_tuple.count(); ++i) {
		if (component_type_tuple[i].val == type.val) {
			int index = collection->entity_handle_table.get_index(entity.handle);
			return collection->component_tables[i][index];
		}
	}

	return NULL;
}

bool cf_entity_has_component(cf_entity_t entity, const char* component_type)
{
	return cf_entity_get_component(entity, component_type) ? true : false;
}

//--------------------------------------------------------------------------------------------------

static inline int s_match(const cf_array<cf_strpool_id>& a, const cf_array<cf_strpool_id>& b)
{
	int matches = 0;
	for (int i = 0; i < a.count(); ++i) {
		for (int j = 0; j < b.count(); ++j) {
			if (a[i].val == b[j].val) {
				++matches;
				break;
			}
		}
	}
	return matches;
}

void cf_ecs_run_systems(float dt)
{
	int system_count = cf_app->systems.count();
	for (int i = 0; i < system_count; ++i) {
		cf_system_internal_t* system = cf_app->systems + i;
		cf_system_update_fn* update_fn = system->update_fn;
		auto pre_update_fn = system->pre_update_fn;
		auto post_update_fn = system->post_update_fn;
		void* udata = system->udata;

		if (pre_update_fn) pre_update_fn(dt, udata);

		if (update_fn) {
			for (int j = 0; j < cf_app->entity_collections.count(); ++j) {
				cf_entity_collection_t* collection = cf_app->entity_collections.items() + j;
				CUTE_ASSERT(collection->component_tables.count() == collection->component_type_tuple.count());
				int component_count = collection->component_tables.count();
				cf_app->current_collection_type_being_iterated = cf_app->entity_collections.keys()[j];
				cf_app->current_collection_being_updated = collection;
				CUTE_DEFER(cf_app->current_collection_type_being_iterated = CF_INVALID_ENTITY_TYPE);
				CUTE_DEFER(cf_app->current_collection_being_updated = NULL);

				int matches = s_match(system->component_type_tuple, collection->component_type_tuple);

				if (matches == system->component_type_tuple.count()) {
					s_arrays.count = collection->component_type_tuple.count();
					s_arrays.ptrs = &collection->component_tables;
					s_arrays.types = collection->component_type_tuple.data();
					s_arrays.entities = collection->entity_handles.data();
					update_fn(dt, &s_arrays, collection->component_tables[0].count(), udata);
				}
			}
		}

		if (post_update_fn) post_update_fn(dt, udata);
	}

	for (int i = 0; i < cf_app->delayed_destroy_entities.count(); ++i) {
		cf_entity_t e = cf_app->delayed_destroy_entities[i];
		cf_entity_destroy(e);
	}
	cf_app->delayed_destroy_entities.clear();
}

//--------------------------------------------------------------------------------------------------

void cf_ecs_component_begin()
{
	cf_app->component_config_builder.clear();
}

void cf_ecs_component_end()
{
	cf_app->component_configs.insert(INJECT(cf_app->component_config_builder.name), cf_app->component_config_builder);
}

void cf_ecs_component_set_name(const char* name)
{
	cf_app->component_config_builder.name = name;
}

void cf_ecs_component_set_size(size_t size)
{
	cf_app->component_config_builder.size_of_component = size;
}

void cf_ecs_component_set_optional_serializer(cf_component_serialize_fn* serializer_fn, void* udata)
{
	cf_app->component_config_builder.serializer_fn = serializer_fn;
	cf_app->component_config_builder.serializer_udata = udata;
}

void cf_ecs_component_set_optional_cleanup(cf_component_cleanup_fn* cleanup_fn, void* udata)
{
	cf_app->component_config_builder.cleanup_fn = cleanup_fn;
	cf_app->component_config_builder.cleanup_udata = udata;
}

static cf_strpool_id cf_s_kv_string(cf_kv_t* kv, const char* key)
{
	cf_error_t err = cf_kv_key(kv, key, NULL);
	if (cf_is_error(err)) {
		if (CUTE_STRCMP(key, "inherits_from")) {
			CUTE_DEBUG_PRINTF("Unable to find the `%s` key.\n", key);
		}
		return { 0 };
	}

	const char* string_raw;
	size_t string_sz;
	err = cf_kv_val_string(kv, &string_raw, &string_sz);
	if (cf_is_error(err)) {
		CUTE_DEBUG_PRINTF("`%s` key found, but is not a string.\n", key);
		return { 0 };
	}

	return cf_strpool_inject_len(cf_app->strpool, string_raw, (int)string_sz);
}

static void cf_s_register_entity_type(const char* schema)
{
	// Parse the schema.
	cf_kv_t* kv = cf_kv_make(NULL);
	bool cleanup_kv = true;
	CUTE_DEFER(if (cleanup_kv) cf_kv_destroy(kv));

	cf_error_t err = cf_kv_parse(kv, schema, CUTE_STRLEN(schema));
	if (cf_is_error(err)) {
		CUTE_DEBUG_PRINTF("Unable to parse the schema when registering entity type.");
		return;
	}

	cf_strpool_id entity_type_string = cf_s_kv_string(kv, "entity_type");
	if (!cf_strpool_isvalid(cf_app->strpool, entity_type_string)) return;

	cf_strpool_id inherits_from_string = cf_s_kv_string(kv, "inherits_from");
	cf_entity_type_t inherits_from = CF_INVALID_ENTITY_TYPE;
	if (cf_strpool_isvalid(cf_app->strpool, inherits_from_string)) {
		cf_app->entity_type_string_to_id.find(inherits_from_string, &inherits_from);
	}

	// Search for all component types present in the schema.
	int component_config_count = cf_app->component_configs.count();
	const cf_component_config_t* component_configs = cf_app->component_configs.items();
	cf_array<cf_strpool_id> component_type_tuple;
	for (int i = 0; i < component_config_count; ++i) {
		const cf_component_config_t* config = component_configs + i;

		err = cf_kv_key(kv, config->name, NULL);
		if (!cf_is_error(err)) {
			component_type_tuple.add(INJECT(config->name));
		}
	}
	cf_kv_reset_read_state(kv);

	// Register component types.
	cf_entity_type_t entity_type = cf_app->entity_type_gen++;
	cf_app->entity_type_string_to_id.insert(entity_type_string, entity_type);
	cf_app->entity_type_id_to_string.add(entity_type_string);
	cf_entity_collection_t* collection = cf_app->entity_collections.insert(entity_type);
	for (int i = 0; i < component_type_tuple.count(); ++i) {
		collection->component_type_tuple.add(component_type_tuple[i]);
		cf_typeless_array& table = collection->component_tables.add();
		cf_component_config_t* config = cf_app->component_configs.find(component_type_tuple[i]);
		table.m_element_size = config->size_of_component;
	}

	// Store the parsed schema.
	cf_app->entity_parsed_schemas.insert(entity_type, kv);
	if (inherits_from != CF_INVALID_ENTITY_TYPE) {
		cf_app->entity_schema_inheritence.insert(entity_type, inherits_from);
	}

	cleanup_kv = false;
}

static void cf_s_register_entity_type(cf_array<const char*> component_type_tuple, const char* entity_type_string)
{
	// Search for all component types present in the schema.
	int component_config_count = cf_app->component_configs.count();
	const cf_component_config_t* component_configs = cf_app->component_configs.items();
	cf_array<cf_strpool_id> component_type_ids;
	for (int i = 0; i < component_config_count; ++i) {
		const cf_component_config_t* config = component_configs + i;

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
	cf_strpool_id entity_type_string_id = INJECT(entity_type_string);
	cf_entity_type_t entity_type = cf_app->entity_type_gen++;
	cf_app->entity_type_string_to_id.insert(entity_type_string_id, entity_type);
	cf_app->entity_type_id_to_string.add(entity_type_string_id);
	cf_entity_collection_t* collection = cf_app->entity_collections.insert(entity_type);
	for (int i = 0; i < component_type_ids.count(); ++i) {
		collection->component_type_tuple.add(component_type_ids[i]);
		cf_typeless_array& table = collection->component_tables.add();
		cf_component_config_t* config = cf_app->component_configs.find(component_type_ids[i]);
		table.m_element_size = config->size_of_component;
	}
}


void cf_ecs_entity_begin()
{
	cf_app->entity_config_builder.clear();
}

void cf_ecs_entity_end()
{
	if (cf_app->entity_config_builder.schema.is_valid()) {
		cf_s_register_entity_type(cf_app->entity_config_builder.schema.c_str());
	} else {
		cf_s_register_entity_type(cf_app->entity_config_builder.component_types, cf_app->entity_config_builder.entity_type);
	}
}

void cf_ecs_entity_set_name(const char* entity_type)
{
	cf_app->entity_config_builder.entity_type = entity_type;
}

void cf_ecs_entity_add_component(const char* component_type)
{
	cf_app->entity_config_builder.component_types.add(component_type);
}

void cf_ecs_entity_set_optional_schema(const char* schema)
{
	cf_app->entity_config_builder.schema = schema;
}

const char* cf_entity_get_type_string(cf_entity_t entity)
{
	cf_entity_type_t entity_type = cf_s_entity_type(entity);
	return cf_strpool_cstr(cf_app->strpool, cf_app->entity_type_id_to_string[entity_type]);
}

bool cf_entity_is_type(cf_entity_t entity, const char* entity_type_name)
{
	if (!cf_entity_is_valid(entity)) return false;
	const char* type_string = cf_entity_get_type_string(entity);
	return !CUTE_STRCMP(type_string, entity_type_name);
}

cf_entity_type_t cf_s_entity_type(cf_kv_t* kv)
{
	cf_strpool_id entity_type_string = cf_s_kv_string(kv, "entity_type");
	if (!cf_strpool_isvalid(cf_app->strpool, entity_type_string)) return CF_INVALID_ENTITY_TYPE;
	cf_entity_type_t entity_type = CF_INVALID_ENTITY_TYPE;
	cf_app->entity_type_string_to_id.find(entity_type_string, &entity_type);
	return entity_type;
}

static cf_error_t cf_s_fill_load_id_table(cf_kv_t* kv)
{
	int entity_count;
	cf_error_t err = cf_kv_array_begin(kv, &entity_count, "entities");
	if (cf_is_error(err)) {
		return cf_error_failure("Unable to find `entities` array in kv file.");
	}

	while (entity_count--) {
		cf_kv_object_begin(kv, NULL);

		cf_entity_type_t entity_type = cf_s_entity_type(kv);
		if (entity_type == CF_INVALID_ENTITY_TYPE) {
			return cf_error_failure("Unable to find entity type.");
		}

		cf_entity_collection_t* collection = cf_app->entity_collections.find(entity_type);
		CUTE_ASSERT(collection);

		int index = collection->entity_handles.count();
		cf_handle_t h = collection->entity_handle_table.alloc_handle(index, entity_type);
		collection->entity_handles.add(h);

		cf_entity_t entity;
		entity.handle = h;
		cf_app->load_id_table->add(entity);

		cf_kv_object_end(kv);
	}

	cf_kv_array_end(kv);

	return cf_error_success();
}



cf_error_t cf_internal_ecs_load_entities(cf_kv_t* kv, cf_array<cf_entity_t>* entities_out)
{
	if (cf_kv_get_state(kv) != CF_KV_STATE_READ) {
		return cf_error_failure("`kv` must be in `KV_STATE_READ` mode.");
	}

	cf_array<cf_entity_t> load_id_table;
	cf_app->load_id_table = &load_id_table;
	CUTE_DEFER(cf_app->load_id_table = NULL);

	cf_error_t err = cf_s_fill_load_id_table(kv);
	if (cf_is_error(err)) return err;

	int entity_count;
	err = cf_kv_array_begin(kv, &entity_count, "entities");
	if (cf_is_error(err)) {
		return cf_error_failure("Unable to find `entities` array in kv file.");
	}

	int entity_index = 0;
	while (entity_count--) {
		cf_entity_t entity = load_id_table[entity_index++];
		cf_kv_object_begin(kv, NULL);

		cf_entity_type_t entity_type = cf_s_entity_type(kv);
		if (entity_type == CF_INVALID_ENTITY_TYPE) {
			return cf_error_failure("Unable to find entity type.");
		}

		cf_entity_collection_t* collection = cf_app->entity_collections.find(entity_type);
		CUTE_ASSERT(collection);

		const cf_array<cf_strpool_id>& component_type_tuple = collection->component_type_tuple;
		for (int i = 0; i < component_type_tuple.count(); ++i) {
			cf_strpool_id component_type = component_type_tuple[i];
			cf_component_config_t* config = cf_app->component_configs.find(component_type);

			if (!config) {
				return cf_error_failure("Unable to find component config.");
			}

			// First load values from the schema.
			void* component = collection->component_tables[i].add();
			err = cf_s_load_from_schema(entity_type, entity, config, component, config->serializer_udata);
			if (cf_is_error(err)) {
				return cf_error_failure("Unable to parse component from schema.");
			}

			// Then load values from the instance.
			cf_error_t err = cf_kv_object_begin(kv, config->name);
			if (!cf_is_error(err)) {
				err = config->serializer_fn(kv, true, entity, component, config->serializer_udata);
				cf_kv_object_end(kv);
				if (cf_is_error(err)) {
					return cf_error_failure("Unable to parse component.");
				}
			}
		}

		cf_kv_object_end(kv);
	}

	cf_kv_array_end(kv);

	if (entities_out) {
		entities_out->steal_from(&load_id_table);
	}

	return cf_error_success();
}

cf_error_t cf_ecs_load_entities(cf_kv_t* kv, cf_entity_t** entities_out, int* entities_count_out)
{
	cf_error_t err;

	if (entities_out) {
		CUTE_ASSERT(entities_count_out);

		cf_array<cute::entity_t> arr = {};

		err = cf_internal_ecs_load_entities(kv, &arr);

		if (cf_is_error(err)) {
			*entities_out = NULL;
			*entities_count_out = 0;
		} else {
			*entities_out = arr.data();
			*entities_count_out = arr.count();

			CUTE_MEMSET(&arr, 0, sizeof(arr));
		}

	} else {
		err = cf_internal_ecs_load_entities(kv, NULL);
	}

	return err;
}

void cf_ecs_free_entities(cf_entity_t* entities)
{
	CUTE_FREE(entities, NULL);
}

cf_error_t cf_internal_ecs_save_entities_kv(const cf_array<cf_entity_t>& entities, cf_kv_t* kv)
{
	if (cf_kv_get_state(kv) != CF_KV_STATE_WRITE) {
		return cf_error_failure("`kv` must be in `KV_STATE_WRITE` mode.");
	}

	cf_dictionary<cf_entity_t, int> id_table;
	for (int i = 0; i < entities.count(); ++i)
		id_table.insert(entities[i], i);

	cf_app->save_id_table = &id_table;
	CUTE_DEFER(cf_app->save_id_table = NULL);

	int entity_count = entities.count();
	cf_error_t err = cf_kv_array_begin(kv, &entity_count, "entities");
	if (cf_is_error(err)) return err;

	for (int i = 0; i < entities.count(); ++i) {
		cf_entity_t entity = entities[i];
		cf_entity_type_t entity_type = cf_s_entity_type(entity);
		cf_entity_collection_t* collection = cf_app->entity_collections.find(entity_type);
		if (!collection) {
			return cf_error_failure("Unable to find entity type.");
		}

		bool is_valid = collection->entity_handle_table.is_valid(entity.handle);
		if (!is_valid) {
			return cf_error_failure("Attempted to save an invalid entity.");
		}
		uint32_t index = collection->entity_handle_table.get_index(entity.handle);

		cf_kv_object_begin(kv, NULL);

		cf_kv_key(kv, "entity_type", NULL);
		const char* entity_type_string = cf_strpool_cstr(cf_app->strpool, cf_app->entity_type_id_to_string[entity_type]);
		size_t entity_type_string_len = CUTE_STRLEN(entity_type_string);
		cf_kv_val_string(kv, &entity_type_string, &entity_type_string_len);

		const cf_array<cf_strpool_id>& component_type_tuple = collection->component_type_tuple;
		const cf_array<cf_typeless_array>& component_tables = collection->component_tables;
		for (int j = 0; j < component_type_tuple.count(); ++j) {
			cf_strpool_id component_type = component_type_tuple[j];
			const cf_typeless_array& component_table = component_tables[j];
			cf_component_config_t* config = cf_app->component_configs.find(component_type);
			const void* component = component_table[index];

			cf_error_t err = cf_kv_object_begin(kv, config->name);
			if (!cf_is_error(err)) {
				err = config->serializer_fn(kv, false, entity, (void*)component, config->serializer_udata);
				cf_kv_object_end(kv);
				if (cf_is_error(err)) {
					return cf_error_failure("Unable to save component.");
				}
			}
		}

		cf_kv_object_end(kv);
	}

	cf_kv_array_end(kv);

	return cf_error_success();
}

cf_error_t cf_ecs_save_entities_kv(const cf_entity_t* entities, int entities_count, cf_kv_t* kv)
{
	// this function won't modify the array, so we can safely cast away the const
	cf_array<cf_entity_t> arr((cf_entity_t*)entities, entities_count, entities_count, NULL);

	cf_error_t err = cf_internal_ecs_save_entities_kv(arr, kv);

	// we don't want the array's deconstructor to free the memory we gave it
	CUTE_MEMSET(&arr, 0, sizeof(arr));

	return err;
}

cf_error_t cf_internal_ecs_save_entities(const cf_array<cf_entity_t>& entities)
{
	cf_dictionary<cf_entity_t, int> id_table;
	for (int i = 0; i < entities.count(); ++i)
		id_table.insert(entities[i], i);

	cf_app->save_id_table = &id_table;
	CUTE_DEFER(cf_app->save_id_table = NULL);

	int entity_count = entities.count();
	for (int i = 0; i < entities.count(); ++i) {
		cf_entity_t entity = entities[i];
		cf_entity_type_t entity_type = cf_s_entity_type(entity);
		cf_entity_collection_t* collection = cf_app->entity_collections.find(entity_type);
		if (!collection) {
			return cf_error_failure("Unable to find entity type.");
		}

		bool is_valid = collection->entity_handle_table.is_valid(entity.handle);
		if (!is_valid) {
			return cf_error_failure("Attempted to save an invalid entity.");
		}
		uint32_t index = collection->entity_handle_table.get_index(entity.handle);

		const char* entity_type_string = cf_strpool_cstr(cf_app->strpool, cf_app->entity_type_id_to_string[entity_type]);

		const cf_array<cf_strpool_id>& component_type_tuple = collection->component_type_tuple;
		const cf_array<cf_typeless_array>& component_tables = collection->component_tables;
		for (int j = 0; j < component_type_tuple.count(); ++j) {
			cf_strpool_id component_type = component_type_tuple[j];
			const cf_typeless_array& component_table = component_tables[j];
			cf_component_config_t* config = cf_app->component_configs.find(component_type);
			const void* component = component_table[index];

			cf_error_t err = config->serializer_fn(NULL, false, entity, (void*)component, config->serializer_udata);
			if (cf_is_error(err)) {
				return cf_error_failure("Unable to save component.");
			}
		}
	}

	return cf_error_success();
}

cf_error_t cf_ecs_save_entities(const cf_entity_t* entities, int entities_count)
{
	// this function won't modify the array, so we can safely cast away the const
	cf_array<cf_entity_t> arr((cf_entity_t*)entities, entities_count, entities_count, NULL);

	cf_error_t err = cf_internal_ecs_save_entities(arr);

	// we don't want the array's deconstructor to free the memory we gave it
	CUTE_MEMSET(&arr, 0, sizeof(arr));

	return err;
}

bool cf_ecs_is_entity_type_valid(const char* entity_type)
{
	if (cf_app->entity_type_string_to_id.find(INJECT(entity_type))) {
		return true;
	} else {
		return false;
	}
}

cf_array<const char*> cf_internal_ecs_get_entity_list()
{
	cf_array<const char*> names;

	for (int i = 0; i < cf_app->entity_type_id_to_string.count(); ++i) {
		cf_strpool_id id = cf_app->entity_type_id_to_string[i];
		const char* name = cf_strpool_cstr(cf_app->strpool, id);
		names.add(name);
	}

	return names;
}

const char** cf_ecs_get_entity_list(int* entities_count_out)
{
	cf_array<const char*> arr = cf_internal_ecs_get_entity_list();

	const char** entities_out = arr.data();
	if (entities_count_out) {
		*entities_count_out = arr.count();
	}

	CUTE_MEMSET(&arr, 0, sizeof(arr));

	return entities_out;
}

cf_array<const char*> cf_internal_ecs_get_component_list()
{
	cf_array<const char*> names;
	int count = cf_app->component_configs.count();
	cf_strpool_id* ids = cf_app->component_configs.keys();

	for (int i = 0; i < count; ++i) {
		cf_strpool_id id = ids[i];
		const char* name = cf_strpool_cstr(cf_app->strpool, id);
		names.add(name);
	}

	return names;
}

const char** cf_ecs_get_component_list(int* components_count_out)
{
	cf_array<const char*> arr = cf_internal_ecs_get_component_list();

	const char** components_out = arr.data();
	if (components_count_out) {
		*components_count_out = arr.count();
	}

	CUTE_MEMSET(&arr, 0, sizeof(arr));

	return components_out;
}

cf_array<const char*> cf_internal_ecs_get_system_list()
{
	cf_array<const char*> names;

	for (int i = 0; i < cf_app->systems.count(); ++i) {
		cf_strpool_id id = cf_app->systems[i].name;
		const char* name = id.val != 0 ? cf_strpool_cstr(cf_app->strpool, id) : "System name was not set.";
		names.add(name);
	}

	return names;
}

const char** cf_ecs_get_system_list(int* systems_count_out)
{
	cf_array<const char*> arr = cf_internal_ecs_get_system_list();

	const char** systems_out = arr.data();
	if (systems_count_out) {
		*systems_count_out = arr.count();
	}

	CUTE_MEMSET(&arr, 0, sizeof(arr));

	return systems_out;
}

cf_array<const char*> cf_internal_ecs_get_component_list_for_entity_type(const char* entity_type)
{
	cf_array<const char*> result;

	cf_entity_type_t type = CF_INVALID_ENTITY_TYPE;
	cf_app->entity_type_string_to_id.find(INJECT(entity_type), &type);
	if (type == CF_INVALID_ENTITY_TYPE) {
		return result;
	}

	cf_entity_collection_t* collection = cf_app->entity_collections.find(type);
	CUTE_ASSERT(collection);

	const cf_array<cf_strpool_id>& component_type_tuple = collection->component_type_tuple;
	for (int i = 0; i < component_type_tuple.count(); ++i) {
		cf_strpool_id component_type = component_type_tuple[i];
		cf_component_config_t* config = cf_app->component_configs.find(component_type);
		CUTE_ASSERT(config);
		result.add(config->name);
	}

	return result;
}

const char** cf_ecs_get_component_list_for_entity_type(const char* entity_type, int* components_count_out)
{
	cf_array<const char*> arr = cf_internal_ecs_get_component_list_for_entity_type(entity_type);

	const char** components_out = arr.data();
	if (components_count_out) {
		*components_count_out = arr.count();
	}

	CUTE_MEMSET(&arr, 0, sizeof(arr));

	return components_out;
}

void cf_ecs_free_list(const char** list)
{
	CUTE_FREE(list, NULL);
}

namespace cute
{
error_t ecs_load_entities(kv_t* kv, array<entity_t>* entities_out) { return cf_internal_ecs_load_entities(kv, entities_out); }

error_t ecs_save_entities(const array<entity_t>& entities, kv_t* kv) { return cf_internal_ecs_save_entities_kv(entities, kv); }
error_t ecs_save_entities(const array<entity_t>& entities) { return cf_internal_ecs_save_entities(entities); }

array<const char*> ecs_get_entity_list() { return cf_internal_ecs_get_entity_list(); }
array<const char*> ecs_get_component_list() { return cf_internal_ecs_get_component_list(); }
array<const char*> ecs_get_system_list() { return cf_internal_ecs_get_system_list(); }
array<const char*> ecs_get_component_list_for_entity_type(const char* entity_type) { return cf_internal_ecs_get_component_list_for_entity_type(entity_type); }
}
