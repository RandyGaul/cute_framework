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

#include <cute_ecs.h>
#include <cute_c_runtime.h>
#include <cute_kv.h>
#include <cute_defer.h>
#include <cute_string.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_alloc_internal.h>

using namespace Cute;

void* cf_get_components(CF_ComponentList component_list, const char* component_type)
{
	CF_ComponentListInternal* list = (CF_ComponentListInternal*)component_list.id;
	return list->find_components(component_type);
}

CF_Entity* cf_get_entities(CF_ComponentList component_list)
{
	CF_ComponentListInternal* list = (CF_ComponentListInternal*)component_list.id;
	return (CF_Entity*)list->entities;
}

void cf_ecs_system_begin()
{
	app->system_internal_builder.clear();
}

void cf_ecs_system_end()
{
	app->systems.add(app->system_internal_builder);
}

void cf_ecs_system_set_name(const char* name)
{
	app->system_internal_builder.name = sintern(name);
}

void cf_ecs_system_set_update(CF_SystemUpdateFn* update_fn)
{
	app->system_internal_builder.update_fn = update_fn;
}

void cf_ecs_system_require_component(const char* component_type)
{
	app->system_internal_builder.component_type_tuple.add(sintern(component_type));
}

void cf_ecs_system_set_optional_pre_update(void (*pre_update_fn)(float dt, void* udata))
{
	app->system_internal_builder.pre_update_fn = pre_update_fn;
}

void cf_ecs_system_set_optional_post_update(void (*post_update_fn)(float dt, void* udata))
{
	app->system_internal_builder.post_update_fn = post_update_fn;
}

void cf_ecs_system_set_optional_update_udata(void* udata)
{
	app->system_internal_builder.udata = udata;
}

static CF_INLINE uint16_t s_entity_type(CF_Entity entity)
{
	return (uint16_t)((entity.handle & 0x00000000FFFF0000ULL) >> 16);
}

CF_Entity cf_make_entity(const char* entity_type, CF_Result* err_out)
{
	auto type_ptr = app->entity_type_string_to_id.try_find(sintern(entity_type));
	if (!type_ptr) {
		if (err_out) *err_out = cf_result_error("`entity_type` is not valid.");
		return CF_INVALID_ENTITY;
	}
	CF_EntityType type = *type_ptr;

	CF_EntityCollection* collection = app->entity_collections.try_find(type);
	CF_ASSERT(collection);

	int index = collection->entity_handles.count();
	CF_Handle h = collection->entity_handle_table.alloc_handle(index, type);
	collection->entity_handles.add(h);
	CF_Entity entity = { h };

	const Array<const char*>& component_type_tuple = collection->component_type_tuple;
	for (int i = 0; i < component_type_tuple.count(); ++i) {
		const char* component_type = component_type_tuple[i];
		CF_ComponentConfig* config = app->component_configs.try_find(component_type);

		if (!config) {
			if (err_out) *err_out = cf_result_error("Unable to find component config.");
			return CF_INVALID_ENTITY;
		}

		void* component = collection->component_tables[i].add();
		if (config->initializer) {
			config->initializer(entity, component, config->initializer_udata);
		}
	}

	if (err_out) *err_out = cf_result_success();
	return entity;
}

static CF_EntityCollection* s_collection(CF_Entity entity)
{
	CF_EntityCollection* collection = NULL;
	uint16_t entity_type = s_entity_type(entity);
	if (entity_type == app->current_collection_type_being_iterated) {
		// Fast path -- check the current entity collection for this entity type first.
		collection = app->current_collection_being_updated;
		CF_ASSERT(collection);
	} else {
		// Slightly slower path -- lookup collection first.
		collection = app->entity_collections.try_find(entity_type);
		if (!collection) return NULL;
	}
	return collection;
}

void cf_destroy_entity(CF_Entity entity)
{
	uint16_t entity_type = s_entity_type(entity);
	CF_EntityCollection* collection = app->entity_collections.try_find(entity_type);
	CF_ASSERT(collection);

	if (collection->entity_handle_table.valid(entity.handle)) {
		int index = collection->entity_handle_table.get_index(entity.handle);

		// Call cleanup function on each component.
		for (int i = 0; i < collection->component_tables.count(); ++i) {
			CF_ComponentConfig* config = app->component_configs.try_find(collection->component_type_tuple[i]);
			if (config && config->cleanup_fn) {
				config->cleanup_fn(entity, collection->component_tables[i][index], config->cleanup_udata);
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

void cf_destroy_entity_delayed(CF_Entity entity)
{
	app->delayed_destroy_entities.add(entity);
}

void cf_entity_delayed_deactivate(CF_Entity entity)
{
	app->delayed_deactivate_entities.add(entity);
}

void cf_entity_delayed_activate(CF_Entity entity)
{
	app->delayed_activate_entities.add(entity);
}

void cf_entity_deactivate(CF_Entity entity)
{
	uint16_t entity_type = s_entity_type(entity);
	CF_EntityCollection* collection = app->entity_collections.try_find(entity_type);
	CF_ASSERT(collection);

	if (collection->entity_handle_table.valid(entity.handle)) {
		if (!collection->entity_handle_table.active(entity.handle)) {
			return;
		}

		int index = collection->entity_handle_table.get_index(entity.handle);

		int copy_index = -1; // TODO.

		// Swap all components into the deactive section (the end) of their respective arrays.
		for (int i = 0; i < collection->component_tables.count(); ++i) {
			collection->component_tables[i].copy(index, copy_index);
		}

		// Update handle of the swapped entity.
		if (index < collection->entity_handles.size()) {
			uint64_t h = collection->entity_handles[index];
			collection->entity_handle_table.update_index(h, index);
		}
	}
}

void cf_entity_activate(CF_Entity entity)
{
}

bool cf_entity_is_active(CF_Entity entity)
{
	return false;
}

bool cf_entity_is_valid(CF_Entity entity)
{
	CF_EntityCollection* collection = s_collection(entity);
	if (collection) return collection->entity_handle_table.valid(entity.handle);
	else return false;
}

void* cf_entity_get_component(CF_Entity entity, const char* component_type)
{
	CF_EntityCollection* collection = s_collection(entity);
	if (!collection) return NULL;

	component_type = sintern(component_type);
	const Array<const char*>& component_type_tuple = collection->component_type_tuple;
	for (int i = 0; i < component_type_tuple.count(); ++i) {
		if (component_type_tuple[i] == component_type) {
			int index = collection->entity_handle_table.get_index(entity.handle);
			return collection->component_tables[i][index];
		}
	}

	return NULL;
}

bool cf_entity_has_component(CF_Entity entity, const char* component_type)
{
	return cf_entity_get_component(entity, component_type) ? true : false;
}

//--------------------------------------------------------------------------------------------------

static inline int s_match(const Array<const char*>& a, const Array<const char*>& b)
{
	int matches = 0;
	for (int i = 0; i < a.count(); ++i) {
		for (int j = 0; j < b.count(); ++j) {
			if (a[i]== b[j]) {
				++matches;
				break;
			}
		}
	}
	return matches;
}

void cf_ecs_run_systems(float dt)
{
	int system_count = app->systems.count();
	for (int i = 0; i < system_count; ++i) {
		CF_SystemInternal* system = app->systems + i;
		CF_SystemUpdateFn* update_fn = system->update_fn;
		auto pre_update_fn = system->pre_update_fn;
		auto post_update_fn = system->post_update_fn;
		void* udata = system->udata;

		if (pre_update_fn) pre_update_fn(dt, udata);

		if (update_fn) {
			for (int j = 0; j < app->entity_collections.count(); ++j) {
				CF_EntityCollection* collection = app->entity_collections.items() + j;
				CF_ASSERT(collection->component_tables.count() == collection->component_type_tuple.count());
				int component_count = collection->component_tables.count();
				app->current_collection_type_being_iterated = app->entity_collections.keys()[j];
				app->current_collection_being_updated = collection;
				CF_DEFER(app->current_collection_type_being_iterated = CF_INVALID_ENTITY_TYPE);
				CF_DEFER(app->current_collection_being_updated = NULL);

				int matches = s_match(system->component_type_tuple, collection->component_type_tuple);

				if (matches == system->component_type_tuple.count()) {
					app->ecs_arrays.count = collection->component_type_tuple.count();
					app->ecs_arrays.ptrs = &collection->component_tables;
					app->ecs_arrays.types = collection->component_type_tuple;
					app->ecs_arrays.entities = collection->entity_handles.data();
					update_fn(dt, &app->ecs_arrays, collection->component_tables[0].count(), udata);
				}
			}
		}

		if (post_update_fn) post_update_fn(dt, udata);
	}

	for (int i = 0; i < app->delayed_destroy_entities.count(); ++i) {
		CF_Entity e = app->delayed_destroy_entities[i];
		cf_destroy_entity(e);
	}
	app->delayed_destroy_entities.clear();
}

//--------------------------------------------------------------------------------------------------

void cf_ecs_component_begin()
{
	app->component_config_builder.clear();
}

void cf_ecs_component_end()
{
	app->component_configs.insert(app->component_config_builder.name, app->component_config_builder);
}

void cf_ecs_component_set_name(const char* name)
{
	app->component_config_builder.name = sintern(name);
}

void cf_ecs_component_set_size(size_t size)
{
	app->component_config_builder.size_of_component = size;
}

void cf_ecs_component_set_optional_serializer(CF_ComponentSerializeFn* serializer_fn, void* udata)
{
	app->component_config_builder.serializer_fn = serializer_fn;
	app->component_config_builder.serializer_udata = udata;
}

void cf_ecs_component_set_optional_cleanup(CF_ComponentCleanupFn* cleanup_fn, void* udata)
{
	app->component_config_builder.cleanup_fn = cleanup_fn;
	app->component_config_builder.cleanup_udata = udata;
}

static const char* s_kv_string(CF_KeyValue* kv, const char* key)
{
	if (!cf_kv_key(kv, key, NULL)) {
		if (CF_STRCMP(key, "inherits_from")) {
			CF_DEBUG_PRINTF("Unable to find the `%s` key.\n", key);
		}
		return NULL;
	}

	const char* string_raw;
	size_t string_sz;
	if (!cf_kv_val_string(kv, &string_raw, &string_sz)) {
		CF_DEBUG_PRINTF("`%s` key found, but is not a string.\n", key);
		return NULL;
	}

	return sintern_range(string_raw, string_raw + string_sz);
}

static void s_register_entity_type(const char* schema)
{
	// Parse the schema.
	CF_KeyValue* kv = cf_kv_read(schema, CF_STRLEN(schema), NULL);
	bool cleanup_kv = true;
	CF_DEFER(if (cleanup_kv) cf_kv_destroy(kv));

	if (!kv) {
		CF_DEBUG_PRINTF("Unable to parse the schema when registering entity type.");
		return;
	}

	const char* entity_type_string = s_kv_string(kv, "entity_type");
	const char* inherits_from_string = s_kv_string(kv, "inherits_from");
	CF_EntityType inherits_from = CF_INVALID_ENTITY_TYPE;
	auto inherits_ptr = app->entity_type_string_to_id.try_find(inherits_from_string);
	if (inherits_ptr) inherits_from = *inherits_ptr;

	// Search for all component types present in the schema.
	int component_config_count = app->component_configs.count();
	const CF_ComponentConfig* component_configs = app->component_configs.items();
	Array<const char*> component_type_tuple;
	for (int i = 0; i < component_config_count; ++i) {
		const CF_ComponentConfig* config = component_configs + i;

		if (cf_kv_key(kv, config->name, NULL)) {
			component_type_tuple.add(sintern(config->name));
		}
	}
	cf_read_reset(kv);

	// Register component types.
	CF_EntityType entity_type = app->entity_type_gen++;
	app->entity_type_string_to_id.insert(entity_type_string, entity_type);
	app->entity_type_id_to_string.add(entity_type_string);
	CF_EntityCollection* collection = app->entity_collections.insert(entity_type);
	for (int i = 0; i < component_type_tuple.count(); ++i) {
		collection->component_type_tuple.add(component_type_tuple[i]);
		CF_TypelessArray& table = collection->component_tables.add();
		CF_ComponentConfig* config = app->component_configs.try_find(component_type_tuple[i]);
		table.m_element_size = config->size_of_component;
	}

	// Store the parsed schema.
	app->entity_parsed_schemas.insert(entity_type, kv);
	if (inherits_from != CF_INVALID_ENTITY_TYPE) {
		app->entity_schema_inheritence.insert(entity_type, inherits_from);
	}

	cleanup_kv = false;
}

static void s_register_entity_type(Array<const char*> component_type_tuple, const char* entity_type_string)
{
	// Search for all component types present in the schema.
	int component_config_count = app->component_configs.count();
	const CF_ComponentConfig* component_configs = app->component_configs.items();
	Array<const char*> component_type_ids;
	for (int i = 0; i < component_config_count; ++i) {
		const CF_ComponentConfig* config = component_configs + i;

		bool found = false;
		for (int i = 0; i < component_type_tuple.count(); ++i) {
			if (!CF_STRCMP(component_type_tuple[i], config->name)) {
				found = true;
				break;
			}
		}

		if (found) {
			component_type_ids.add(sintern(config->name));
		}
	}

	// Register component types.
	const char* entity_type_string_id = sintern(entity_type_string);
	CF_EntityType entity_type = app->entity_type_gen++;
	app->entity_type_string_to_id.insert(entity_type_string_id, entity_type);
	app->entity_type_id_to_string.add(entity_type_string_id);
	CF_EntityCollection* collection = app->entity_collections.insert(entity_type);
	for (int i = 0; i < component_type_ids.count(); ++i) {
		collection->component_type_tuple.add(component_type_ids[i]);
		CF_TypelessArray& table = collection->component_tables.add();
		CF_ComponentConfig* config = app->component_configs.try_find(component_type_ids[i]);
		table.m_element_size = config->size_of_component;
	}
}


void cf_ecs_entity_begin()
{
	app->entity_config_builder.clear();
}

void cf_ecs_entity_end()
{
	if (!app->entity_config_builder.schema.empty()) {
		s_register_entity_type(app->entity_config_builder.schema);
	} else {
		s_register_entity_type(app->entity_config_builder.component_types, app->entity_config_builder.entity_type);
	}
}

void cf_ecs_entity_set_name(const char* entity_type)
{
	app->entity_config_builder.entity_type = entity_type;
}

void cf_ecs_entity_add_component(const char* component_type)
{
	app->entity_config_builder.component_types.add(component_type);
}

void cf_ecs_entity_set_optional_schema(const char* schema)
{
	app->entity_config_builder.schema = schema;
}

const char* cf_entity_get_type_string(CF_Entity entity)
{
	CF_EntityType entity_type = s_entity_type(entity);
	return app->entity_type_id_to_string[entity_type];
}

bool cf_entity_is_type(CF_Entity entity, const char* entity_type_name)
{
	if (!cf_entity_is_valid(entity)) return false;
	const char* type_string = cf_entity_get_type_string(entity);
	return !CF_STRCMP(type_string, entity_type_name);
}

CF_EntityType s_entity_type(CF_KeyValue* kv)
{
	const char* entity_type_string = s_kv_string(kv, "entity_type");
	CF_EntityType entity_type = CF_INVALID_ENTITY_TYPE;
	auto type_ptr = app->entity_type_string_to_id.try_find(entity_type_string);
	if (type_ptr) entity_type = *type_ptr;
	return entity_type;
}

static CF_Result s_fill_load_id_table(CF_KeyValue* kv)
{
	int entity_count;
	if (!cf_kv_array_begin(kv, &entity_count, "entities")) {
		return cf_result_error("Unable to find `entities` array in kv file.");
	}

	while (entity_count--) {
		cf_kv_object_begin(kv, NULL);

		CF_EntityType entity_type = s_entity_type(kv);
		if (entity_type == CF_INVALID_ENTITY_TYPE) {
			return cf_result_error("Unable to find entity type.");
		}

		CF_EntityCollection* collection = app->entity_collections.try_find(entity_type);
		CF_ASSERT(collection);

		int index = collection->entity_handles.count();
		CF_Handle h = collection->entity_handle_table.alloc_handle(index, entity_type);
		collection->entity_handles.add(h);

		CF_Entity entity;
		entity.handle = h;
		app->load_id_table->add(entity);

		cf_kv_object_end(kv);
	}

	cf_kv_array_end(kv);

	return cf_result_success();
}


bool cf_ecs_is_entity_type_valid(const char* entity_type)
{
	if (app->entity_type_string_to_id.try_find(sintern(entity_type))) {
		return true;
	} else {
		return false;
	}
}

Array<const char*> cf_internal_ecs_get_entity_list()
{
	Array<const char*> names;

	for (int i = 0; i < app->entity_type_id_to_string.count(); ++i) {
		const char* name = app->entity_type_id_to_string[i];
		names.add(name);
	}

	return names;
}

const char** cf_ecs_get_entity_list(int* entities_count_out)
{
	Array<const char*> arr = cf_internal_ecs_get_entity_list();

	const char** entities_out = arr.data();
	if (entities_count_out) {
		*entities_count_out = arr.count();
	}

	CF_MEMSET(&arr, 0, sizeof(arr));

	return entities_out;
}

Array<const char*> cf_internal_ecs_get_component_list()
{
	Array<const char*> names;
	int count = app->component_configs.count();
	const char** ids = app->component_configs.keys();

	for (int i = 0; i < count; ++i) {
		const char* name = app->entity_type_id_to_string[i];
		names.add(name);
	}

	return names;
}

const char** cf_ecs_get_component_list(int* components_count_out)
{
	Array<const char*> arr = cf_internal_ecs_get_component_list();

	const char** components_out = arr.data();
	if (components_count_out) {
		*components_count_out = arr.count();
	}

	CF_MEMSET(&arr, 0, sizeof(arr));

	return components_out;
}

Array<const char*> cf_internal_ecs_get_system_list()
{
	Array<const char*> names;

	for (int i = 0; i < app->systems.count(); ++i) {
		const char* name = app->systems[i].name;
		names.add(name);
	}

	return names;
}

const char** cf_ecs_get_system_list(int* systems_count_out)
{
	Array<const char*> arr = cf_internal_ecs_get_system_list();

	const char** systems_out = arr.data();
	if (systems_count_out) {
		*systems_count_out = arr.count();
	}

	CF_MEMSET(&arr, 0, sizeof(arr));

	return systems_out;
}

Array<const char*> cf_internal_ecs_get_component_list_for_entity_type(const char* entity_type)
{
	Array<const char*> result;

	auto type_ptr = app->entity_type_string_to_id.try_find(sintern(entity_type));
	if (!type_ptr) return result;

	CF_EntityType type = *type_ptr;
	CF_EntityCollection* collection = app->entity_collections.try_find(type);
	CF_ASSERT(collection);

	const Array<const char*>& component_type_tuple = collection->component_type_tuple;
	for (int i = 0; i < component_type_tuple.count(); ++i) {
		const char* component_type = component_type_tuple[i];
		CF_ComponentConfig* config = app->component_configs.try_find(component_type);
		CF_ASSERT(config);
		result.add(config->name);
	}

	return result;
}

const char** cf_ecs_get_component_list_for_entity_type(const char* entity_type, int* components_count_out)
{
	Array<const char*> arr = cf_internal_ecs_get_component_list_for_entity_type(entity_type);

	const char** components_out = arr.data();
	if (components_count_out) {
		*components_count_out = arr.count();
	}

	CF_MEMSET(&arr, 0, sizeof(arr));

	return components_out;
}

void cf_ecs_free_list(const char** list)
{
	CF_FREE(list);
}

namespace Cute
{

Result ecs_load_entities(KeyValue* kv, Array<Entity>* entities_out) { return cf_internal_ecs_load_entities(kv, entities_out); }

Result ecs_save_entities(const Array<Entity>& entities, KeyValue* kv) { return cf_internal_ecs_save_entities_kv(entities.data(), entities.count(), kv); }
Result ecs_save_entities(const Array<Entity>& entities) { return cf_internal_ecs_save_entities(entities.data(), entities.count()); }

Array<const char*> ecs_get_entity_list() { return cf_internal_ecs_get_entity_list(); }
Array<const char*> ecs_get_component_list() { return cf_internal_ecs_get_component_list(); }
Array<const char*> ecs_get_system_list() { return cf_internal_ecs_get_system_list(); }
Array<const char*> ecs_get_component_list_for_entity_type(const char* entity_type) { return cf_internal_ecs_get_component_list_for_entity_type(entity_type); }

}
