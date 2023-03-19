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

void cf_system_begin()
{
	app->system_internal_builder.clear();
}

void cf_system_end()
{
	app->systems.add(app->system_internal_builder);
}

void cf_system_set_name(const char* name)
{
	app->system_internal_builder.name = sintern(name);
}

void cf_system_set_update(CF_SystemUpdateFn* update_fn)
{
	app->system_internal_builder.update_fn = update_fn;
}

void cf_system_require_component(const char* component_type)
{
	app->system_internal_builder.component_type_tuple.add(sintern(component_type));
}

void cf_system_set_optional_pre_update(void (*pre_update_fn)(void* udata))
{
	app->system_internal_builder.pre_update_fn = pre_update_fn;
}

void cf_system_set_optional_post_update(void (*post_update_fn)(void* udata))
{
	app->system_internal_builder.post_update_fn = post_update_fn;
}

void cf_system_set_optional_udata(void* udata)
{
	app->system_internal_builder.udata = udata;
}

static CF_INLINE uint16_t s_entity_type(CF_Entity entity)
{
	return (uint16_t)((entity.handle & 0x00000000FFFF0000ULL) >> 16);
}

static CF_WorldInternal* s_world()
{
	return (CF_WorldInternal*)app->world.id;
}

CF_Entity cf_make_entity(const char* entity_type, CF_Result* err_out)
{
	auto type_ptr = app->entity_type_string_to_id.try_find(sintern(entity_type));
	if (!type_ptr) {
		if (err_out) *err_out = cf_result_error("`entity_type` is not valid.");
		return CF_INVALID_ENTITY;
	}
	CF_EntityType type = *type_ptr;

	CF_WorldInternal* world = s_world();
	CF_EntityCollection* collection = world->entity_collections.try_find(type);
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
		CF_MEMSET(component, 0, config->size_of_component);
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
		CF_WorldInternal* world = s_world();
		collection = world->entity_collections.try_find(entity_type);
		if (!collection) return NULL;
	}
	return collection;
}

void cf_destroy_entity(CF_Entity entity)
{
	uint16_t entity_type = s_entity_type(entity);
	CF_WorldInternal* world = s_world();
	CF_EntityCollection* collection = world->entity_collections.try_find(entity_type);
	CF_ASSERT(collection);

	if (collection->entity_handle_table.valid(entity.handle)) {
		int index = collection->entity_handle_table.get_index(entity.handle);

		// Call cleanup function on each component.
		for (int i = 0; i < collection->component_tables.count(); ++i) {
			CF_ComponentConfig* config = app->component_configs.try_find(collection->component_type_tuple[i]);
			if (config && config->cleanup) {
				config->cleanup(entity, collection->component_tables[i][index], config->cleanup_udata);
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
	CF_WorldInternal* world = s_world();
	world->delayed_destroy_entities.add(entity);
}

void cf_entity_delayed_deactivate(CF_Entity entity)
{
	CF_WorldInternal* world = s_world();
	world->delayed_deactivate_entities.add(entity);
}

void cf_entity_delayed_activate(CF_Entity entity)
{
	CF_WorldInternal* world = s_world();
	world->delayed_activate_entities.add(entity);
}

void cf_entity_deactivate(CF_Entity entity)
{
	uint16_t entity_type = s_entity_type(entity);
	CF_WorldInternal* world = s_world();
	CF_EntityCollection* collection = world->entity_collections.try_find(entity_type);
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

void cf_run_systems()
{
	CF_WorldInternal* world = s_world();
	int system_count = app->systems.count();
	for (int i = 0; i < system_count; ++i) {
		CF_SystemInternal* system = app->systems + i;
		CF_SystemUpdateFn* update_fn = system->update_fn;
		auto pre_update_fn = system->pre_update_fn;
		auto post_update_fn = system->post_update_fn;
		void* udata = system->udata;

		if (pre_update_fn) pre_update_fn(udata);

		if (update_fn) {
			for (int j = 0; j < world->entity_collections.count(); ++j) {
				CF_EntityCollection* collection = world->entity_collections.items() + j;
				CF_ASSERT(collection->component_tables.count() == collection->component_type_tuple.count());
				int component_count = collection->component_tables.count();
				app->current_collection_type_being_iterated = world->entity_collections.keys()[j];
				app->current_collection_being_updated = collection;
				CF_DEFER(app->current_collection_type_being_iterated = CF_INVALID_ENTITY_TYPE);
				CF_DEFER(app->current_collection_being_updated = NULL);

				int matches = s_match(system->component_type_tuple, collection->component_type_tuple);
				CF_ComponentList component_list = { (uint64_t)&app->component_list };

				if (matches == system->component_type_tuple.count()) {
					app->component_list.count = collection->component_type_tuple.count();
					app->component_list.ptrs = &collection->component_tables;
					app->component_list.types = collection->component_type_tuple;
					app->component_list.entities = collection->entity_handles.data();
					update_fn(component_list, collection->component_tables[0].count(), udata);
				}
			}
		}

		if (post_update_fn) post_update_fn(udata);
	}

	for (int i = 0; i < world->delayed_destroy_entities.count(); ++i) {
		CF_Entity e = world->delayed_destroy_entities[i];
		cf_destroy_entity(e);
	}
	world->delayed_destroy_entities.clear();
}

void cf_component_begin()
{
	app->component_config_builder.clear();
}

void cf_component_end()
{
	app->component_configs.insert(app->component_config_builder.name, app->component_config_builder);
}

void cf_component_set_name(const char* name)
{
	app->component_config_builder.name = sintern(name);
}

void cf_component_set_size(size_t size)
{
	app->component_config_builder.size_of_component = size;
}

void cf_component_set_optional_initializer(CF_ComponentFn* initializer, void* udata)
{
	app->component_config_builder.initializer = initializer;
	app->component_config_builder.initializer_udata = udata;
}

void cf_component_set_optional_cleanup(CF_ComponentFn* cleanup, void* udata)
{
	app->component_config_builder.cleanup = cleanup;
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
	CF_WorldInternal* world = s_world();
	CF_EntityCollection* collection = world->entity_collections.insert(entity_type);
	for (int i = 0; i < component_type_ids.count(); ++i) {
		collection->component_type_tuple.add(component_type_ids[i]);
		CF_TypelessArray& table = collection->component_tables.add();
		CF_ComponentConfig* config = app->component_configs.try_find(component_type_ids[i]);
		table.m_element_size = config->size_of_component;
	}
}


void cf_entity_begin()
{
	app->entity_config_builder.clear();
}

void cf_entity_end()
{
	s_register_entity_type(app->entity_config_builder.component_types, app->entity_config_builder.entity_type);
}

void cf_entity_set_name(const char* entity_type)
{
	app->entity_config_builder.entity_type = entity_type;
}

void cf_entity_add_component(const char* component_type)
{
	app->entity_config_builder.component_types.add(component_type);
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

bool cf_is_entity_type_valid(const char* entity_type)
{
	if (app->entity_type_string_to_id.try_find(sintern(entity_type))) {
		return true;
	} else {
		return false;
	}
}

CF_World cf_make_world()
{
	CF_WorldInternal* world = CF_NEW(CF_WorldInternal);
	CF_World result;
	result.id = (uint64_t)world;
	return result;
}

void cf_destroy_world(CF_World world_handle)
{
	CF_WorldInternal* world = (CF_WorldInternal*)world_handle.id;
	world->~CF_WorldInternal();
	CF_FREE(world);
}

void cf_world_push(CF_World world)
{
	app->worlds.add(app->world);
	app->world = world;
}

CF_World cf_world_pop()
{
	if (app->worlds.count() > 1) {
		app->world = app->worlds.pop();
	}
	return app->world;
}

CF_World cf_world_peek()
{
	return app->world;
}

dyna const char** cf_get_entity_list()
{
	dyna const char** names = NULL;
	for (int i = 0; i < app->entity_type_id_to_string.count(); ++i) {
		const char* name = app->entity_type_id_to_string[i];
		apush(names, name);
	}

	return names;
}

dyna const char** cf_get_component_list()
{
	dyna const char** names = NULL;
	int count = app->component_configs.count();
	const char** ids = app->component_configs.keys();

	for (int i = 0; i < count; ++i) {
		const char* name = ids[i];
		apush(names, name);
	}

	return names;
}

dyna const char** cf_get_system_list()
{
	dyna const char** names = NULL;

	for (int i = 0; i < app->systems.count(); ++i) {
		const char* name = app->systems[i].name;
		apush(names, name);
	}

	return names;
}

dyna const char** cf_get_component_list_for_entity_type(const char* entity_type)
{
	dyna const char** result = NULL;
	auto type_ptr = app->entity_type_string_to_id.try_find(sintern(entity_type));
	if (!type_ptr) return result;

	CF_WorldInternal* world = s_world();
	CF_EntityType type = *type_ptr;
	CF_EntityCollection* collection = world->entity_collections.try_find(type);
	CF_ASSERT(collection);

	const Array<const char*>& component_type_tuple = collection->component_type_tuple;
	for (int i = 0; i < component_type_tuple.count(); ++i) {
		const char* component_type = component_type_tuple[i];
		CF_ComponentConfig* config = app->component_configs.try_find(component_type);
		CF_ASSERT(config);
		apush(result, config->name);
	}

	return result;
}
