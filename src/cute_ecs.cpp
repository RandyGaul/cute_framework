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

using namespace Cute;

static bool s_load_from_schema(CF_EntityType schema_type, CF_Entity entity, cf_component_config_t* config, void* component, void* udata)
{
	// Look for parent.
	// If parent exists, load values from it first.
	auto inherit_ptr = app->entity_schema_inheritence.try_find(schema_type);
	if (inherit_ptr) {
		CF_EntityType inherits_from = *inherit_ptr;
		if (!s_load_from_schema(inherits_from, entity, config, component, udata)) {
			return false;
		}
	}

	auto schema_ptr = app->entity_parsed_schemas.try_find(schema_type);
	bool result = true;
	if (!schema_ptr) {
		if (config->serializer_fn) result = config->serializer_fn(NULL, true, entity, component, udata);
	} else {
		CF_KeyValue* schema = *schema_ptr;
		result = cf_kv_object_begin(schema, config->name);
		if (result) {
			if (config->serializer_fn) result = config->serializer_fn(schema, true, entity, component, udata);
			if (!result) return result;
			result = cf_kv_object_end(schema);
		}
	}

	return result;
}

//--------------------------------------------------------------------------------------------------

void* cf_ecs_arrays_find_components(CF_EcsArrays* arrays, const char* component_type)
{
	return arrays->find_components(component_type);
}

CF_Entity* cf_ecs_arrays_get_entities(CF_EcsArrays* arrays)
{
	return (CF_Entity*)arrays->entities;
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

static CUTE_INLINE uint16_t s_Entityype(CF_Entity entity)
{
	return (uint16_t)((entity.handle & 0x00000000FFFF0000ULL) >> 16);
}

CF_Entity cf_make_entity(const char* Entityype, CF_Result* err_out)
{
	auto type_ptr = app->Entityype_string_to_id.try_find(sintern(Entityype));
	if (!type_ptr) {
		if (err_out) *err_out = cf_result_error("`Entityype` is not valid.");
		return CF_INVALID_ENTITY;
	}
	CF_EntityType type = *type_ptr;

	cf_entity_collection_t* collection = app->entity_collections.try_find(type);
	CUTE_ASSERT(collection);

	int index = collection->entity_handles.count();
	CF_Handle h = collection->entity_handle_table.alloc_handle(index, type);
	collection->entity_handles.add(h);
	CF_Entity entity = { h };

	const Array<const char*>& component_type_tuple = collection->component_type_tuple;
	for (int i = 0; i < component_type_tuple.count(); ++i) {
		const char* component_type = component_type_tuple[i];
		cf_component_config_t* config = app->component_configs.try_find(component_type);

		if (!config) {
			if (err_out) *err_out = cf_result_error("Unable to find component config.");
			return CF_INVALID_ENTITY;
		}

		void* component = collection->component_tables[i].add();
		if (!s_load_from_schema(type, entity, config, component, config->serializer_udata)) {
			// TODO - Unload the components that were added with `.add()` a couple lines above here.
			return CF_INVALID_ENTITY;
		}
	}

	if (err_out) *err_out = cf_result_success();
	return entity;
}

static cf_entity_collection_t* s_collection(CF_Entity entity)
{
	cf_entity_collection_t* collection = NULL;
	uint16_t Entityype = s_Entityype(entity);
	if (Entityype == app->current_collection_type_being_iterated) {
		// Fast path -- check the current entity collection for this entity type first.
		collection = app->current_collection_being_updated;
		CUTE_ASSERT(collection);
	} else {
		// Slightly slower path -- lookup collection first.
		collection = app->entity_collections.try_find(Entityype);
		if (!collection) return NULL;
	}
	return collection;
}

void cf_destroy_entity(CF_Entity entity)
{
	uint16_t Entityype = s_Entityype(entity);
	cf_entity_collection_t* collection = app->entity_collections.try_find(Entityype);
	CUTE_ASSERT(collection);

	if (collection->entity_handle_table.valid(entity.handle)) {
		int index = collection->entity_handle_table.get_index(entity.handle);

		// Call cleanup function on each component.
		for (int i = 0; i < collection->component_tables.count(); ++i) {
			cf_component_config_t* config = app->component_configs.try_find(collection->component_type_tuple[i]);
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
	uint16_t Entityype = s_Entityype(entity);
	cf_entity_collection_t* collection = app->entity_collections.try_find(Entityype);
	CUTE_ASSERT(collection);

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
	cf_entity_collection_t* collection = s_collection(entity);
	if (collection) return collection->entity_handle_table.valid(entity.handle);
	else return false;
}

void* cf_entity_get_component(CF_Entity entity, const char* component_type)
{
	cf_entity_collection_t* collection = s_collection(entity);
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
		cf_system_internal_t* system = app->systems + i;
		CF_SystemUpdateFn* update_fn = system->update_fn;
		auto pre_update_fn = system->pre_update_fn;
		auto post_update_fn = system->post_update_fn;
		void* udata = system->udata;

		if (pre_update_fn) pre_update_fn(dt, udata);

		if (update_fn) {
			for (int j = 0; j < app->entity_collections.count(); ++j) {
				cf_entity_collection_t* collection = app->entity_collections.items() + j;
				CUTE_ASSERT(collection->component_tables.count() == collection->component_type_tuple.count());
				int component_count = collection->component_tables.count();
				app->current_collection_type_being_iterated = app->entity_collections.keys()[j];
				app->current_collection_being_updated = collection;
				CUTE_DEFER(app->current_collection_type_being_iterated = CF_INVALID_ENTITY_TYPE);
				CUTE_DEFER(app->current_collection_being_updated = NULL);

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
		if (CUTE_STRCMP(key, "inherits_from")) {
			CUTE_DEBUG_PRINTF("Unable to find the `%s` key.\n", key);
		}
		return { 0 };
	}

	const char* string_raw;
	size_t string_sz;
	if (!cf_kv_val_string(kv, &string_raw, &string_sz)) {
		CUTE_DEBUG_PRINTF("`%s` key found, but is not a string.\n", key);
		return { 0 };
	}

	return sintern_range(string_raw, string_raw + string_sz);
}

static void s_register_Entityype(const char* schema)
{
	// Parse the schema.
	CF_KeyValue* kv = cf_kv_read(schema, CUTE_STRLEN(schema), NULL);
	bool cleanup_kv = true;
	CUTE_DEFER(if (cleanup_kv) cf_kv_destroy(kv));

	if (!kv) {
		CUTE_DEBUG_PRINTF("Unable to parse the schema when registering entity type.");
		return;
	}

	const char* Entityype_string = s_kv_string(kv, "Entityype");
	const char* inherits_from_string = s_kv_string(kv, "inherits_from");
	CF_EntityType inherits_from = CF_INVALID_ENTITY_TYPE;
	auto inherits_ptr = app->Entityype_string_to_id.try_find(inherits_from_string);
	if (inherits_ptr) inherits_from = *inherits_ptr;

	// Search for all component types present in the schema.
	int component_config_count = app->component_configs.count();
	const cf_component_config_t* component_configs = app->component_configs.items();
	Array<const char*> component_type_tuple;
	for (int i = 0; i < component_config_count; ++i) {
		const cf_component_config_t* config = component_configs + i;

		if (cf_kv_key(kv, config->name, NULL)) {
			component_type_tuple.add(sintern(config->name));
		}
	}
	cf_read_reset(kv);

	// Register component types.
	CF_EntityType Entityype = app->Entityype_gen++;
	app->Entityype_string_to_id.insert(Entityype_string, Entityype);
	app->Entityype_id_to_string.add(Entityype_string);
	cf_entity_collection_t* collection = app->entity_collections.insert(Entityype);
	for (int i = 0; i < component_type_tuple.count(); ++i) {
		collection->component_type_tuple.add(component_type_tuple[i]);
		CF_TypelessArray& table = collection->component_tables.add();
		cf_component_config_t* config = app->component_configs.try_find(component_type_tuple[i]);
		table.m_element_size = config->size_of_component;
	}

	// Store the parsed schema.
	app->entity_parsed_schemas.insert(Entityype, kv);
	if (inherits_from != CF_INVALID_ENTITY_TYPE) {
		app->entity_schema_inheritence.insert(Entityype, inherits_from);
	}

	cleanup_kv = false;
}

static void s_register_Entityype(Array<const char*> component_type_tuple, const char* Entityype_string)
{
	// Search for all component types present in the schema.
	int component_config_count = app->component_configs.count();
	const cf_component_config_t* component_configs = app->component_configs.items();
	Array<const char*> component_type_ids;
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
			component_type_ids.add(sintern(config->name));
		}
	}

	// Register component types.
	const char* Entityype_string_id = sintern(Entityype_string);
	CF_EntityType Entityype = app->Entityype_gen++;
	app->Entityype_string_to_id.insert(Entityype_string_id, Entityype);
	app->Entityype_id_to_string.add(Entityype_string_id);
	cf_entity_collection_t* collection = app->entity_collections.insert(Entityype);
	for (int i = 0; i < component_type_ids.count(); ++i) {
		collection->component_type_tuple.add(component_type_ids[i]);
		CF_TypelessArray& table = collection->component_tables.add();
		cf_component_config_t* config = app->component_configs.try_find(component_type_ids[i]);
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
		s_register_Entityype(app->entity_config_builder.schema);
	} else {
		s_register_Entityype(app->entity_config_builder.component_types, app->entity_config_builder.Entityype);
	}
}

void cf_ecs_entity_set_name(const char* Entityype)
{
	app->entity_config_builder.Entityype = Entityype;
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
	CF_EntityType Entityype = s_Entityype(entity);
	return app->Entityype_id_to_string[Entityype];
}

bool cf_entity_is_type(CF_Entity entity, const char* Entityype_name)
{
	if (!cf_entity_is_valid(entity)) return false;
	const char* type_string = cf_entity_get_type_string(entity);
	return !CUTE_STRCMP(type_string, Entityype_name);
}

CF_EntityType s_Entityype(CF_KeyValue* kv)
{
	const char* Entityype_string = s_kv_string(kv, "Entityype");
	CF_EntityType Entityype = CF_INVALID_ENTITY_TYPE;
	auto type_ptr = app->Entityype_string_to_id.try_find(Entityype_string);
	if (type_ptr) Entityype = *type_ptr;
	return Entityype;
}

static CF_Result s_fill_load_id_table(CF_KeyValue* kv)
{
	int entity_count;
	if (!cf_kv_array_begin(kv, &entity_count, "entities")) {
		return cf_result_error("Unable to find `entities` array in kv file.");
	}

	while (entity_count--) {
		cf_kv_object_begin(kv, NULL);

		CF_EntityType Entityype = s_Entityype(kv);
		if (Entityype == CF_INVALID_ENTITY_TYPE) {
			return cf_result_error("Unable to find entity type.");
		}

		cf_entity_collection_t* collection = app->entity_collections.try_find(Entityype);
		CUTE_ASSERT(collection);

		int index = collection->entity_handles.count();
		CF_Handle h = collection->entity_handle_table.alloc_handle(index, Entityype);
		collection->entity_handles.add(h);

		CF_Entity entity;
		entity.handle = h;
		app->load_id_table->add(entity);

		cf_kv_object_end(kv);
	}

	cf_kv_array_end(kv);

	return cf_result_success();
}



CF_Result cf_internal_ecs_load_entities(CF_KeyValue* kv, Array<CF_Entity>* entities_out)
{
	if (cf_kv_state(kv) != CF_KV_STATE_READ) {
		return cf_result_error("`kv` must be in `KV_STATE_READ` mode.");
	}

	Array<CF_Entity> load_id_table;
	app->load_id_table = &load_id_table;
	CUTE_DEFER(app->load_id_table = NULL);

	CF_Result err = s_fill_load_id_table(kv);
	if (cf_is_error(err)) return err;

	int entity_count;
	if (!cf_kv_array_begin(kv, &entity_count, "entities")) {
		return cf_result_error("Unable to find `entities` array in kv file.");
	}

	int entity_index = 0;
	while (entity_count--) {
		CF_Entity entity = load_id_table[entity_index++];
		cf_kv_object_begin(kv, NULL);

		CF_EntityType Entityype = s_Entityype(kv);
		if (Entityype == CF_INVALID_ENTITY_TYPE) {
			return cf_result_error("Unable to find entity type.");
		}

		cf_entity_collection_t* collection = app->entity_collections.try_find(Entityype);
		CUTE_ASSERT(collection);

		const Array<const char*>& component_type_tuple = collection->component_type_tuple;
		for (int i = 0; i < component_type_tuple.count(); ++i) {
			const char* component_type = component_type_tuple[i];
			cf_component_config_t* config = app->component_configs.try_find(component_type);

			if (!config) {
				return cf_result_error("Unable to find component config.");
			}

			// First load values from the schema.
			void* component = collection->component_tables[i].add();
			if (!s_load_from_schema(Entityype, entity, config, component, config->serializer_udata)) {
				return cf_result_error("Unable to parse component from schema.");
			}

			// Then load values from the instance.
			if (cf_kv_object_begin(kv, config->name)) {
				if (!config->serializer_fn(kv, true, entity, component, config->serializer_udata)) {
					return cf_result_error("Unable to parse component.");
				}
				cf_kv_object_end(kv);
			}
		}

		cf_kv_object_end(kv);
	}

	cf_kv_array_end(kv);

	if (entities_out) {
		entities_out->steal_from(&load_id_table);
	}

	return cf_result_success();
}

CF_Result cf_ecs_load_entities(CF_KeyValue* kv, CF_Entity** entities_out, int* entities_count_out)
{
	CF_Result err;

	if (entities_out) {
		CUTE_ASSERT(entities_count_out);

		Array<Cute::Entity> arr = {};

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

void cf_ecs_free_entities(CF_Entity* entities)
{
	CUTE_FREE(entities);
}

CF_Result cf_internal_ecs_save_entities_kv(const CF_Entity* entities, int entities_count, CF_KeyValue* kv)
{
	if (cf_kv_state(kv) != CF_KV_STATE_WRITE) {
		return cf_result_error("`kv` must be in `KV_STATE_WRITE` mode.");
	}

	Cute::Map<CF_Entity, int> id_table;
	for (int i = 0; i < entities_count; ++i)
		id_table.insert(entities[i], i);

	app->save_id_table = &id_table;
	CUTE_DEFER(app->save_id_table = NULL);

	int entity_count = entities_count;
	if (!cf_kv_array_begin(kv, &entity_count, "entities")) {
		return cf_result_error("Unable to find `entities` array.");
	}

	for (int i = 0; i < entities_count; ++i) {
		CF_Entity entity = entities[i];
		CF_EntityType Entityype = s_Entityype(entity);
		cf_entity_collection_t* collection = app->entity_collections.try_find(Entityype);
		if (!collection) {
			return cf_result_error("Unable to find entity type.");
		}

		bool is_valid = collection->entity_handle_table.valid(entity.handle);
		if (!is_valid) {
			return cf_result_error("Attempted to save an invalid entity.");
		}
		uint32_t index = collection->entity_handle_table.get_index(entity.handle);

		cf_kv_object_begin(kv, NULL);

		cf_kv_key(kv, "Entityype", NULL);
		const char* Entityype_string = app->Entityype_id_to_string[Entityype];
		size_t Entityype_string_len = CUTE_STRLEN(Entityype_string);
		cf_kv_val_string(kv, &Entityype_string, &Entityype_string_len);

		const Array<const char*>& component_type_tuple = collection->component_type_tuple;
		const Array<CF_TypelessArray>& component_tables = collection->component_tables;
		for (int j = 0; j < component_type_tuple.count(); ++j) {
			const char* component_type = component_type_tuple[j];
			const CF_TypelessArray& component_table = component_tables[j];
			cf_component_config_t* config = app->component_configs.try_find(component_type);
			const void* component = component_table[index];

			if (cf_kv_object_begin(kv, config->name)) {
				if (!config->serializer_fn(kv, false, entity, (void*)component, config->serializer_udata)) {
					return cf_result_error("Unable to save component.");
				}
				cf_kv_object_end(kv);
			}
		}

		cf_kv_object_end(kv);
	}

	cf_kv_array_end(kv);

	return cf_result_success();
}

CF_Result cf_ecs_save_entities_kv(const CF_Entity* entities, int entities_count, CF_KeyValue* kv)
{
	CF_Result err = cf_internal_ecs_save_entities_kv(entities, entities_count, kv);
	return err;
}

CF_Result cf_internal_ecs_save_entities(const CF_Entity* entities, int entities_count)
{
	Cute::Map<CF_Entity, int> id_table;
	for (int i = 0; i < entities_count; ++i) {
		id_table.insert(entities[i], i);
	}

	app->save_id_table = &id_table;
	CUTE_DEFER(app->save_id_table = NULL);

	int entity_count = entities_count;
	for (int i = 0; i < entities_count; ++i) {
		CF_Entity entity = entities[i];
		CF_EntityType Entityype = s_Entityype(entity);
		cf_entity_collection_t* collection = app->entity_collections.try_find(Entityype);
		if (!collection) {
			return cf_result_error("Unable to find entity type.");
		}

		bool is_valid = collection->entity_handle_table.valid(entity.handle);
		if (!is_valid) {
			return cf_result_error("Attempted to save an invalid entity.");
		}
		uint32_t index = collection->entity_handle_table.get_index(entity.handle);

		const char* Entityype_string = app->Entityype_id_to_string[Entityype];
		const Array<const char*>& component_type_tuple = collection->component_type_tuple;
		const Array<CF_TypelessArray>& component_tables = collection->component_tables;
		for (int j = 0; j < component_type_tuple.count(); ++j) {
			const char* component_type = component_type_tuple[j];
			const CF_TypelessArray& component_table = component_tables[j];
			cf_component_config_t* config = app->component_configs.try_find(component_type);
			const void* component = component_table[index];

			if (!config->serializer_fn(NULL, false, entity, (void*)component, config->serializer_udata)) {
				return cf_result_error("Unable to save component.");
			}
		}
	}

	return cf_result_success();
}

CF_Result cf_ecs_save_entities(const CF_Entity* entities, int entities_count)
{
	CF_Result err = cf_internal_ecs_save_entities(entities, entities_count);
	return err;
}

bool cf_ecs_is_Entityype_valid(const char* Entityype)
{
	if (app->Entityype_string_to_id.try_find(sintern(Entityype))) {
		return true;
	} else {
		return false;
	}
}

Array<const char*> cf_internal_ecs_get_entity_list()
{
	Array<const char*> names;

	for (int i = 0; i < app->Entityype_id_to_string.count(); ++i) {
		const char* name = app->Entityype_id_to_string[i];
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

	CUTE_MEMSET(&arr, 0, sizeof(arr));

	return entities_out;
}

Array<const char*> cf_internal_ecs_get_component_list()
{
	Array<const char*> names;
	int count = app->component_configs.count();
	const char** ids = app->component_configs.keys();

	for (int i = 0; i < count; ++i) {
		const char* name = app->Entityype_id_to_string[i];
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

	CUTE_MEMSET(&arr, 0, sizeof(arr));

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

	CUTE_MEMSET(&arr, 0, sizeof(arr));

	return systems_out;
}

Array<const char*> cf_internal_ecs_get_component_list_for_Entityype(const char* Entityype)
{
	Array<const char*> result;

	auto type_ptr = app->Entityype_string_to_id.try_find(sintern(Entityype));
	if (!type_ptr) return result;

	CF_EntityType type = *type_ptr;
	cf_entity_collection_t* collection = app->entity_collections.try_find(type);
	CUTE_ASSERT(collection);

	const Array<const char*>& component_type_tuple = collection->component_type_tuple;
	for (int i = 0; i < component_type_tuple.count(); ++i) {
		const char* component_type = component_type_tuple[i];
		cf_component_config_t* config = app->component_configs.try_find(component_type);
		CUTE_ASSERT(config);
		result.add(config->name);
	}

	return result;
}

const char** cf_ecs_get_component_list_for_Entityype(const char* Entityype, int* components_count_out)
{
	Array<const char*> arr = cf_internal_ecs_get_component_list_for_Entityype(Entityype);

	const char** components_out = arr.data();
	if (components_count_out) {
		*components_count_out = arr.count();
	}

	CUTE_MEMSET(&arr, 0, sizeof(arr));

	return components_out;
}

void cf_ecs_free_list(const char** list)
{
	CUTE_FREE(list);
}

namespace Cute
{

Result ecs_load_entities(KeyValue* kv, Array<Entity>* entities_out) { return cf_internal_ecs_load_entities(kv, entities_out); }

Result ecs_save_entities(const Array<Entity>& entities, KeyValue* kv) { return cf_internal_ecs_save_entities_kv(entities.data(), entities.count(), kv); }
Result ecs_save_entities(const Array<Entity>& entities) { return cf_internal_ecs_save_entities(entities.data(), entities.count()); }

Array<const char*> ecs_get_entity_list() { return cf_internal_ecs_get_entity_list(); }
Array<const char*> ecs_get_component_list() { return cf_internal_ecs_get_component_list(); }
Array<const char*> ecs_get_system_list() { return cf_internal_ecs_get_system_list(); }
Array<const char*> ecs_get_component_list_for_Entityype(const char* Entityype) { return cf_internal_ecs_get_component_list_for_Entityype(Entityype); }

}
