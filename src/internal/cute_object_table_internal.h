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

#include <cute_handle_table.h>
#include <cute_error.h>
#include <cute_array.h>

namespace cute
{

template <typename T>
struct object_table_t
{
	object_table_t(int reserve_count = 0, void* user_allocator_context = NULL);
	~object_table_t();

	handle_t allocate(const T* object);
	T* get_object(handle_t id) const;
	T* remove_object(handle_t id, int* moved_index);
	T* remove_object(int index);
	bool has_object(handle_t id) const;
	void update_handle(handle_t moved_handle, int moved_index);

	T* get_objects();
	const T* get_objects() const;
	int get_object_count() const;

	handle_table_t* m_table;
	array<T> m_objects;
};

//--------------------------------------------------------------------------------------------------

template <typename T>
object_table_t<T>::object_table_t(int reserve_count, void* user_allocator_context)
	: m_table(handle_table_make(reserve_count, user_allocator_context))
	, m_objects(reserve_count, user_allocator_context)
{
}

template <typename T>
object_table_t<T>::~object_table_t()
{
	handle_table_destroy(m_table);
}

template <typename T>
handle_t object_table_t<T>::allocate(const T* object)
{
	int index = m_objects.count();
	T* slot = &m_objects.add();
	CUTE_MEMCPY(slot, object, sizeof(T));
	handle_t handle = handle_table_alloc(m_table, index);
	return handle;
}

template <typename T>
T* object_table_t<T>::get_object(handle_t id) const
{
	if (!handle_table_is_valid(m_table, id)) {
		return NULL;
	} else {
		int index = (int)handle_table_get_index(m_table, id);
		const T* object = &m_objects[index];
		return object;
	}
}

template <typename T>
T* object_table_t<T>::remove_object(handle_t id, int* moved_index)
{
	if (handle_table_is_valid(m_table, id)) {
		handle_table_free(m_table, id);
		int index = (int)handle_table_get_index(m_table, id);
		if (moved_index) *moved_index = index;
		m_objects.unordered_remove(index);
	} else {
		return NULL;
	}
}

template <typename T>
T* object_table_t<T>::remove_object(int index)
{
	m_objects.unordered_remove(index);
	return &m_objects[index];
}

template <typename T>
bool object_table_t<T>::has_object(handle_t id) const
{
	return handle_table_is_valid(m_table, id) ? true : false;
}

template <typename T>
void object_table_t<T>::update_handle(handle_t moved_handle, int moved_index)
{
	handle_table_update_index(m_table, moved_handle, moved_index);
}

template <typename T>
T* object_table_t<T>::get_objects()
{
	return m_objects.data();
}

template <typename T>
const T* object_table_t<T>::get_objects() const
{
	return m_objects.data();
}

template <typename T>
int object_table_t<T>::get_object_count() const
{
	return m_objects.count();
}

}

#endif // CUTE_OBJECT_TABLE_INTERNAL_H
