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

static CF_WorldInternal* s_world()
{
	return (CF_WorldInternal*)app->world.id;
}

static CF_INLINE uint16_t s_entity_type(CF_Entity entity)
{
	CF_WorldInternal* world = s_world();
	return world->handles.get_type(entity.handle);
}

CF_Entity cf_make_entity(const char* entity_type)
{
	auto type_ptr = app->entity_type_string_to_id.try_find(sintern(entity_type));
	if (!type_ptr) {
		return CF_INVALID_ENTITY;
	}
	CF_EntityType type = *type_ptr;

	CF_WorldInternal* world = s_world();
	CF_EntityCollection* collection = world->entity_collections.find(type);
	CF_ASSERT(collection);

	// Create the entity handle.
	int index = collection->entity_handles.count();
	CF_Handle h = world->handles.alloc_handle(index, type);
	collection->entity_handles.add(h);
	CF_Entity entity = { h };

	// Create and initialize each component.
	const Array<const char*>& tuple = collection->component_type_tuple;
	for (int i = 0; i < tuple.count(); ++i) {
		const char* component_type = tuple[i];
		CF_ComponentConfig* config = app->component_configs.try_find(component_type);
		CF_ASSERT(config); // `component_type` is not a valid type.
		void* component = collection->component_tables[i].add();
		CF_MEMSET(component, 0, config->size_of_component);
		if (config->initializer) {
			config->initializer(entity, component, config->initializer_udata);
		}
	}

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
		collection = world->entity_collections.find(entity_type);
		if (!collection) return NULL;
	}
	return collection;
}

void cf_destroy_entity(CF_Entity entity)
{
	CF_WorldInternal* world = s_world();
	if (world->handles.valid(entity.handle)) {
		uint16_t entity_type = s_entity_type(entity);
		CF_EntityCollection* collection = world->entity_collections.find(entity_type);
		CF_ASSERT(collection);
		int index = world->handles.get_index(entity.handle);

		// Call cleanup function on each component in reverse order.
		for (int i = collection->component_tables.count() - 1; i >= 0 ; --i) {
			CF_ComponentConfig* config = app->component_configs.try_find(collection->component_type_tuple[i]);
			if (config->cleanup) {
				config->cleanup(entity, collection->component_tables[i][index], config->cleanup_udata);
			}
		}

		// Update index in case user changed it (by destroying enties).
		index = world->handles.get_index(entity.handle);

		// Free the handle.
		collection->entity_handles.unordered_remove(index);
		world->handles.free_handle(entity.handle);

		// Free each component.
		for (int i = 0; i < collection->component_tables.count(); ++i) {
			collection->component_tables[i].unordered_remove(index);
		}

		// Update handle of the swapped entity.
		if (index < collection->entity_handles.size()) {
			CF_Handle h = collection->entity_handles[index];
			world->handles.update_index(h, index);
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
	CF_EntityCollection* collection = world->entity_collections.find(entity_type);

	if (world->handles.valid(entity.handle)) {
		if (!world->handles.active(entity.handle)) {
			return;
		}

		int index = world->handles.get_index(entity.handle);
		int last_active_index = collection->entity_handles.count() - collection->inactive_count - 1;

		// Swap the component to the end of the active section.
		for (int i = 0; i < collection->component_tables.count(); ++i) {
			collection->component_tables[i].swap(index, last_active_index);
		}

		collection->inactive_count++;

		// Update indices for each swapped entity.
		CF_Handle handle = collection->entity_handles[index];
		CF_Handle last_active_handle = collection->entity_handles[last_active_index];
		CF_ASSERT(handle == entity.handle);
		collection->entity_handles[index] = last_active_handle;
		collection->entity_handles[last_active_index] = handle;
		world->handles.update_index(handle, last_active_index);
		world->handles.update_index(last_active_handle, index);

		// Mark handle as inactive.
		world->handles.deactivate(handle);
	}
}

void cf_entity_activate(CF_Entity entity)
{
	uint16_t entity_type = s_entity_type(entity);
	CF_WorldInternal* world = s_world();
	CF_EntityCollection* collection = world->entity_collections.find(entity_type);

	if (world->handles.valid(entity.handle)) {
		if (world->handles.active(entity.handle)) {
			return;
		}

		int index = world->handles.get_index(entity.handle);
		int last_inactive_index = collection->entity_handles.count() - collection->inactive_count;

		// Swap the inactive component to the beginning of the inactive section.
		for (int i = 0; i < collection->component_tables.count(); ++i) {
			collection->component_tables[i].swap(index, last_inactive_index);
		}

		collection->inactive_count--;

		// Update indices for each swapped entity.
		CF_Handle handle = collection->entity_handles[index];
		CF_Handle last_inactive_handle = collection->entity_handles[last_inactive_index];
		CF_ASSERT(handle == entity.handle);
		collection->entity_handles[index] = last_inactive_handle;
		collection->entity_handles[last_inactive_index] = handle;
		world->handles.update_index(handle, last_inactive_index);
		world->handles.update_index(last_inactive_handle, index);

		// Mark handle as active.
		world->handles.activate(handle);
	}
}

bool cf_entity_is_active(CF_Entity entity)
{
	CF_WorldInternal* world = s_world();
	return world->handles.active(entity.handle);
}

bool cf_entity_is_valid(CF_Entity entity)
{
	CF_WorldInternal* world = s_world();
	return world->handles.valid(entity.handle);
}

void cf_entity_delayed_change_type(CF_Entity entity, const char* entity_type)
{
	auto type_ptr = app->entity_type_string_to_id.try_find(entity_type);
	if (type_ptr && cf_entity_is_valid(entity)) {
		CF_WorldInternal* world = s_world();
		CF_ChangeType change;
		change.entity = entity;
		change.type = *type_ptr;
		world->delayed_change_type.add(change);
	}
}

void cf_entity_change_type(CF_Entity entity, const char* entity_type)
{
	// Lookup new/old entity types.
	entity_type = sintern(entity_type);
	CF_EntityType old_type = s_entity_type(entity);
	auto type_ptr = app->entity_type_string_to_id.try_find(sintern(entity_type));
	if (!type_ptr) return;
	CF_EntityType new_type = *type_ptr;
	if (old_type == new_type) return;

	// Look new/old collections.
	CF_WorldInternal* world = s_world();
	CF_EntityCollection* new_collection = world->entity_collections.find(new_type);
	CF_ASSERT(new_collection);
	CF_EntityCollection* old_collection = world->entity_collections.find(old_type);
	CF_ASSERT(old_collection);

	// Place entity handle into the new collection.
	int old_index = world->handles.get_index(entity.handle);
	int new_index = new_collection->entity_handles.count();
	new_collection->entity_handles.add(entity.handle);

	// Construct the new components.
	const Array<const char*>& new_tuple = new_collection->component_type_tuple;
	const Array<const char*>& old_tuple = old_collection->component_type_tuple;
	for (int i = 0; i < new_tuple.count(); ++i) {
		// Allocate the new component.
		const char* new_component_type = new_tuple[i];
		CF_ComponentConfig* config = app->component_configs.try_find(new_component_type);
		CF_ASSERT(config); // `new_component_type` is not a valid type.
		void* new_component = new_collection->component_tables[i].add();

		// Look for a matching old component.
		bool match = false;
		for (int i = 0; i < old_tuple.count(); ++i) {
			const char* old_component_type = old_tuple[i];
			if (new_component_type == old_component_type) {
				// Copy over contents to the new component.
				void* old_component = old_collection->component_tables[i][old_index];
				CF_MEMCPY(new_component, old_component, config->size_of_component);
				match = true;
				break;
			}
		}

		// Only perform initialization if a matching old component wasn't copied.
		if (!match) {
			CF_MEMSET(new_component, 0, config->size_of_component);
			if (config->initializer) {
				config->initializer(entity, new_component, config->initializer_udata);
			}
		}
	}

	// Cleanup the old components.
	for (int i = old_tuple.count() - 1; i >= 0 ; --i) {
		const char* old_component_type = old_tuple[i];
		CF_ComponentConfig* config = app->component_configs.try_find(old_component_type);
		
		// Look for a matching old component.
		bool match = false;
		for (int i = 0; i < new_tuple.count(); ++i) {
			const char* new_component_type = new_tuple[i];
			if (old_component_type == new_component_type) {
				match = true;
				break;
			}
		}

		// Only call cleanup if an old component was not copied over.
		if (!match && config->cleanup) {
			config->cleanup(entity, old_collection->component_tables[i][old_index], config->cleanup_udata);
		}
	}
	
	// Remove handle from the old collection.
	old_collection->entity_handles.unordered_remove(old_index);

	// Remove the old components.
	for (int i = 0; i < old_collection->component_tables.count(); ++i) {
		old_collection->component_tables[i].unordered_remove(old_index);
	}

	// Update handle of the swapped entity.
	if (old_index < old_collection->entity_handles.size()) {
		CF_Handle h = old_collection->entity_handles[old_index];
		world->handles.update_index(h, old_index);
	}

	// Set the entity's handle to it's new index in the new collection, and update its type.
	world->handles.update_index(entity.handle, new_index);
	world->handles.update_type(entity.handle, new_type);
}

void* cf_entity_get_component(CF_Entity entity, const char* component_type)
{
	CF_WorldInternal* world = s_world();
	CF_EntityCollection* collection = s_collection(entity);
	if (!collection) return NULL;

	component_type = sintern(component_type);
	const Array<const char*>& component_type_tuple = collection->component_type_tuple;
	for (int i = 0; i < component_type_tuple.count(); ++i) {
		if (component_type_tuple[i] == component_type) {
			int index = world->handles.get_index(entity.handle);
			return collection->component_tables[i][index];
		}
	}

	return NULL;
}

bool cf_entity_has_component(CF_Entity entity, const char* component_type)
{
	return cf_entity_get_component(entity, component_type) ? true : false;
}

void cf_entity_type_rename(const char* entity_type, const char* new_entity_type_name)
{
	entity_type = sintern(entity_type);
	new_entity_type_name = sintern(new_entity_type_name);
	CF_EntityType* type = app->entity_type_string_to_id.try_find(entity_type);
	bool new_type_found = app->entity_type_string_to_id.try_find(new_entity_type_name) ? true : false;
	if (!type && !new_type_found) return;
	app->entity_type_string_to_id.remove(entity_type);
	app->entity_type_string_to_id.insert(new_entity_type_name, *type);
	app->entity_type_id_to_string[*type] = new_entity_type_name;
	if (app->entity_config_builder.entity_type == entity_type) {
		app->entity_config_builder.entity_type = new_entity_type_name;
	}
}

//--------------------------------------------------------------------------------------------------

static inline int s_match(const Array<const char*>& a, const Array<const char*>& b)
{
	int matches = 0;
	for (int i = 0; i < a.count(); ++i) {
		for (int j = 0; j < b.count(); ++j) {
			if (a[i] == b[j]) {
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
				CF_EntityCollection* collection = world->entity_collections.items()[j];
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
					int active_count = collection->component_tables[0].count() - collection->inactive_count;
					update_fn(component_list, active_count, udata);
				}
			}
		}

		if (post_update_fn) post_update_fn(udata);
	}

	// Perform delayed operations.
	for (int i = 0; i < world->delayed_destroy_entities.count(); ++i) {
		CF_Entity e = world->delayed_destroy_entities[i];
		cf_destroy_entity(e);
	}
	world->delayed_destroy_entities.clear();

	for (int i = 0; i < world->delayed_deactivate_entities.count(); ++i) {
		CF_Entity e = world->delayed_deactivate_entities[i];
		cf_entity_deactivate(e);
	}
	world->delayed_deactivate_entities.clear();

	for (int i = 0; i < world->delayed_activate_entities.count(); ++i) {
		CF_Entity e = world->delayed_activate_entities[i];
		cf_entity_activate(e);
	}
	world->delayed_activate_entities.clear();

	for (int i = 0; i < world->delayed_change_type.count(); ++i) {
		CF_ChangeType change = world->delayed_change_type[i];
		const char* type = app->entity_type_id_to_string[change.type];
		cf_entity_change_type(change.entity, type);
	}
	world->delayed_change_type.clear();
}

void cf_component_begin()
{
	app->component_config_builder.clear();
}

void cf_component_end()
{
	app->component_configs.insert(app->component_config_builder.name, app->component_config_builder);
}

void cf_component_rename(const char* component_name, const char* new_component_name)
{
	component_name = sintern(component_name);
	new_component_name = sintern(new_component_name);
	CF_ComponentConfig config;
	CF_ComponentConfig* ptr = app->component_configs.try_find(component_name);
	bool new_name_found = app->component_configs.try_find(new_component_name) ? true : false;
	if (ptr && !new_name_found) {
		config = *ptr;
		config.name = new_component_name;
		app->component_configs.insert(new_component_name, config);
		for (int i = 0; i < app->worlds.count(); ++i) {
			CF_WorldInternal* world = (CF_WorldInternal*)app->worlds[i].id;
			CF_EntityCollection** collections = world->entity_collections.items();
			for (int i = 0; i < world->entity_collections.count(); ++i) {
				CF_EntityCollection* collection = collections[i];
				const char* type = collection->component_type_tuple[i];
				if (type == component_name) {
					collection->component_type_tuple[i] = new_component_name;
				}
			}
		}
		for (int i = 0; i < app->systems.count(); ++i) {
			CF_SystemInternal* system = app->systems + i;
			for (int i = 0; i < system->component_type_tuple.size(); ++i) {
				if (system->component_type_tuple[i] == component_name) {
					system->component_type_tuple[i] = new_component_name;
				}
			}
		}
		if (app->component_config_builder.name == component_name) {
			app->component_config_builder.name = new_component_name;
		}
		for (int i = 0; i < app->component_list.types.count(); ++i) {
			if (app->component_list.types[i] == component_name) {
				app->component_list.types[i] = new_component_name;
			}
		}
	}
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
	CF_EntityCollection* collection = CF_NEW(CF_EntityCollection);
	world->entity_collections.insert(entity_type, collection);
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
	for (int i = 0; i < world->entity_collections.count(); ++i) {
		CF_EntityCollection* collection = world->entity_collections.items()[i];
		collection->~CF_EntityCollection();
		CF_FREE(collection);
	}
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
	CF_EntityCollection* collection = world->entity_collections.find(type);
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
