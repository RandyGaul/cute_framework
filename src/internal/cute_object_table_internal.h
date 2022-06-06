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

#ifndef CUTE_OBJECT_TABLE_INTERNAL_H
#define CUTE_OBJECT_TABLE_INTERNAL_H

#include "cute_defines.h"

#ifdef CUTE_CPP

#include <cute_handle_table.h>
#include <cute_error.h>
#include <cute_array.h>

template <typename T>
struct cf_object_table_t
{
	cf_object_table_t(int reserve_count = 0, void* user_allocator_context = NULL);
	~cf_object_table_t();

	cf_handle_t allocate(const T* object);
	T* get_object(cf_handle_t id) const;
	T* remove_object(cf_handle_t id, int* moved_index);
	T* remove_object(int index);
	bool has_object(cf_handle_t id) const;
	void update_handle(cf_handle_t moved_handle, int moved_index);

	T* get_objects();
	const T* get_objects() const;
	int get_object_count() const;

	cf_handle_allocator_t* m_table;
	cf_array<T> m_objects;
};

//--------------------------------------------------------------------------------------------------

template <typename T>
cf_object_table_t<T>::cf_object_table_t(int reserve_count, void* user_allocator_context)
	: m_table(cf_handle_allocator_make(reserve_count, user_allocator_context))
	, m_objects(reserve_count, user_allocator_context)
{}

template <typename T>
cf_object_table_t<T>::~cf_object_table_t()
{
	cf_handle_allocator_destroy(m_table);
}

template <typename T>
cf_handle_t cf_object_table_t<T>::allocate(const T* object)
{
	int index = m_objects.count();
	T* slot = &m_objects.add();
	CUTE_MEMCPY(slot, object, sizeof(T));
	cf_handle_t handle = cf_handle_allocator_alloc(m_table, index);
	return handle;
}

template <typename T>
T* cf_object_table_t<T>::get_object(cf_handle_t id) const
{
	if (!cf_handle_allocator_is_handle_valid(m_table, id)) {
		return NULL;
	} else {
		int index = (int)cf_handle_allocator_get_index(m_table, id);
		const T* object = &m_objects[index];
		return object;
	}
}

template <typename T>
T* cf_object_table_t<T>::remove_object(cf_handle_t id, int* moved_index)
{
	if (cf_handle_allocator_is_handle_valid(m_table, id)) {
		cf_handle_allocator_free(m_table, id);
		int index = (int)cf_handle_allocator_get_index(m_table, id);
		if (moved_index) *moved_index = index;
		m_objects.unordered_remove(index);
	} else {
		return NULL;
	}
}

template <typename T>
T* cf_object_table_t<T>::remove_object(int index)
{
	m_objects.unordered_remove(index);
	return &m_objects[index];
}

template <typename T>
bool cf_object_table_t<T>::has_object(cf_handle_t id) const
{
	return cf_handle_allocator_is_handle_valid(m_table, id) ? true : false;
}

template <typename T>
void cf_object_table_t<T>::update_handle(cf_handle_t moved_handle, int moved_index)
{
	cf_handle_allocator_update_index(m_table, moved_handle, moved_index);
}

template <typename T>
T* cf_object_table_t<T>::get_objects()
{
	return m_objects.data();
}

template <typename T>
const T* cf_object_table_t<T>::get_objects() const
{
	return m_objects.data();
}

template <typename T>
int cf_object_table_t<T>::get_object_count() const
{
	return m_objects.count();
}

namespace cute
{
template<typename T> using object_table_t = cf_object_table_t<T>;
}


#endif // CUTE_CPP

#endif // CUTE_OBJECT_TABLE_INTERNAL_H
