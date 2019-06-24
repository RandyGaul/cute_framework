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

#ifndef CUTE_ECS_H
#define CUTE_ECS_H

#include <cute_error.h>
#include <cute_handle_table.h>
#include <cute_array.h>

#include <internal/cute_object_table_internal.h>

#define CUTE_ENTITY_MAX_COMPONENTS (16)

namespace cute
{

using entity_id_t = handle_t;
using entity_type_t = uint32_t;
using component_id_t = handle_t;
using component_type_t = uint32_t;

#define CUTE_INVALID_ENTITY_ID CUTE_INVALID_HANDLE
#define CUTE_INVALID_ENTITY_TYPE ((entity_type_t)(~0))
#define CUTE_INVALID_COMPONENT_ID CUTE_INVALID_HANDLE
#define CUTE_INVALID_COMPONENT_TYPE ((component_type_t)(~0))

struct entity_t
{
	entity_type_t type = CUTE_INVALID_ENTITY_TYPE;
	int component_count = 0;
	component_id_t component_id[CUTE_ENTITY_MAX_COMPONENTS];
	component_type_t component_type[CUTE_ENTITY_MAX_COMPONENTS];

	CUTE_INLINE void add(component_id_t id, component_type_t type)
	{
		CUTE_ASSERT(component_count < CUTE_ENTITY_MAX_COMPONENTS);
		component_id[component_count] = id;
		component_type[component_count] = type;
		++component_count;
	}
};

struct entity_schema_t
{
	const char* entity_name;
	entity_t entity;
	struct kv_t* parsed_kv_schema;
};

struct component_t
{
	component_id_t id;
	entity_id_t entity_id;
	entity_type_t entity_type;
};

//--------------------------------------------------------------------------------------------------

struct CUTE_API system_interface_t
{
	CUTE_CALL system_interface_t(const char* name, const char* component_name, component_type_t component_type);
	virtual CUTE_CALL ~system_interface_t() { }

	virtual void CUTE_CALL update(float dt) = 0;

	const char* CUTE_CALL get_name() const;
	const char* CUTE_CALL get_component_name() const;
	component_type_t CUTE_CALL get_component_type() const;

private:
	const char* m_name;
	const char* m_component_name;
	component_type_t m_component_type;
};

//--------------------------------------------------------------------------------------------------

template <typename T>
struct system_t : public system_interface_t
{
	system_t(
		const char* name,
		const char* component_name,
		component_type_t component_type,
		int reserve_count = 1024,
		void* user_allocator_context = NULL
	);
	virtual ~system_t();

	virtual void update(float dt) override { };

	component_id_t add_component(const T* component);
	T* get_component(component_id_t id);
	void remove_component(component_id_t id);
	void remove_component(int index);
	bool has_component(component_id_t id) const;

	void* get_components();
	const void* get_components() const;
	int get_components_count();

private:
	object_table_t<T> m_components;
};

//--------------------------------------------------------------------------------------------------

template <typename T>
system_t<T>::system_t(const char* name, const char* component_name, component_type_t component_type, int reserve_count, void* user_allocator_context)
	: system_interface_t(name, component_name, component_type)
	, m_components(reserve_count, user_allocator_context)
{
}

template <typename T>
system_t<T>::~system_t()
{
}

template <typename T>
component_id_t system_t<T>::add_component(const T* component)
{
	return m_components.allocate(component);
}

template <typename T>
T* system_t<T>::get_component(component_id_t id)
{
	return m_components.get_object(id);
}

template <typename T>
void system_t<T>::remove_component(component_id_t id)
{
	int moved_index = ~0;
	T* component = m_components.remove_object(id, &moved_index);
	if (component) {
		CUTE_ASSERT(moved_index != ~0);
		component_id_t moved_handle = component->id;
		handle_table_update_index(m_components.m_table, moved_handle, moved_index);
	}
}

template <typename T>
void system_t<T>::remove_component(int index)
{
	void* object = m_objects.remove_object(index);
	component_t* component = (component_t*)object;
	component_id_t moved_handle = component->id;
	handle_table_update_index(m_components.m_table, moved_handle, index);
}

template <typename T>
bool system_t<T>::has_component(component_id_t id) const
{
	return m_components.has_object(id);
}

template <typename T>
void* system_t<T>::get_components()
{
	return m_components.get_objects();
}

template <typename T>
const void* system_t<T>::get_components() const
{
	return m_components.get_objects();
}

template <typename T>
int system_t<T>::get_components_count()
{
	return m_components.get_object_count();
}

//--------------------------------------------------------------------------------------------------

extern CUTE_API void CUTE_CALL app_add_system(app_t* app, system_interface_t* system);
extern CUTE_API system_interface_t* CUTE_CALL app_get_system(app_t* app, const char* system_name);
extern CUTE_API void CUTE_CALL app_set_update_systems_for_me_flag(app_t* app, bool true_to_update_false_to_do_nothing);

//--------------------------------------------------------------------------------------------------

struct kv_t;

typedef void (component_initialize_fn)(component_t* component);
typedef error_t (component_serialize_fn)(kv_t* kv, component_t* component);

#define CUTE_COMPONENT_MAX_DEPENDENCIES (16)

struct component_config_t
{
	const char* component_name = NULL;
	component_type_t component_type = CUTE_INVALID_COMPONENT_TYPE;
	component_initialize_fn* component_initializer = NULL;
	component_serialize_fn* component_serializer = NULL;
	int component_dependency_count = 0;
	component_type_t component_dependencies[CUTE_COMPONENT_MAX_DEPENDENCIES] = { CUTE_INVALID_COMPONENT_TYPE };

	CUTE_INLINE void add_dependency(component_type_t component_type)
	{
		CUTE_ASSERT(component_dependency_count < CUTE_COMPONENT_MAX_DEPENDENCIES);
		component_dependencies[component_dependency_count++] = component_type;
	}
};

extern CUTE_API void CUTE_CALL app_register_component(app_t* app, const component_config_t* component_config);

//--------------------------------------------------------------------------------------------------

extern CUTE_API error_t CUTE_CALL app_register_entity_schema(app_t* app, const char* entity_name, entity_type_t entity_type, const void* schema, int schema_size);
extern CUTE_API error_t CUTE_CALL app_load_entities(app_t* app, const void* memory, int size);

}

#endif // CUTE_ECS_H
