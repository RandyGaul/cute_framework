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

CF_TypelessArray::CF_TypelessArray()
{
}

CF_TypelessArray::CF_TypelessArray(size_t element_size)
	: m_element_size(element_size)
{
}

CF_TypelessArray::CF_TypelessArray(size_t element_size, int capacity)
	: m_element_size(element_size)
	, m_capacity(capacity)
{
	m_items = CUTE_ALLOC(m_element_size);
	CUTE_ASSERT(m_items);
}

CF_TypelessArray::~CF_TypelessArray()
{
	CUTE_FREE(m_items);
}

void* CF_TypelessArray::add()
{
	ensure_capacity(m_count + 1);
	void* slot = (void*)(((uintptr_t)m_items) + m_count++ * m_element_size);
	return slot;
}

void* CF_TypelessArray::add(const void* item)
{
	ensure_capacity(m_count + 1);
	void* slot = (void*)(((uintptr_t)m_items) + m_count++ * m_element_size);
	CUTE_MEMCPY(slot, item, m_element_size);
	return slot;
}

void* CF_TypelessArray::insert(int index)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	add();
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	void* slot_plus_one = (void*)(((uintptr_t)m_items) + (index + 1) * m_element_size);
	int count_to_move = m_count - 1 - index;
	CUTE_MEMMOVE(slot_plus_one, slot, m_element_size * count_to_move);
	return slot;
}

void* CF_TypelessArray::insert(int index, const void* item)
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

void CF_TypelessArray::set(int index, const void* item)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	CUTE_MEMCPY(slot, item, m_element_size);
}

void CF_TypelessArray::remove(int index)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	void* slot_plus_one = (void*)(((uintptr_t)m_items) + (index + 1) * m_element_size);
	int count_to_move = m_count - 1 - index;
	CUTE_MEMMOVE(slot_plus_one, slot, m_element_size * count_to_move);
}

void* CF_TypelessArray::pop()
{
	CUTE_ASSERT(m_count > 0);
	void* slot = (void*)(((uintptr_t)m_items) + --m_count);
	return slot;
}
void CF_TypelessArray::unordered_remove(int index)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	void* slot_last = (void*)(((uintptr_t)m_items) + (m_count - 1) * m_element_size);
	CUTE_MEMCPY(slot, slot_last, m_element_size);
	--m_count;
}

void CF_TypelessArray::copy(int src, int dst, int count)
{
	CUTE_ASSERT(src >= 0 && src + count - 1 < m_count);
	CUTE_ASSERT(dst >= 0 && dst + count - 1 < m_count);
	void* dst_slot = (void*)(((uintptr_t)m_items) + dst * m_element_size);
	void* src_last = (void*)(((uintptr_t)m_items) + src * m_element_size);
	CUTE_MEMCPY(dst_slot, src_last, m_element_size * count);
}

void CF_TypelessArray::clear()
{
	m_count = 0;
}

void CF_TypelessArray::ensure_capacity(int num_elements)
{
	if (num_elements > m_capacity) {
		int new_capacity = m_capacity ? m_capacity * 2 : 256;
		while (new_capacity < num_elements)
		{
			new_capacity <<= 1;
			CUTE_ASSERT(new_capacity); // Detect overflow.
		}

		size_t new_size = m_element_size * new_capacity;
		void* new_items = CUTE_ALLOC(new_size);
		CUTE_ASSERT(new_items);
		CUTE_MEMCPY(new_items, m_items, m_element_size * m_count);
		CUTE_FREE(m_items);
		m_items = new_items;
		m_capacity = new_capacity;
	}
}

void CF_TypelessArray::steal_from(CF_TypelessArray* steal_from_me)
{
	this->~CF_TypelessArray();
	m_capacity = steal_from_me->m_capacity;
	m_count = steal_from_me->m_count;
	m_items = steal_from_me->m_items;
	CUTE_PLACEMENT_NEW(steal_from_me) CF_TypelessArray();
}

int CF_TypelessArray::capacity() const
{
	return m_capacity;
}

int CF_TypelessArray::count() const
{
	return m_count;
}

void* CF_TypelessArray::operator[](int index)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	return slot;
}

const void* CF_TypelessArray::operator[](int index) const
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	return slot;
}

void* CF_TypelessArray::operator+(int index)
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	return slot;
}

const void* CF_TypelessArray::operator+(int index) const
{
	CUTE_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	return slot;
}

void* CF_TypelessArray::last()
{
	CUTE_ASSERT(m_count > 0);
	void* slot_last = (void*)(((uintptr_t)m_items) + (m_count - 1) * m_element_size);
	return slot_last;
}

const void* CF_TypelessArray::last() const
{
	CUTE_ASSERT(m_count > 0);
	void* slot_last = (void*)(((uintptr_t)m_items) + (m_count - 1) * m_element_size);
	return slot_last;
}

void* CF_TypelessArray::data()
{
	return m_items;
}

const void* CF_TypelessArray::data() const
{
	return m_items;
}
