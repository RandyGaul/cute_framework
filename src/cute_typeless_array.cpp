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

#include <cute_typeless_array.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>

#include <internal/cute_alloc_internal.h>

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
	m_items = CF_ALLOC(m_element_size * m_capacity); 
	CF_ASSERT(m_items);
}

CF_TypelessArray::~CF_TypelessArray()
{
	CF_FREE(m_items);
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
	CF_MEMCPY(slot, item, m_element_size);
	return slot;
}

void* CF_TypelessArray::insert(int index)
{
	CF_ASSERT(index >= 0 && index < m_count);
	add();
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	void* slot_plus_one = (void*)(((uintptr_t)m_items) + (index + 1) * m_element_size);
	int count_to_move = m_count - 1 - index;
	CF_MEMMOVE(slot_plus_one, slot, m_element_size * count_to_move);
	return slot;
}

void* CF_TypelessArray::insert(int index, const void* item)
{
	CF_ASSERT(index >= 0 && index < m_count);
	add();
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	void* slot_plus_one = (void*)(((uintptr_t)m_items) + (index + 1) * m_element_size);
	int count_to_move = m_count - 1 - index;
	CF_MEMMOVE(slot_plus_one, slot, m_element_size * count_to_move);
	CF_MEMCPY(slot, item, m_element_size);
	return slot;
}

void CF_TypelessArray::set(int index, const void* item)
{
	CF_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	CF_MEMCPY(slot, item, m_element_size);
}

void CF_TypelessArray::remove(int index)
{
	CF_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	void* slot_plus_one = (void*)(((uintptr_t)m_items) + (index + 1) * m_element_size);
	int count_to_move = m_count - 1 - index;
	CF_MEMMOVE(slot_plus_one, slot, m_element_size * count_to_move);
}

void* CF_TypelessArray::pop()
{
	CF_ASSERT(m_count > 0);
	void* slot = (void*)(((uintptr_t)m_items) + --m_count);
	return slot;
}
void CF_TypelessArray::unordered_remove(int index)
{
	CF_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	void* slot_last = (void*)(((uintptr_t)m_items) + (m_count - 1) * m_element_size);
	CF_MEMCPY(slot, slot_last, m_element_size);
	--m_count;
}

void CF_TypelessArray::copy(int src, int dst, int count)
{
	CF_ASSERT(src >= 0 && src + count - 1 < m_count);
	CF_ASSERT(dst >= 0 && dst + count - 1 < m_count);
	void* dst_slot = (void*)(((uintptr_t)m_items) + dst * m_element_size);
	void* src_last = (void*)(((uintptr_t)m_items) + src * m_element_size);
	CF_MEMCPY(dst_slot, src_last, m_element_size * count);
}

void CF_TypelessArray::swap(int index_a, int index_b)
{
	CF_ASSERT(index_a >= 0 && index_a < m_count);
	CF_ASSERT(index_b >= 0 && index_b < m_count);
	int index_temp = count();
	add();
	copy(index_a, index_temp);
	copy(index_b, index_a);
	copy(index_temp, index_b);
	pop();
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
			CF_ASSERT(new_capacity); // Detect overflow.
		}

		size_t new_size = m_element_size * new_capacity;
		void* new_items = CF_ALLOC(new_size);
		CF_ASSERT(new_items);
		CF_MEMCPY(new_items, m_items, m_element_size * m_count);
		CF_FREE(m_items);
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
	CF_PLACEMENT_NEW(steal_from_me) CF_TypelessArray();
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
	CF_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	return slot;
}

const void* CF_TypelessArray::operator[](int index) const
{
	CF_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	return slot;
}

void* CF_TypelessArray::operator+(int index)
{
	CF_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	return slot;
}

const void* CF_TypelessArray::operator+(int index) const
{
	CF_ASSERT(index >= 0 && index < m_count);
	void* slot = (void*)(((uintptr_t)m_items) + index * m_element_size);
	return slot;
}

void* CF_TypelessArray::last()
{
	CF_ASSERT(m_count > 0);
	void* slot_last = (void*)(((uintptr_t)m_items) + (m_count - 1) * m_element_size);
	return slot_last;
}

const void* CF_TypelessArray::last() const
{
	CF_ASSERT(m_count > 0);
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
