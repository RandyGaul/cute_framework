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

#include <cute_handle_table.h>
#include <cute_alloc.h>
#include <cute_result.h>
#include <cute_c_runtime.h>
#include <cute_array.h>

#include <internal/cute_alloc_internal.h>

union CF_HandleEntry
{
	struct
	{
		uint64_t user_index : 32;
		uint64_t user_type : 15;
		uint64_t active : 1;
		uint64_t generation : 16;
	} data;
	uint64_t val = 0;
};

struct CF_HandleTable
{
	CF_HandleTable()
		: m_handles()
	{
	}

	uint32_t m_freelist = ~0;
	Cute::Array<CF_HandleEntry> m_handles;
};

static void s_add_elements_to_freelist(CF_HandleTable* table, int first_index, int last_index)
{
	CF_HandleEntry* m_handles = table->m_handles.data();
	for (int i = first_index; i < last_index; ++i)
	{
		CF_HandleEntry handle;
		handle.data.user_index = i + 1;
		handle.data.generation = 0;
		m_handles[i] = handle;
	}

	CF_HandleEntry last_handle;
	last_handle.data.user_index = UINT32_MAX;
	last_handle.data.generation = 0;
	m_handles[last_index] = last_handle;

	table->m_freelist = first_index;
}

CF_HandleTable* cf_make_handle_allocator(int initial_capacity)
{
	CF_HandleTable* table = (CF_HandleTable*)CF_ALLOC(sizeof(CF_HandleTable));
	CF_PLACEMENT_NEW(table) CF_HandleTable();

	if (initial_capacity) {
		table->m_handles.ensure_capacity(initial_capacity);
		int last_index = table->m_handles.capacity() - 1;
		s_add_elements_to_freelist(table, 0, last_index);
	}

	return table;
}

void cf_destroy_handle_allocator(CF_HandleTable* table)
{
	if (!table) return;
	table->~CF_HandleTable();
	CF_FREE(table);
}

CF_Handle cf_handle_allocator_alloc(CF_HandleTable* table, uint32_t index, uint16_t type)
{
	int freelist_index = table->m_freelist;
	if (freelist_index == UINT32_MAX) {
		int first_index = table->m_handles.capacity();
		if (!first_index) first_index = 1;
		table->m_handles.ensure_capacity(first_index * 2);
		int last_index = table->m_handles.capacity() - 1;
		s_add_elements_to_freelist(table, first_index, last_index);
		freelist_index = table->m_freelist;
	}

	// Pop m_freelist.
	CF_HandleEntry* m_handles = table->m_handles.data();
	table->m_freelist = m_handles[freelist_index].data.user_index;

	// Setup handle indices.
	m_handles[freelist_index].data.user_index = index;
	m_handles[freelist_index].data.user_type = type;
	m_handles[freelist_index].data.active = true;
	CF_Handle handle = (((uint64_t)freelist_index) << 32) | (((uint64_t)type) << 16) | m_handles[freelist_index].data.generation;
	return handle;
}

static CF_INLINE uint32_t s_table_index(CF_Handle handle)
{
	return (uint32_t)((handle & 0xFFFFFFFF00000000ULL) >> 32);
}

uint32_t cf_handle_allocator_get_index(CF_HandleTable* table, CF_Handle handle)
{
	CF_HandleEntry* m_handles = table->m_handles.data();
	uint32_t table_index = s_table_index(handle);
	uint64_t generation = handle & 0xFFFF;
	CF_ASSERT(m_handles[table_index].data.generation == generation);
	return m_handles[table_index].data.user_index;
}

uint16_t cf_handle_allocator_get_type(CF_HandleTable* table, CF_Handle handle)
{
	CF_HandleEntry* m_handles = table->m_handles.data();
	uint32_t table_index = s_table_index(handle);
	uint64_t generation = handle & 0xFFFF;
	CF_ASSERT(m_handles[table_index].data.generation == generation);
	return m_handles[table_index].data.user_type;
}

bool cf_handle_allocator_active(CF_HandleTable* table, CF_Handle handle)
{
	CF_HandleEntry* m_handles = table->m_handles.data();
	uint32_t table_index = s_table_index(handle);
	uint64_t generation = handle & 0xFFFF;
	CF_ASSERT(m_handles[table_index].data.generation == generation);
	return m_handles[table_index].data.active;
}

void cf_handle_allocator_activate(CF_HandleTable* table, CF_Handle handle)
{
	CF_HandleEntry* m_handles = table->m_handles.data();
	uint32_t table_index = s_table_index(handle);
	uint64_t generation = handle & 0xFFFF;
	CF_ASSERT(m_handles[table_index].data.generation == generation);
	m_handles[table_index].data.active = true;
}

void cf_handle_allocator_deactivate(CF_HandleTable* table, CF_Handle handle)
{
	CF_HandleEntry* m_handles = table->m_handles.data();
	uint32_t table_index = s_table_index(handle);
	uint64_t generation = handle & 0xFFFF;
	CF_ASSERT(m_handles[table_index].data.generation == generation);
	m_handles[table_index].data.active = false;
}

void cf_handle_allocator_update_index(CF_HandleTable* table, CF_Handle handle, uint32_t index)
{
	CF_HandleEntry* m_handles = table->m_handles.data();
	uint32_t table_index = s_table_index(handle);
	uint64_t generation = handle & 0xFFFF;
	CF_ASSERT(m_handles[table_index].data.generation == generation);
	m_handles[table_index].data.user_index = index;
}

void cf_handle_allocator_free(CF_HandleTable* table, CF_Handle handle)
{
	// Push handle onto m_freelist.
	CF_HandleEntry* m_handles = table->m_handles.data();
	uint32_t table_index = s_table_index(handle);
	m_handles[table_index].data.user_index = table->m_freelist;
	m_handles[table_index].data.generation++;
	table->m_freelist = table_index;
}

int cf_handle_allocator_handle_valid(CF_HandleTable* table, CF_Handle handle)
{
	CF_HandleEntry* m_handles = table->m_handles.data();
	uint32_t table_index = s_table_index(handle);
	uint64_t generation = handle & 0xFFFF;
	uint64_t type = (handle & 0x00000000FFFF0000ULL) >> 16;
	bool match_generation = m_handles[table_index].data.generation == generation;
	bool match_type = m_handles[table_index].data.user_type == type;
	return match_generation && match_type;
}
