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

#include <cute_typeless_array.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>

namespace cute
{

cf_typeless_array::cf_typeless_array()
{
}

cf_typeless_array::cf_typeless_array(size_t element_size, void* user_allocator_context)
	: m_element_size(element_size)
	, m_mem_ctx(user_allocator_context)
{
}

cf_typeless_array::cf_typeless_array(size_t element_size, int capacity, void* user_allocator_context)
	: m_element_size(element_size)
	, m_capacity(capacity)
	, m_mem_ctx(user_allocator_context)
{
	m_items = CUTE_ALLOC(m_element_size, m_mem_ctx);
	CUTE_ASSERT(m_items);
}

cf_typeless_array::~cf_typeless_array()
{
	CUTE_FREE(m_items, m_mem_ctx);
}

void* cf_typeless_array::add()
{
	ensure_capacity(m_count + 1);
	void* slot = (void*)(((uintptr_t)m_items) + m_count++ * m_element_size);
	return slot;
}

void* cf_typeless_array::add(const void* item)
{
	ensure_capacity(m_count + 1);
	void* slot = (void*)(((uintptr_t)m_items) + m_count++ * m_element_size);
	CUTE_MEMCPY(slot, item, m_element_size);
	return slot;
}

void* cf_typeless_array::insert(int index)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	add();
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	void* slot_plus_one = (void*)(((uintptr_t)m_items) + (index + 1) * m_element_size);
	int count_to_move = m_count - 1 - index;
	CUTE_MEMMOVE(slot_plus_one, slot, m_element_size * count_to_move);
	return slot;
}

void* cf_typeless_array::insert(int index, const void* item)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	add();
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	void* slot_plus_one = (void*)(((uintptr_t)m_items) + (index + 1) * m_element_size);
	int count_to_move = m_count - 1 - index;
	CUTE_MEMMOVE(slot_plus_one, slot, m_element_size * count_to_move);
	CUTE_MEMCPY(slot, item, m_element_size);
	return slot;
}

void cf_typeless_array::set(int index, const void* item)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	CUTE_MEMCPY(slot, item, m_element_size);
}

void cf_typeless_array::remove(int index)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	void* slot_plus_one = (void*)(((uintptr_t)m_items) + (index + 1) * m_element_size);
	int count_to_move = m_count - 1 - index;
	CUTE_MEMMOVE(slot_plus_one, slot, m_element_size * count_to_move);
}

void* cf_typeless_array::pop()
{
	CUTE_ASSERT(m_count > 0);
	void* slot = (void*)(((uintptr_t)m_items) + --m_count);
	return slot;
}
void cf_typeless_array::unordered_remove(int index)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	void* slot_last = (void*)(((uintptr_t)m_items) + (m_count - 1) * m_element_size);
	CUTE_MEMCPY(slot, slot_last, m_element_size);
	--m_count;
}

void cf_typeless_array::clear()
{
	m_count = 0;
}

void cf_typeless_array::ensure_capacity(int num_elements)
{
	if (num_elements > m_capacity) {
		int new_capacity = m_capacity ? m_capacity * 2 : 256;
		while (new_capacity < num_elements)
		{
			new_capacity <<= 1;
			CUTE_ASSERT(new_capacity); // Detect overflow.
		}

		size_t new_size = m_element_size * new_capacity;
		void* new_items = CUTE_ALLOC(new_size, m_mem_ctx);
		CUTE_ASSERT(new_items);
		CUTE_MEMCPY(new_items, m_items, m_element_size * m_count);
		CUTE_FREE(m_items, m_mem_ctx);
		m_items = new_items;
		m_capacity = new_capacity;
	}
}

void cf_typeless_array::steal_from(cf_typeless_array* steal_from_me)
{
	this->~cf_typeless_array();
	m_capacity = steal_from_me->m_capacity;
	m_count = steal_from_me->m_count;
	m_items = steal_from_me->m_items;
	m_mem_ctx = steal_from_me->m_mem_ctx;
	CUTE_PLACEMENT_NEW(steal_from_me) cf_typeless_array();
}

int cf_typeless_array::capacity() const
{
	return m_capacity;
}

int cf_typeless_array::count() const
{
	return m_count;
}

void* cf_typeless_array::operator[](int index)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	return slot;
}

const void* cf_typeless_array::operator[](int index) const
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	return slot;
}

void* cf_typeless_array::operator+(int index)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	return slot;
}

const void* cf_typeless_array::operator+(int index) const
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	return slot;
}

void* cf_typeless_array::last()
{
	CUTE_ASSERT(m_count > 0);
	void* slot_last = (void*)(((uintptr_t)m_items) + (m_count - 1) * m_element_size);
	return slot_last;
}

const void* cf_typeless_array::last() const
{
	CUTE_ASSERT(m_count > 0);
	void* slot_last = (void*)(((uintptr_t)m_items) + (m_count - 1) * m_element_size);
	return slot_last;
}

void* cf_typeless_array::data()
{
	return m_items;
}

const void* cf_typeless_array::data() const
{
	return m_items;
}

}
