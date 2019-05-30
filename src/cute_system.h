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

#ifndef CUTE_SYSTEM_H
#define CUTE_SYSTEM_H

#include <cute_error.h>
#include <cute_array.h>
#include <cute_component.h>

namespace cute
{

template <typename T>
struct system_t
{
	system_t(int max_components, int reserve_count, void* user_allocator_context = NULL);
	~system_t();

	component_id_t add_component(const T& component);
	error_t get_component(component_id_t id, T* component);
	void remove_component(component_id_t id);
	void unordered_remove_component(int index);

	T* get_components();
	int get_components_count();

private:
	int m_max_components;
	handle_table_t m_component_table;
	array<T> m_components;
};

template <typename T>
system_t<T>::system_t(int max_components, int reserve_count, void* user_allocator_context)
	: m_max_components(max_components)
{
	handle_table_init(&m_component_table, max_components, user_allocator_context);
	m_components.ensure_capacity(reserve_count);
}
template <typename T>
system_t<T>::~system_t()
{
	handle_table_cleanup(&m_component_table);
}

template <typename T>
component_id_t system_t<T>::add_component(const T& component)
{
	int index = m_components.count();
	CUTE_ASSERT(index < m_max_components);
	T& c = m_components.add(component);
	handle_t handle = handle_table_alloc(&m_component_table, index);
	CUTE_ASSERT(handle != CUTE_INVALID_HANDLE);
	c.id = handle;
	return handle;
}

template <typename T>
error_t system_t<T>::get_component(component_id_t id, T* component)
{
	if (!handle_table_is_valid(&m_component_table, id)) {
		return error_failure("Tried to get component with invalid id.");
	} else {
		int index = (int)handle_table_get_index(&m_component_table, id);
		*component = m_components[index];
		return error_success();
	}
}

template <typename T>
void system_t<T>::remove_component(component_id_t id)
{
	if (handle_table_is_valid(&m_component_table, id)) {
		int index = (int)handle_table_get_index(&m_component_table, id);
		m_components.unordered_remove(index);
		handle_table_free(&m_component_table, id);

		int moved_index = index;
		if (moved_index != m_components.count()) {
			component_id_t moved_handle = m_components[moved_index].id;
			handle_table_update_index(&m_component_table, moved_handle, moved_index);
		}
	}
}

// WORKING HERE
// Realized I need to lookup components by entity id.
// Perhaps the entity can store an array of component id's, index'd by component type.
// Which means the entity system would need to be run-time initialized and have code to store an array of variable length entities.

}

#endif // CUTE_SYSTEM_H
